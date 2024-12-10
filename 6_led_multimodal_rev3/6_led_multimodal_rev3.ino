#include <FastLED.h>
#include <SPI.h>
#include <NRFLite.h>

#define LEFT 3 //CLK
#define RIGHT 2 //DT
#define PUSH 4
#define LED_PIN 5
#define NUM_LEDS 6
#define BRIGHTNESS 200

bool brightnessAdjust = 0;

unsigned long difference;
unsigned long demoDifference;
uint8_t ghue = 0;
unsigned long timeOut = 0;
unsigned long lastPacketSent = 0;
int timeLEDstripOn = 3000;

int longPressLength = 800;

int spinInversion = 1;

bool inProgramAddressUpdate = 1;

bool lightStatus = 0;
bool skipTheAdjust = 0;
bool LEDstripOn = 0;
bool firstRun = 1;
bool loadColors = 0;

int res; //for primary layer
int res2; //for secondary layer
int prevRes;
int prevRes2;
int lastSentRes = 0;
int lastSentRes2 = 0;
int delta;

bool unchanged = 0;

int whitesShort[7] = {200, 5 ,300, 2, 600, 700, 3};
int whites[21] = {123, 200, 270, 300, 5, 350, 400, 450, 2, 500, 550, 600, 650, 700, 750, 800, 850, 900, 950, 1000, 3};

int mode = 0;
int numPatterns = 64; //colors whites, patterns


//mode specific numbers
//1
int favoriteIndex = 0;
int numFavorites = 6;
bool firstTime = 0;
//2
//bool buriedWhiteSelect = false;
int whiteRes = 0;
//uint8_t numWhitePointIndex = sizeof(whitesShort) / sizeof(int);
int var = 0;
int pressLength = 0;
bool additionalTime = 0;
bool avoidColorWheel = 0;
//3
bool mode3InstantUpdate = 1; //1 for instant update, 0 for click update
//4
uint8_t gradientIndex = 0;
int levelsBrightness = 24;
unsigned long demoPace;

const static uint8_t RADIO_ID = 112;
const static uint8_t DESTINATION_RADIO_ID = 97;   //97 new 88 old
const static uint8_t PIN_RADIO_CE = 7;
const static uint8_t PIN_RADIO_CSN = 8;

CRGB leds[NUM_LEDS];
CRGB ledsOffStrip[128];
CRGBPalette16 colorScroll;

uint8_t colorIndex[NUM_LEDS];

////circle pcb rev2
int pinDip1 = 10;
int pinDip2 = 9;
int pinDip3 = A0;
int pinDip4 = A1;
int pinDip5 = A2;
int pinDip6 = A3;
int pinDip7 = 1;
int pinDip8 = 0;

DEFINE_GRADIENT_PALETTE( Green_3D_1_gp ) { //leaves
  0, 120, 220, 120,
  42,  45, 147, 48,
  84,  13, 114, 14,
  127,   2, 95,  2,
  170,   0, 80,  0,
  212,   0, 40,  0,
  255,   0, 50, 0
};

//0 80 120 160 190 220
DEFINE_GRADIENT_PALETTE( Special ) {
  0, 255,   0,   0, //red
  42,  86, 213,   0, //lime green
  84,   0, 191,  64, //cyan
  126,   0,   0, 255, //blue
  168,  80,   0, 175, //purple
  210, 160,   0,  96, //magenta
  255, 255,   0,   0
}; //red

DEFINE_GRADIENT_PALETTE( Caribbean_Blues_gp ) {
    0,   0,  2, 66,
  102,   1, 51, 88,
  144,   3,169,114,
  163,  67,207,142,
  173, 255,248,174,
  176, 190,217,132,
  178, 159,199,111,
  181, 135,186, 95,
  183, 117,175, 82,
  186, 100,164, 71,
  188,  87,156, 62,
  191,  75,147, 53,
  194,  65,139, 47,
  196,  56,133, 41,
  199,  48,125, 35,
  201,  41,119, 30,
  204,  35,114, 26,
  206,  30,108, 22,
  209,  26,103, 19,
  211,  21, 97, 16,
  214,  18, 92, 13,
  216,  15, 88, 11,
  219,  12, 84,  9,
  221,   9, 80,  7,
  224,   7, 75,  6,
  227,   6, 72,  4,
  229,   4, 69,  3,
  232,   3, 65,  2,
  234,   2, 61,  2,
  237,   1, 58,  1,
  239,   1, 55,  1,
  242,   1, 53,  1,
  244,   1, 50,  1,
  247,   1, 47,  1,
  249,   1, 45,  1,
  252,   1, 42,  1,
  255,   0, 40,  0};

CRGBPalette16 leaves = Green_3D_1_gp;
CRGBPalette16 cbPal = Caribbean_Blues_gp;
CRGBPalette16 anchor = Special;

struct RadioPacket // Any packet up to 32 bytes can be sent.
{
    uint8_t brightKey;
    uint16_t zone;
    uint16_t qcomm; //quick command or pattern 
    uint16_t color;
    uint16_t speed;
    uint16_t extra;
    uint16_t rh;
    uint16_t gs;
    uint16_t bv;
    uint8_t rh2;
    uint8_t gs2;
    uint8_t bv2;
    uint8_t ext1;
    uint8_t ext2;
    uint8_t ext3;
    uint8_t ext4;
    uint8_t sw1;
    uint8_t sw2;
    uint16_t randfactor;
    uint8_t formfactor; //Specify whether a command is meant for a strip or pod or shadow projector, etc.
    uint8_t cclass; //Specify how white only lights will respond with color lights
    /*
     * colorclass, cclass
     * 0 color and white lights respond, color to color commands, whites to brightness/colorTemp only
     * 1 color lights only respond
     * 2 color lights respond, turn off all white lights, could be used to switch whites >> color
     * 
     * 3 white lights only respond
     * 4 white lights respond, suppress color lights, could be used to switch color >> whites
     */
    uint8_t rev; //if received rev is <= defined rev, signal can be interpreted
    //SOFTWARE_REVISION   indicates upto what level of commands the light module possesses
    /*
     * active grid modules will be assigned to the same room, DIP 7-8
     * said modules will be given a grid position, DIP 1-6, this sets agnum
     * active grid commands will be sent to a room, then the associated actriveGrid _.qcomm (200-300) will use agnum to govern its pattern
     */
};

NRFLite _base;
RadioPacket _satelliteData;
RadioPacket _brightnessData;
RadioPacket _lastPacket;

