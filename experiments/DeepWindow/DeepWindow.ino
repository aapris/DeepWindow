/*
   References:
   Palette array:
   https://gist.github.com/kriegsman/8281905786e8b2632aeb
*/

#include <FastLED.h>

#define NUM_LEDS            90
#define NUM_STRIPS           3
#define NUM_LEDS_PER_STRIP  30
#define BRIGHTNESS         255

#define LED_TYPE     WS2812B
#define LED_PIN      D5  // 13    // D7

// #define LED_TYPE    LPD8806
// #define LED_PIN      D7  // 13    // D7
// #define CLK_PIN      D5  // 14    // D5

#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

#define UPDATES_PER_SECOND 100

uint8_t SECONDS_PER_PALETTE = 5;
uint8_t MILLISECONDS_PER_BLEND = 200;

// PIR variables
const byte interruptPin = D3; // 2; // D4
volatile byte interruptCounter = 0;
int numberOfInterrupts = 0;
long lastInterruptTime = 0;
long lastMotionTime = 0;

TBlendType    currentBlending;

// Forward declarations of an array of cpt-city gradient palettes, and
// a count of how many there are.  The actual color palette definitions
// are at the bottom of this file.
extern const TProgmemRGBGradientPalettePtr gGradientPalettes[];
extern const uint8_t gGradientPaletteCount;

// Current palette number from the 'playlist' of color palettes
uint8_t gCurrentPaletteNumber = 0;

CRGBPalette16 gCurrentPalette( CRGB::Black);
CRGBPalette16 gTargetPalette( gGradientPalettes[0] );

DEFINE_GRADIENT_PALETTE( black_gp ) {
  0,  0,  0,  0,  // black
  255,  0,  0,  0  // black
};

DEFINE_GRADIENT_PALETTE( dawn_gp010 ) {
  0,  0, 0, 0,
  85,  0, 0, 10,
  170, 0, 10, 20,
  210,  250,  50, 5,
  255,  250,  10, 5
};

DEFINE_GRADIENT_PALETTE( white_gp ) {
  0,  255, 255, 255,
  255,  255,  255, 255
};

DEFINE_GRADIENT_PALETTE( dawn_gp012 ) {
  0,  255, 0, 0,
  85,  0, 0, 255,
  170, 0, 255, 20,
  210,  255,  5, 5,
  255,  250,  10, 5
};

DEFINE_GRADIENT_PALETTE( dawn_gp015 ) {
  0,  0, 0, 10,
  85,  0, 5, 20,
  170, 0, 20, 40,
  255,  40,  5, 1
};

DEFINE_GRADIENT_PALETTE( dawn_gp020 ) {
  0,  0, 10, 30,
  85,  0, 20, 80,
  170, 0, 40, 80,
  255,  50,  30, 15
};

DEFINE_GRADIENT_PALETTE( dawn_gp030 ) {
  0,  40, 80, 130,
  85,  60, 120, 180,
  170, 100, 200, 250,
  255, 150, 150, 50
};

const TProgmemRGBGradientPalettePtr gGradientPalettes[] = {
  dawn_gp010,
  //  dawn_gp012,
  dawn_gp015,
  dawn_gp020,
  dawn_gp030,
  black_gp,
  black_gp
};

// Count of how many cpt-city gradients are defined:
const uint8_t gGradientPaletteCount =
  sizeof( gGradientPalettes) / sizeof( TProgmemRGBGradientPalettePtr );


void handleInterrupt() {
  interruptCounter++;
  lastMotionTime = millis();
  // state = digitalRead(interruptPin);
}


void setup() {
  Serial.begin( 115200 );
  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, RISING);
  delay( 1000 ); // power-up safety delay
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  // FastLED.addLeds<LED_TYPE, LED_PIN, CLK_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );
  currentBlending = LINEARBLEND;
}