void turnOff() {
  if(LEDstripOn) {
    for (int i = 0; i <= BRIGHTNESS; i++) {
      FastLED.setBrightness(BRIGHTNESS - i);
      ledsProject();
      delay(1);
    }
    fill_solid(leds, NUM_LEDS, CRGB(0,0,0));
    ledsProject();
    LEDstripOn = 0;
    //mode = 0;
  }
}

void turnOn() {
  if(!LEDstripOn) {
    for (int i = 0; i <= BRIGHTNESS; i++) {
      FastLED.setBrightness(i);
      ledsProject();
      if(i % 4 == 0) {
        delay(1);
      }
      if(rotary() != 0) {
        break;
      }
    }
    FastLED.setBrightness(BRIGHTNESS);
    ledsProject();
    LEDstripOn = 1;
  }
}

uint8_t lrmem = 3;
int lrsum = 0;
int rotary()
{
  static int8_t TRANS[] = {0, -1, 1, 14, 1, 0, 14, -1, -1, 14, 0, 1, 14, 1, -1, 0};
  int8_t l, r;

  l = digitalRead(LEFT);
  r = digitalRead(RIGHT);
  lrmem = ((lrmem & 0x03) << 2) + 2 * l + r;
  lrsum = lrsum + TRANS[lrmem];
  /* encoder not in the neutral state */
  if (lrsum % 4 != 0) return (0);
  /* encoder in the neutral state */

  if (lrsum == 4) {
    lrsum = 0;
    return (1);
  }
  if (lrsum == -4) {
    lrsum = 0;
    return (-1);
  }
  /* lrsum > 0 if the impossible transition */
  lrsum = 0;
  return (0);
}

void zeroOut() {
  _satelliteData.qcomm = 0;
  _satelliteData.color = 0;
  _satelliteData.speed = 0;
  _satelliteData.extra = 0;
  _satelliteData.rh = 0;
  _satelliteData.gs = 0;
  _satelliteData.bv = 0;
  _satelliteData.rh2 = 0;
  _satelliteData.gs2 = 0;
  _satelliteData.bv2 = 0;
  _satelliteData.ext1 = 0;
  _satelliteData.ext2 = 0;
  _satelliteData.ext3 = 0;
  _satelliteData.randfactor = 0;
}

void tallyAddress() {
  _satelliteData.zone = 0;
  if (!digitalRead(pinDip1)) {
    _satelliteData.zone += 1;
  }
  if (!digitalRead(pinDip2)) {
    _satelliteData.zone += 2;
  }
  if (!digitalRead(pinDip3)) {
    _satelliteData.zone += 4;
  }
  if (!digitalRead(pinDip4)) {
    _satelliteData.zone += 8;
  }
  if(!digitalRead(pinDip5)) {
    _satelliteData.zone += 16;
  }
  if(!digitalRead(pinDip6)) {
    _satelliteData.zone += 32;
  }
  _brightnessData.zone = _satelliteData.zone;
}

void modeTally() {
  mode = 0;
  if (!digitalRead(pinDip7)) {
    mode += 1;
  }
  if (!digitalRead(pinDip8)) {
    mode += 2;
  }
  res = 0;
  
  switch(mode) {
    case 0: //top lev brightness, favorite colors/patterns click (pre-order spec), click and hold for off
      numPatterns = 16;
      prevRes = NUM_LEDS;
      res = 0;
      prevRes2 = 15;
      res2 = 0;
      break;
    case 1: //top lev brightness, favorite whites click, click and hold for off
      numPatterns = 24;
      prevRes = 15;
      res = 0;
      prevRes2 = 15;
      res2 = 0;     
      break;
    case 2: //top lev color wheel, default white click, click and twist for brightness, click to toggle on/off
      numPatterns = 32;
      prevRes = 31;
      res = 0;
      prevRes2 = NUM_LEDS;
      res2 = 0;
      break;
    case 3: //single layer colors whites and patterns, click is toggle on off
      //numPatterns = 67;
      numPatterns = 16 + 7 + 14;
      prevRes = 66;
      res = 0;
      prevRes2 = 15;
      res2 = 0;
      break;
  }
}

void ledsProject() {
  for(int i = 0; i < 128; i++) {
    ledsOffStrip[i] = leds[0];
  }
  FastLED.show();
}

void setup() {
  //Serial.begin(115200);
  
  pinMode(pinDip1, INPUT_PULLUP);
  pinMode(pinDip2, INPUT_PULLUP);
  pinMode(pinDip3, INPUT_PULLUP);
  pinMode(pinDip4, INPUT_PULLUP);
  pinMode(pinDip5, INPUT_PULLUP);
  pinMode(pinDip6, INPUT_PULLUP);
  pinMode(pinDip7, INPUT_PULLUP);
  pinMode(pinDip8, INPUT_PULLUP);

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  //FastLED.addLeds<WS2812B, 6, GRB>(ledsOffStrip, 128);

  pinMode(LEFT, INPUT);
  pinMode(RIGHT, INPUT);
  pinMode(PUSH, INPUT);
  pinMode(LEFT, INPUT_PULLUP);
  pinMode(RIGHT, INPUT_PULLUP);
  pinMode(PUSH, INPUT_PULLUP);
  
  delay(500);
  difference = millis();
  if(!_base.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN, NRFLite::BITRATE2MBPS)) {
    while (1) {
      //Serial.println("mjj");
      leds[0] = CRGB::Red;
      ledsProject();
      delay(500);
      leds[0] = CRGB::Black;
      ledsProject();
      delay(500);
    }
  }
  //Serial.println("good");
  //initialize all radio vars to 0
  zeroOut();

  demoPace = millis();  
  
  delta = 0;
  
  for (int i = 0; i < NUM_LEDS; i++) { //make it so that pallettes arent solid colored
    colorIndex[i] = random8();
  }

  //types of lighting device
  _satelliteData.formfactor = 0;
  _satelliteData.cclass = 0;
  //software revision / pattern support
  _satelliteData.rev = 1;
  
  _satelliteData.brightKey = 255;

  _brightnessData.formfactor = 0;
  _brightnessData.cclass = 0;
  _brightnessData.rev = 1;
  _brightnessData.brightKey = 255;

  //read in zone from dip switches
  tallyAddress();
  modeTally();
  
  delay(500);
  FastLED.setBrightness(BRIGHTNESS);

  //Serial.println(_satelliteData.zone);
}

void sendPacket() {
  if(millis() - lastPacketSent > 10) {
    _base.send(DESTINATION_RADIO_ID, &_satelliteData, sizeof(_satelliteData), NRFLite::NO_ACK);
    lastSentRes = res;
    lastSentRes2 = res2;
    if(_satelliteData.qcomm >= 10) {
      _lastPacket = _satelliteData;
    }
    lastPacketSent = millis();
  }
}

void displayWhiteTemp(int colorKey) {
  switch(colorKey) { //custom whites, incandescent, pastels maybe
    case 0: //pure white
      fill_solid(leds, NUM_LEDS, CRGB(255,255,255));
      break;
    case 1: //greenish white
      fill_solid(leds, NUM_LEDS, CRGB(250,203,72));
      break;
    case 2: //good work light
      fill_solid(leds, NUM_LEDS, CRGB(255,139,78));
      break;
    case 3: //purplish white
      fill_solid(leds, NUM_LEDS, CHSV(176,135,255));
      break;
    case 4: //golden
      fill_solid(leds, NUM_LEDS, CRGB(255,120,25));
      break;
    case 5: //salmon incandescent
      fill_solid(leds, NUM_LEDS, CRGB(255,73,21));
      break;
    case 6: //moonlight
      
      break;
    //color temps here
    case 200: //2000K
      fill_solid(leds, NUM_LEDS, CRGB(255,57,4));
      break;
    case 270: //2700K
      fill_solid(leds, NUM_LEDS, CRGB(255,78,8));
      break;
    case 300: //3000K
      fill_solid(leds, NUM_LEDS, CRGB(255,87,15));
      break;
    case 350: //3500K
      fill_solid(leds, NUM_LEDS, CRGB(255,100,27));
      break;
    case 400: //4000K
      fill_solid(leds, NUM_LEDS, CRGB(255,106,44));
      break;
    case 450: //4500K
      fill_solid(leds, NUM_LEDS, CRGB(255,132,56));
      break;
    case 500: //5000K
      fill_solid(leds, NUM_LEDS, CRGB(255,157,90));
      break;
    case 550: //5500K
      fill_solid(leds, NUM_LEDS, CRGB(255,138,98));
      break;
    case 600: //6000K
      fill_solid(leds, NUM_LEDS, CRGB(255,166,149));
      break;
    case 650: //6500K
      fill_solid(leds, NUM_LEDS, CRGB(239,164,173));
      break;
    case 700: //7000K
      fill_solid(leds, NUM_LEDS, CRGB(220,181,192));
      break;
    case 750: //7500K
      fill_solid(leds, NUM_LEDS, CRGB(219,198,205));
      break;
    case 800: //8000K
      fill_solid(leds, NUM_LEDS, CRGB(202,212,239));
      break;
    case 850: //8500K
      fill_solid(leds, NUM_LEDS, CRGB(179,155,255));
      break;
    case 900: //9000K
      fill_solid(leds, NUM_LEDS, CRGB(111,148,255));
      break;
    case 950: //9500K
      fill_solid(leds, NUM_LEDS, CRGB(94,125,255));
      break;
    case 1000: //1000K
      fill_solid(leds, NUM_LEDS, CRGB(37,82,255));
      break; 
  }
}