void loop()
{
  if (interruptCounter > 0) {

    interruptCounter--;
    numberOfInterrupts++;
    Serial.print(lastMotionTime);
    Serial.print(" at ");
    Serial.print(millis());
    Serial.print(" ms uptime. Total: ");
    Serial.println(numberOfInterrupts);

  }

  uint8_t maxChanges = 48;
  EVERY_N_SECONDS( SECONDS_PER_PALETTE ) {
    gCurrentPaletteNumber = addmod8( gCurrentPaletteNumber, 1, gGradientPaletteCount);
    gTargetPalette = gGradientPalettes[ gCurrentPaletteNumber ];
    Serial.println(gCurrentPaletteNumber);
  }

  EVERY_N_MILLISECONDS( MILLISECONDS_PER_BLEND ) {
    nblendPaletteTowardPalette( gCurrentPalette, gTargetPalette, maxChanges);
  }

  static uint8_t startIndex = 0;
  long age = millis() - lastMotionTime;
  if (age < 5000) {
    //Serial.println(age);
    Fire2012();
  } else if (age >= 5000 && age < 10000) {
    for ( int i = 0; i < NUM_LEDS; i++) {
      leds[i].fadeToBlackBy( 2 );
    }
  } else {
    FillLEDsFromPaletteColors( startIndex);
    //Serial.println(age);
  }
  FastLED.show();
  FastLED.delay(1000 / UPDATES_PER_SECOND);
}

void FillLEDsFromPaletteColors( uint8_t colorIndex)
{
  uint8_t brightness = 255;

  for ( int i = 0; i < NUM_LEDS_PER_STRIP; i++) {
    //colorIndex = map(NUM_LEDS);
    float colorIndex = map(i, 0, NUM_LEDS_PER_STRIP, 0, 255);
    leds[i] = ColorFromPalette( gCurrentPalette, colorIndex, brightness, currentBlending);
    leds[60 - i] = ColorFromPalette( gCurrentPalette, colorIndex, brightness, currentBlending);
    leds[60 + i] = ColorFromPalette( gCurrentPalette, colorIndex, brightness, currentBlending);
  }
}


// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 50, suggested range 20-100
#define COOLING  55

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 120

void Fire2012()
{
  // Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS_PER_STRIP];
  static byte heat2[NUM_LEDS_PER_STRIP];
  static byte heat3[NUM_LEDS_PER_STRIP];

  // Step 1.  Cool down every cell a little
  for ( int i = 0; i < NUM_LEDS_PER_STRIP; i++) {
    heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS_PER_STRIP) + 2));
    heat2[i] = qsub8( heat2[i],  random8(0, ((COOLING * 10) / NUM_LEDS_PER_STRIP) + 2));
    heat3[i] = qsub8( heat3[i],  random8(0, ((COOLING * 10) / NUM_LEDS_PER_STRIP) + 2));
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for ( int k = NUM_LEDS_PER_STRIP - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    heat2[k] = (heat2[k - 1] + heat2[k - 2] + heat2[k - 2] ) / 3;
    heat3[k] = (heat3[k - 1] + heat3[k - 2] + heat3[k - 2] ) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if ( random8() < SPARKING ) {
    int y = random8(7);
    int y2 = random8(7);
    int y3 = random8(7);
    heat[y] = qadd8( heat[y], random8(160, 255) );
    heat2[y] = qadd8( heat2[y2], random8(160, 255) );
    heat3[y] = qadd8( heat3[y3], random8(160, 255) );
  }

  // Step 4.  Map from heat cells to LED colors
  for ( int j = 0; j < NUM_LEDS_PER_STRIP; j++) {
    CRGB color = HeatColor( heat[j]);
    CRGB color2 = HeatColor( heat2[j]);
    CRGB color3 = HeatColor( heat3[j]);
    int pixelnumber;
    pixelnumber = j;
    leds[pixelnumber] = color;
    leds[2 * NUM_LEDS_PER_STRIP - pixelnumber] = color2;
    leds[2 * NUM_LEDS_PER_STRIP + pixelnumber] = color3;
  }
}