void loop() {
  if(millis() % 500 == 0 && inProgramAddressUpdate) {
    tallyAddress();
    //tallyMode();
  }

  switch(mode) {
    case 0:
      //
      //check knob for new state
      //
      if (millis() - timeOut > (timeLEDstripOn + 2000) && firstTime) {
        turnOff();
        
        if(favoriteIndex >= 3 && !loadColors && lightStatus) {
          _base.send(DESTINATION_RADIO_ID, &_lastPacket, sizeof(_lastPacket), NRFLite::NO_ACK);
        }
        loadColors = 0;
        firstTime = 0;
      }
      
      if(loadColors) {
        switch(favoriteIndex) {
          case 0:
            fill_solid(leds, NUM_LEDS, CRGB(34,224,54));
            ledsProject();
            break;
          case 1:
            fill_solid(leds, NUM_LEDS, CHSV(240,145,255));
            ledsProject();
            break;
          case 2:
            fill_solid(leds, NUM_LEDS, CRGB(30,6,250));
            ledsProject();
            break;
          case 3:
            for (int i = 0; i < NUM_LEDS; i++) {
              leds[i] = ColorFromPalette(leaves, colorIndex[i], 255);
            }
            ledsProject();
            //makes the palette move
            //delay(_satelliteData.speed);
            if (millis() % 5 == 0) {
              for (int i = 0; i < NUM_LEDS; i++) {
                colorIndex[i]++;
              }
            }
            break;
          case 4:
            if(firstRun) {
              if (millis() % 5 == 0) {
                for (int i = 0; i < NUM_LEDS; i++) {
                  if (i > (2 * (NUM_LEDS / 3))) {
                    leds[i] = CHSV(13, 255, random(50, 255));
                  } else if (i > (NUM_LEDS / 3)) {
                    leds[i] = CHSV(165, 255, random(50, 255));
                  } else {
                    leds[i] = CHSV(190, 255, random(50, 255));
                  }
                  colorIndex[i]++;
                }
                firstRun = 0;
              }
              ledsProject();
            }
            break;
          case 5:
            fill_rainbow(leds, NUM_LEDS, ghue, 10);
            ledsProject();
            if (millis() % 10 == 0) {
              ghue++;
            }
            break;
        }
        firstTime = 1;
        turnOn();
      }

      prevRes = res;
      res += rotary() * spinInversion;
      if (prevRes != res) {
        if (res < 0) {
          res = 0;
        }
        if (res >= NUM_LEDS) {
          res = NUM_LEDS - 1;
        }
        loadColors = 0;
        for (int i = 0; i < NUM_LEDS; i++) {
          if (i <= res) {
            leds[i] = CRGB::Blue;
          } else {
            leds[i] = CRGB::Black;
          }
        }
        ledsProject();
        turnOn();
        firstTime = 1;
        _brightnessData.qcomm = 2;
        _brightnessData.extra = 5;
        uint8_t bKey = ( ((float) res) / (NUM_LEDS - 1) * 255);
        if (bKey > 255) {
          bKey = 255;
          lightStatus = 1;
        } else if (bKey < 1) {
          bKey = 0;
          lightStatus = 0;
        } else {
          lightStatus = 1;
        }
        _brightnessData.color = bKey;
        _brightnessData.speed = 1;
        _base.send(DESTINATION_RADIO_ID, &_brightnessData, sizeof(_brightnessData), NRFLite::NO_ACK);
        timeOut = millis();
      }
      //
      //end of top level scroll
      //
      
      //
      //you clicked button
      //
      if (!digitalRead(PUSH)) {
        delay(30);
        difference = millis();
        while (!digitalRead(PUSH) ) {
          //
          //long press
          //
          if (millis() - difference > longPressLength) {
            lightStatus = 0;
            favoriteIndex = 0;
            turnOff();
            _satelliteData.qcomm = 0;
            _satelliteData.speed = 0;
            _satelliteData.extra = 0;
            sendPacket();
            skipTheAdjust = 1;
            res = 0;
            delay(30);
            while(!digitalRead(PUSH)) {}
            delay(30);
            break;
          }
          //
          //end of long press
          //
          //click and twist
          //
          prevRes2 = res2;
          res2 += rotary() * spinInversion * 6;          
          if(res2 != prevRes2) {
            fill_solid(leds, NUM_LEDS, CHSV(res2, 255, 255));
            ledsProject();
            turnOn();
            zeroOut();
            while(!digitalRead(PUSH)) {
              prevRes2 = res2;
              res2 += rotary() * spinInversion * 6;
              if(res2 < 0) {
                res2 = 255;
              }
              res2 = res2 % 256;
              if(res2 != prevRes2) {
                _satelliteData.qcomm = 11;
                _satelliteData.color = res2;
                fill_solid(leds, NUM_LEDS, CHSV(res2, 255, 255));
                ledsProject();
                sendPacket();
              }
              lightStatus = 1;
              firstTime = 1;
            }
            skipTheAdjust = 1;
            timeOut = millis();
            delay(30);
          }
          //
          //end of click and twist
          //
        }
        //if a long click event was registered, don't double count a short click
        if (!skipTheAdjust) {
          //
          //single short click
          //
          if (lightStatus) {
            //click, already on
            //lights are on, cycle through favorites
            //BEGIN FAVORITE PATTERNS
            favoriteIndex++;
            firstRun = 1;
            zeroOut();
            favoriteIndex = favoriteIndex % numFavorites;
            if(favoriteIndex == 0) {
              //RGB 34,224,54
              _satelliteData.qcomm = 11;
              _satelliteData.color = 256;
              _satelliteData.extra = 0;
              _satelliteData.rh = 34;
              _satelliteData.gs = 224;
              _satelliteData.bv = 54;
            } else if(favoriteIndex == 1) {
              //HSV 240,145,255
              _satelliteData.qcomm = 11;
              _satelliteData.color = 256;
              _satelliteData.extra = 1;
              _satelliteData.rh = 240;
              _satelliteData.gs = 145;
              _satelliteData.bv = 255;
            } else if(favoriteIndex == 2) {
              //RGB 30,6,250
              _satelliteData.qcomm = 11;
              _satelliteData.color = 256;
              _satelliteData.extra = 0;
              _satelliteData.rh = 30;
              _satelliteData.gs = 6;
              _satelliteData.bv = 250;
            } else if(favoriteIndex == 3) {
              //leaves
              _satelliteData.qcomm = 17;
              _satelliteData.extra = 13;
              _satelliteData.speed = 30;
              _satelliteData.randfactor = 0;
            } else if(favoriteIndex == 4) {
              //sunset triangle
              _satelliteData.qcomm = 21;
              _satelliteData.color = 2000;
              _satelliteData.extra = 11;
              _satelliteData.speed = 15;
              _satelliteData.randfactor = 3000;
              _satelliteData.ext1 = 1;
            } else if(favoriteIndex == 5) {
              //zoomy rainbow
              _satelliteData.qcomm = 34;
              _satelliteData.color = 3;
              _satelliteData.ext1 = 8;
              _satelliteData.ext2 = 17;
            }
            loadColors = 1;
            timeOut = millis();
            //END FAVORITE PATTERNS
            sendPacket();
          } else {
            //intro click
            lightStatus = 1;
            res = NUM_LEDS - 1;
            _satelliteData.qcomm = 10;
            _satelliteData.speed = 0;
            _satelliteData.extra = 5;
            sendPacket();
          }
          //
          //end of single short click
          //
        }
        skipTheAdjust = 0;
        delay(20);
      }
      //
      //end of button control
      //
      break;
    case 1: //brightness knob
      if (millis() - timeOut > timeLEDstripOn - 1000) {
        turnOff();
      }
      prevRes = res;
      res += rotary() * spinInversion;
      if (prevRes != res) {
        if (res < 0) {
          res = 0;
        }
        if (res >= numPatterns) {
          res = numPatterns - 1;
        }
        uint8_t fullLED = (res+1) / (numPatterns / NUM_LEDS);  //number of LEDs that will be on
        float partialLED = (256 / (numPatterns / NUM_LEDS)) * ((res+1) % (numPatterns / NUM_LEDS));  //percentage brightness of last LED        
        for(int i = 0; i < fullLED; i++){
          leds[i] = CHSV(160,255,255);
        }
        leds[fullLED] = CHSV(160,255,partialLED);
        ledsProject();
        turnOn();
        _satelliteData.qcomm = 2;
        _satelliteData.extra = 5;
        uint8_t bKey = ( ((float) res) / (numPatterns - 1) * 255);
        if (bKey > 255) {
          bKey = 255;
        } else if (bKey < 1) {
          bKey = 0;
        }
        _satelliteData.color = bKey;
        _satelliteData.speed = 1;
        sendPacket();
        if(bKey > 0 && lightStatus == 0) {
          lightStatus = 1;
        } else if(bKey == 0 && lightStatus == 1) {
          lightStatus = 0;
        }
        timeOut = millis();
      }

      if(!digitalRead(PUSH)) {
        delay(30);
        difference = millis();
        while(!digitalRead(PUSH)) {
          if(additionalTime) {
            pressLength = longPressLength + 3000;
            //avoidColorWheel = 1;
          } else {
            pressLength = longPressLength;
          }
          var = rotary();
          if(millis() - difference > pressLength && !avoidColorWheel) {
            //long press, instant update color wheel
            bool colorAdjustMode = 1;
            fill_solid(leds, NUM_LEDS, CHSV(ghue,255,255));
            ledsProject();
            turnOn();
            while(!digitalRead(PUSH)) {}
            delay(20);
            var = rotary();
            while(colorAdjustMode) {
              var = rotary();
              if(var != 0) {
                ghue += var * 12;
                fill_solid(leds, NUM_LEDS, CHSV(ghue,255,255));
                ledsProject();
                _brightnessData.qcomm = 11;
                _brightnessData.extra = 300;
                _brightnessData.color = ghue;
                _brightnessData.brightKey = 255;
                _base.send(DESTINATION_RADIO_ID, &_brightnessData, sizeof(_brightnessData), NRFLite::NO_ACK);
              }
              if(!digitalRead(PUSH)) {
                colorAdjustMode = 0;
                while(!digitalRead(PUSH)) {}
                turnOff();
                delay(30);
              }
            }
            skipTheAdjust = 1;
          } else if(var != 0) {
            //scroll through longWhites , instant update
            turnOn();
            additionalTime = 1;
            difference = millis();
            whiteRes += var; //think it would go here, but button iterator is lower
            if(whiteRes < 0) {
              whiteRes = 0;
            } else if(whiteRes >= sizeof(whites)/sizeof(int)) {
              whiteRes = sizeof(whites)/sizeof(int) - 1;
            }
            _satelliteData.qcomm = 10;
            _satelliteData.speed = 0;
            _satelliteData.extra = 0;
            if(whites[whiteRes] == 123) {
              _satelliteData.qcomm = 11;
              _satelliteData.color = 8;
              fill_solid(leds, NUM_LEDS, CHSV(8,255,255));
              ledsProject();
            } else {
              _satelliteData.color = whites[whiteRes];
              //update the LED strip on remote
              displayWhiteTemp(_satelliteData.color); 
              ledsProject();
            }
//            whiteRes += var;
//            //whiteRes++;
//            whiteRes = whiteRes % (sizeof(whites)/sizeof(int));  //should it loop?? //NO IT ABS SHOULD NOT
            sendPacket();
            skipTheAdjust = 1;
          }
          
        }
        additionalTime = 0;
        if(!skipTheAdjust) {
          if(lightStatus) { //ignore LED strip status as this needs to be a super seamless mode, scroll for brightness, click for on off, dead simple explanation
            //lights are on, turn them off
            lightStatus = 0;
            res = 0;
            _satelliteData.qcomm = 0;
            _satelliteData.speed = 0;
            _satelliteData.extra = 0;
            sendPacket();
          } else {
            //lights are off, turn them on
            //should it turn on to the most recently used white color temp? yes, as opposed to default white
            lightStatus = 1;
            res = numPatterns - 1;
            _satelliteData.qcomm = 10;
            _satelliteData.speed = 0;
            _satelliteData.extra = 0;
            if(whites[whiteRes] == 123) {
              _satelliteData.qcomm = 11;
              _satelliteData.color = 8;
            } else {
              _satelliteData.color = whites[whiteRes];  
            }
            sendPacket();
          }
        }
        skipTheAdjust = 0;
        delay(20);
      }
      break;
    case 2: //color wheel
      if (millis() - timeOut > timeLEDstripOn) {
        turnOff();
      }
      prevRes = res;
      delta = rotary();
      res += delta;
      if (res != prevRes) {
        if (!LEDstripOn) {
          //turn On
          loadColors = 1;
        }
        firstRun = 1;
        timeOut = millis();
        unchanged = 0;
      }
      if (LEDstripOn || loadColors) {
        if(0) { //0 for circular pattern cycle, 1 for linear pattern cycle firm endStop
          if (res >= numPatterns) {
            res = numPatterns - 1;
          }
          //perhaps include a red flash or something to indicate the end of a list
          if (res < 0) {
            res = 0;
          }
        } else {
          if (res >= numPatterns) {
            res = 0;
          }
          if (res < 0) {
            res = numPatterns - 1;
          }
        }
      }
      if(res != prevRes) { 
        _satelliteData.qcomm = 11;
        _satelliteData.color = res * (256/numPatterns);
        fill_solid(leds, NUM_LEDS, CHSV(_satelliteData.color, 255, 255));
        ledsProject();
      }

      if(mode3InstantUpdate) {  //1 for instant update, 0 for click update
        if(delta != 0) {
          lightStatus = 1;
          sendPacket();
        }
      }
    
      if(loadColors) {
        turnOn();
        loadColors = 0;
      }
    
      if(!digitalRead(PUSH)) {
        delay(30);
        difference = millis();
        _brightnessData = _satelliteData;
        while (!digitalRead(PUSH) ) {
          //difference = millis();
          if (millis() - difference > longPressLength) {
            //brightness adjust mode
            brightnessAdjust = 1;
            fill_solid(leds, NUM_LEDS, CRGB::Black);
            for (int i = 0; i < NUM_LEDS; i++) {
              if (i <= res2) {
                leds[i] = CRGB::Blue;
              } else {
                leds[i] = CRGB::Black;
              }
            }
            FastLED.setBrightness(BRIGHTNESS);
            ledsProject();
            while (!digitalRead(PUSH)) {}
            delay(30);
            while (brightnessAdjust) {
              prevRes2 = res2;
              res2 += rotary() * spinInversion;
              if (res2 < 0) {
                res2 = 0;
              }
              if (res2 >= NUM_LEDS) {
                res2 = NUM_LEDS - 1;
                
              }
              for (int i = 0; i < NUM_LEDS; i++) {
                if (i <= res2) {
                  leds[i] = CRGB::Blue;
                } else {
                  leds[i] = CRGB::Black;
                }
              }
              ledsProject();
              if (!digitalRead(PUSH)) {
                brightnessAdjust = 0;
              }
              _brightnessData.qcomm = 2;
              _brightnessData.extra = 5;
              uint8_t bKey = ( ((float) res2) / (NUM_LEDS - 1) * 255);
              if (bKey > 255) {
                bKey = 255;
              } else if (bKey < 1) {
                bKey = 1;
              }
    
              _brightnessData.color = bKey;
              _brightnessData.speed = 1;
              if (prevRes2 != res2) {
                _base.send(DESTINATION_RADIO_ID, &_brightnessData, sizeof(_brightnessData), NRFLite::NO_ACK);
              }
            }
            if (!LEDstripOn) {
              FastLED.setBrightness(0);
              ledsProject();
            }
            turnOff();
            if(res2 > 0) {
              //_base.send(DESTINATION_RADIO_ID, &_lastPacket, sizeof(_lastPacket), NRFLite::NO_ACK);
              lightStatus = 1;
            } else {
              lightStatus = 0;
            }
            firstRun = 1;
            timeOut = millis();
            skipTheAdjust = 1;
            while (!digitalRead(PUSH)) {}
            delay(30);
          }
        }
        if (!skipTheAdjust) {
          if(LEDstripOn) {
            timeOut = millis();
            if(!lightStatus) {
              res2 = NUM_LEDS - 1;
              prevRes2 = res2;
            }
            lightStatus = 1;
            sendPacket();
          } else {
            if (lightStatus) {
              //lights are on, LED strip off: turn off lights
              lightStatus = 0;
              _satelliteData.qcomm = 0;
              _satelliteData.speed = 0;
              _satelliteData.extra = 0;
              res2 = 0;
              prevRes2 = res2;
              sendPacket();
            } else {
              //lights are off, LED strip is off, send default white
              lightStatus = 1;
              _satelliteData.qcomm = 10;
              _satelliteData.speed = 0;
              _satelliteData.extra = 5;
              res2 = NUM_LEDS - 1;
              prevRes2 = res2;
              sendPacket();
            }
          }
        }
        //timeOut = millis();
        skipTheAdjust = 0;
        delay(20);
      }
      break;
    case 3: //single layer scroll
      if (millis() - timeOut > timeLEDstripOn) {
        turnOff();
      }
      prevRes = res;
      res += rotary();
      if (res != prevRes) {
        if (!LEDstripOn) {
          //turn On
          loadColors = 1;
        }
        firstRun = 1;
        timeOut = millis();
        unchanged = 0;
      }
      if (LEDstripOn || loadColors) {
        if(0) { //0 for circular pattern cycle, 1 for linear pattern cycle
          if (res >= numPatterns) {
            res = numPatterns - 1;
          }
          //perhaps include a red flash or something to indicate the end of a list
          if (res < 0) {
            res = 0;
          }
        } else {
          if (res >= numPatterns) {
            res = 0;
          }
          if (res < 0) {
            res = numPatterns - 1;
          }
        }
        if(res < 16) { //color cycle
          _satelliteData.qcomm = 11;
          _satelliteData.color = res * 16;
          if(res != prevRes) {
            fill_solid(leds, NUM_LEDS, CHSV(_satelliteData.color, 255, 255));
            ledsProject();
          }
        } else if(res < (16 + sizeof(whitesShort)/sizeof(int)) ) { //whites cycle
          _satelliteData.qcomm = 10;
          _satelliteData.extra = 0;
          _satelliteData.color = whitesShort[res - 16];
          if(res != prevRes) {
            if(_satelliteData.color == 123) {
              _satelliteData.qcomm = 11;
              _satelliteData.color = 8;
              fill_solid(leds, NUM_LEDS, CHSV(8,255,255));
              ledsProject();
            } else {
              displayWhiteTemp(_satelliteData.color);
            }
            ledsProject();
          }
        } else {
          zeroOut();
          switch (res) {
            case 0 + (16 + sizeof(whitesShort)/sizeof(int)): //leaves
              _satelliteData.qcomm = 17;
              _satelliteData.extra = 13;
              _satelliteData.speed = 30;
              _satelliteData.sw1 = 0;
              _satelliteData.sw2 = 0;
              _satelliteData.randfactor = 0;
              for (int i = 0; i < NUM_LEDS; i++) {
                leds[i] = ColorFromPalette(leaves, colorIndex[i], 255);
              }
              ledsProject();
              //makes the palette move
              //delay(_satelliteData.speed);
              if (millis() % 5 == 0) {
                for (int i = 0; i < NUM_LEDS; i++) {
                  colorIndex[i]++;
                }
              }
              //ledsProject();
              break;
            case 1 + (16 + sizeof(whitesShort)/sizeof(int)): //leaves but with anchor palette
              _satelliteData.qcomm = 17;
              _satelliteData.extra = 10;
              _satelliteData.speed = 30;
              _satelliteData.sw1 = 0;
              _satelliteData.sw2 = 0;
              _satelliteData.randfactor = 0;
              for (int i = 0; i < NUM_LEDS; i++) {
                leds[i] = ColorFromPalette(anchor, colorIndex[i], 255);
              }
              ledsProject();
              //makes the palette move
              //delay(_satelliteData.speed);
              if (millis() % 5 == 0) {
                for (int i = 0; i < NUM_LEDS; i++) {
                  colorIndex[i]++;
                }
              }
              break;
            case 2 + (16 + sizeof(whitesShort)/sizeof(int)): //HALLOWEEN dipole
              _satelliteData.qcomm = 21;
              _satelliteData.sw1 = 5;
              _satelliteData.sw2 = 0;
              _satelliteData.color = 2000;
              _satelliteData.extra = 5;
              _satelliteData.speed = 40;
              _satelliteData.randfactor = 1000;
              _satelliteData.rh = 13;
              _satelliteData.gs = 195;
              if (firstRun) {
                for (int i = 0; i < NUM_LEDS; i++) {
                  if (i >= (NUM_LEDS / 2) ) {
                    leds[i] = CHSV(13, 255, random(50, 255));
                  } else {
                    leds[i] = CHSV(195, 255, random(50, 255));
                  }
                  firstRun = 0;
                }
                ledsProject();
              }
              break;
            case 3 + (16 + sizeof(whitesShort)/sizeof(int)): //peach and teal dipole
              _satelliteData.qcomm = 21;
              _satelliteData.sw1 = 5;
              _satelliteData.sw2 = 0;
              _satelliteData.color = 2000;
              _satelliteData.extra = 5;
              _satelliteData.speed = 50;
              _satelliteData.randfactor = 1000;
              _satelliteData.rh = 130;
              _satelliteData.gs = 250;
              if (firstRun) {
                for (int i = 0; i < NUM_LEDS; i++) {
                  if (i >= (NUM_LEDS / 2) ) {
                    leds[i] = CHSV(_satelliteData.rh, 255, random(50, 255));
                  } else {
                    leds[i] = CHSV(_satelliteData.gs, 255, random(50, 255));
                  }
                  firstRun = 0;
                }
                ledsProject();
              }
              break;
            case 4 + (16 + sizeof(whitesShort)/sizeof(int)): //vaporwave dipole
              _satelliteData.qcomm = 21;
              _satelliteData.sw1 = 5;
              _satelliteData.sw2 = 0;
              _satelliteData.color = 2000;
              _satelliteData.extra = 0;
              _satelliteData.speed = 40;
              _satelliteData.randfactor = 1000;
              _satelliteData.rh = 110;
              _satelliteData.gs = 255;
              _satelliteData.bv = 255;
              _satelliteData.rh2 = 215;
              _satelliteData.gs2 = 255;
              _satelliteData.bv2 = 255;
              if (firstRun) {
                for (int i = 0; i < NUM_LEDS; i++) {
                  if (i >= (NUM_LEDS / 2) ) {
                    leds[i] = CHSV(110, 255, random(50, 255));
                  } else {
                    leds[i] = CHSV(215, 255, random(50, 255));
                  }
                  firstRun = 0;
                }
                ledsProject();
              }
              break;
            case 5 + (16 + sizeof(whitesShort)/sizeof(int)): //sunset triangle
              _satelliteData.qcomm = 21;
              _satelliteData.sw1 = 5;
              _satelliteData.sw2 = 0;
              _satelliteData.color = 2000;
              _satelliteData.extra = 11;
              _satelliteData.speed = 15;
              _satelliteData.randfactor = 3000;
              _satelliteData.ext1 = 1;
              if (firstRun) {
                for (int i = 0; i < NUM_LEDS; i++) {
                  if (i > (2 * (NUM_LEDS / 3))) {
                    leds[i] = CHSV(13, 255, random(50, 255));
                  } else if (i > (NUM_LEDS / 3)) {
                    leds[i] = CHSV(165, 255, random(50, 255));
                  } else {
                    leds[i] = CHSV(190, 255, random(50, 255));
                  }
                }
                firstRun = 0;
              }
              ledsProject();
              break;
            case 6 + (16 + sizeof(whitesShort)/sizeof(int)): //tropical triangle (pink, green, cyan)
              _satelliteData.qcomm = 21;
              _satelliteData.sw1 = 5;
              _satelliteData.sw2 = 0;
              _satelliteData.color = 2000;
              _satelliteData.extra = 4;
              _satelliteData.speed = 25;
              _satelliteData.randfactor = 3000;
              _satelliteData.rh = 80;
              _satelliteData.gs = 130;
              _satelliteData.bv = 230;
              _satelliteData.ext1 = 1;
              if (firstRun) {
                for (int i = 0; i < NUM_LEDS; i++) {
                  if (i > 2 * (NUM_LEDS / 3)) {
                    leds[i] = CHSV(80, 255, random(50, 255));
                  } else if (i > (NUM_LEDS / 3)) {
                    leds[i] = CHSV(130, 255, random(50, 255));
                  } else {
                    leds[i] = CHSV(230, 255, random(50, 255));
                  }
                }
                firstRun = 0;
              }
              ledsProject();
              break;
            case 7 + (16 + sizeof(whitesShort)/sizeof(int)): //cherry blossom triangle fade (pink, rose, purple)
              _satelliteData.qcomm = 21;
              _satelliteData.sw1 = 1;
              _satelliteData.sw2 = 0;
              _satelliteData.color = 2000;
              _satelliteData.speed = 30;
              _satelliteData.randfactor = 3000;
              _satelliteData.ext1 = 1;
              _satelliteData.extra = 14;
              _satelliteData.rh = 230;  //pink
              _satelliteData.rh2 = 255;
              _satelliteData.gs = 250;  //rose
              _satelliteData.gs2 = 140;
              _satelliteData.bv = 195;  //purple
              _satelliteData.bv2 = 255;          
              if (firstRun) {
                for (int i = 0; i < NUM_LEDS; i++) {
                  if (i > 2 * (NUM_LEDS / 3)) {
                    leds[i] = CHSV(_satelliteData.rh, _satelliteData.rh2, random(50, 255));
                  } else if (i > (NUM_LEDS / 3)) {
                    leds[i] = CHSV(_satelliteData.gs, _satelliteData.gs2, random(50, 255));
                  } else {
                    leds[i] = CHSV(_satelliteData.bv, _satelliteData.bv2, random(50, 255));
                  }
                }
                firstRun = 0;
              }
              ledsProject();
              break;
            case 8 + (16 + sizeof(whitesShort)/sizeof(int)): //party color jump
              _satelliteData.qcomm = 24;
              _satelliteData.ext4 = 0;
              _satelliteData.sw1 = 2;
              _satelliteData.sw2 = 1; //0 for gradient, 1 for anc points
              _satelliteData.extra = 7;
              _satelliteData.rh = 36;
              _satelliteData.gs = 0;
              _satelliteData.bv = 207;
              _satelliteData.rh2 = 123;
              _satelliteData.speed = 300;
              _satelliteData.randfactor = 0;
              if(millis() - demoPace > 300) {
                int hues[4] = {36,0,207,123};
                fill_solid(leds, NUM_LEDS, CHSV(hues[random(0,4)], 255, 255));
                
              }
              ledsProject();
              break;
            case 9 + (16 + sizeof(whitesShort)/sizeof(int)): //basic palette scroll, LEAP colors
              _satelliteData.qcomm = 40;
              _satelliteData.extra = 4;
              _satelliteData.rh = 90;
              _satelliteData.gs = 160;
              _satelliteData.bv = 195;
              _satelliteData.ext3 = 0;
              _satelliteData.speed = 150;
              _satelliteData.sw2 = 0;
              _satelliteData.sw1 = 2;

              //add demo leds
              if(firstRun) {
                fill_palette(leds, NUM_LEDS, gradientIndex, 10, CRGBPalette16(CHSV(90 ,255,255), CHSV(160 ,255,255), CHSV(195 ,255,255)), 255, LINEARBLEND);
                FastLED.show();
                demoDifference = millis();
                gradientIndex = random8();
                firstRun = 0;
              }
              if(millis() - demoDifference > 10) {
                demoDifference = millis();
                gradientIndex++;
                fill_palette(leds, NUM_LEDS, gradientIndex, 10, CRGBPalette16(CHSV(90 ,255,255), CHSV(160 ,255,255), CHSV(195 ,255,255)), 255, LINEARBLEND);
                FastLED.show();
              }
              ledsProject();
              break;
            case 10 + (16 + sizeof(whitesShort)/sizeof(int)):
              //solid pick cbPal
              _satelliteData.qcomm = 27;
              _satelliteData.sw2 = 1;
              _satelliteData.extra = 12;
              fill_palette(leds, NUM_LEDS, 0, 255/NUM_LEDS, cbPal, 255, NOBLEND);
              ledsProject();
              break;
            case 11 + (16 + sizeof(whitesShort)/sizeof(int)): //shifty sine wave rainbow
              _satelliteData.qcomm = 34;
              _satelliteData.color = 3;
//              _satelliteData.sw1 = 1;
//              _satelliteData.extra = 10;
//              _satelliteData.ext1 = 5;
//              _satelliteData.ext2 = 200;
//              _satelliteData.ext3 = 5;


               //change demo leds
               
              for (int i = 0; i < NUM_LEDS; i++) {
                if (i % 2 == 0) {
                  leds[i] = CHSV((255 / NUM_LEDS) * i, 255, 255);
                } else {
                  leds[i] = CHSV(0, 0, 0);
                }
              }
              ledsProject();
              break;
            case 12 + (16 + sizeof(whitesShort)/sizeof(int)): //random color JUMP with fade
              _satelliteData.qcomm = 30;
              _satelliteData.color = 20;
              _satelliteData.speed = 450;
              _satelliteData.ext1 = 30;
              _satelliteData.randfactor = 65;
              _satelliteData.extra = 0;
              if(firstRun) {
                fill_solid(leds, NUM_LEDS, CHSV(random8(), 255, 255));
                FastLED.show();
                demoDifference = millis();
                firstRun = 0;
              }
              if(millis() - demoDifference > 200) {
                fill_solid(leds, NUM_LEDS, CHSV(random8(), 255, 255));
                FastLED.show();
              }
//              if (firstRun) {
//                colorScroll = CRGBPalette32(CRGB(0, 0, 0), CRGB(255, 120, 25));
//                firstRun = 0;
//              }
//              for (int i = 0; i < NUM_LEDS; i++) {
//                leds[i] = ColorFromPalette(colorScroll, colorIndex[i], 255);
//              }
//              ledsProject();
//              //delay(_satelliteData.speed);
//              for (int i = 0; i < NUM_LEDS; i++) {
//                colorIndex[i]++;
//              }
              break;
            case 13 + (16 + sizeof(whitesShort)/sizeof(int)):
              //straight up rainbow
              _satelliteData.qcomm = 12;
              _satelliteData.color = 256;
              _satelliteData.speed = 15;
              _satelliteData.extra = 1;
              _satelliteData.rh = 1;
              fill_rainbow(leds, NUM_LEDS, ghue, 10);
              ledsProject();
              if (millis() % 10 == 0) {
                ghue++;
              }
              break;
          }
        }
      }
      if(loadColors) {
        turnOn();
        loadColors = 0;
      }
      if(!digitalRead(PUSH)) {
        delay(30);
        difference = millis();
        _brightnessData = _satelliteData;
        while (!digitalRead(PUSH) ) {
          //difference = millis();
          if (millis() - difference > longPressLength) {
            //brightness adjust mode
            brightnessAdjust = 1;
            fill_solid(leds, NUM_LEDS, CRGB::Black);
            FastLED.setBrightness(BRIGHTNESS);
            uint8_t fullLED = (res2+1) / (levelsBrightness / NUM_LEDS);  //number of LEDs that will be on
            float partialLED = (256 / (levelsBrightness / NUM_LEDS)) * ((res2+1) % (levelsBrightness / NUM_LEDS));  //percentage brightness of last LED        
            for(int i = 0; i < fullLED; i++){
              leds[i] = CHSV(160,255,255);
            }
            leds[fullLED] = CHSV(160,255,partialLED);
            ledsProject();
            while (!digitalRead(PUSH)) {}
            delay(30);
            while (brightnessAdjust) {
              prevRes2 = res2;
              res2 += rotary() * spinInversion;
              if (prevRes2 != res2) {
                if (res2 < 0) {
                  res2 = 0;
                }
                if (res2 >= levelsBrightness) {
                  res2 = levelsBrightness - 1;
                }
                fullLED = (res2+1) / (levelsBrightness / NUM_LEDS);  //number of LEDs that will be on
                partialLED = (256 / (levelsBrightness / NUM_LEDS)) * ((res2+1) % (levelsBrightness / NUM_LEDS));  //percentage brightness of last LED        
                for(int i = 0; i < fullLED; i++){
                  leds[i] = CHSV(160,255,255);
                }
                leds[fullLED] = CHSV(160,255,partialLED);
                ledsProject();
                _brightnessData.qcomm = 2;
                _brightnessData.extra = 5;
                uint8_t bKey = ( ((float) res2) / (levelsBrightness - 1) * 255);
                if (bKey > 255) {
                  bKey = 255;
                } else if (bKey < 1) {
                  bKey = 0;
                }
                _brightnessData.color = bKey;
                _brightnessData.speed = 1;
                _base.send(DESTINATION_RADIO_ID, &_brightnessData, sizeof(_brightnessData), NRFLite::NO_ACK);
              }
              if (!digitalRead(PUSH)) {
                brightnessAdjust = 0;
              }
            }
            if (!LEDstripOn) {
              FastLED.setBrightness(0);
              ledsProject();
            }
            turnOff();
            if(res2 > 0) {
//              if(res >= 32 + sizeof(whites)/sizeof(int) && lastSentRes >= 32 + sizeof(whites)/sizeof(int)) {
//                sendPacket();
//              }
              _base.send(DESTINATION_RADIO_ID, &_lastPacket, sizeof(_lastPacket), NRFLite::NO_ACK);
              lightStatus = 1;
            } else {
              lightStatus = 0;
            }
            firstRun = 1;
            timeOut = millis();
            skipTheAdjust = 1;
            while (!digitalRead(PUSH)) {}
            delay(30);
          }
        }
        if (!skipTheAdjust) {
          if(LEDstripOn) {
            //routine palette send
            timeOut = millis();
            if(lightStatus == 0) {
              res2 = levelsBrightness - 1;
              prevRes2 = res2;
            }
            lightStatus = 1;
            sendPacket();
          } else {
            if (lightStatus) {
              //lights are on, LED strip off: turn off lights
              lightStatus = 0;
              _satelliteData.qcomm = 0;
              _satelliteData.speed = 0;
              _satelliteData.extra = 0;
              res2 = 0;
              prevRes2 = res2;
              sendPacket();
            } else {
              //lights are off, LED strip is off, send default white
              lightStatus = 1;
              _satelliteData.qcomm = 10;
              _satelliteData.speed = 0;
              _satelliteData.extra = 5;
              res2 = levelsBrightness - 1;
              prevRes2 = res2;
              sendPacket();
            }
          }
        }
        //timeOut = millis();
        skipTheAdjust = 0;
        delay(20);
      }
      break;
     
  }
}
