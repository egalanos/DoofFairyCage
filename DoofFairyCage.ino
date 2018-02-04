/*
 * ~= Doof fairy cage =~
 *
 * WS2811 pixel things for egg for Psyfari 2018
 * 
 */
#include <FastLED.h>
#include <elapsedMillis.h>


FASTLED_USING_NAMESPACE

#define VERSION "1"

// Pin definitions
#define DATA_PIN 6

#define NUM_LEDS 100
CRGB leds[NUM_LEDS];


void setup() {
  Serial.begin(9600);
  Serial.println("Doof Fairy Cage version#" VERSION " setup()...");
  
  FastLED.addLeds<WS2812, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setCorrection( CRGB(255, 140, 240) );

  random16_set_seed(analogRead(0));
  randomSeed(analogRead(0));
  
  Serial.println("Setup complete!");
}


#define LOOP_FREQUENCY_HZ 50
// #define DEBUG 1


// Our colour themes
enum colour_themes { Electric, Warm };
byte colour_theme;

const byte electric_hues[] = { HUE_BLUE, HUE_AQUA, HUE_PURPLE, HUE_PURPLE, HUE_BLUE, HUE_AQUA, HUE_PURPLE, HUE_PINK };
const byte warm_hues[] = { HUE_RED, HUE_ORANGE, HUE_ORANGE };


// All of our pretty patterns!
struct {
  void (*setup)(byte);
  void (*loop)(void);
  byte theme;
  byte data;
} patterns[] = {
  { fairy_cage_setup, fairy_cage_loop, Warm, 0 },
  { fairy_cage_setup, fairy_cage_loop, Electric, 0 },
  { NULL, NULL, 0, 0 } // End of list
};
byte pattern_index = 0;
byte prev_pattern_index = 255;








#ifdef DEBUG
unsigned long loop_printout_time = 0;
unsigned long loop_count = 0;
#endif

void loop() {
  
  unsigned long loop_start_time = millis();
  
  if (pattern_index != prev_pattern_index) {
    colour_theme = patterns[pattern_index].theme;
    patterns[pattern_index].setup(patterns[pattern_index].data);
    FastLED.show();
    prev_pattern_index = pattern_index;
  }
  patterns[pattern_index].loop();
  FastLED.show();
  
    
#ifdef DEBUG
  loop_count++;
  unsigned long ms_since_last_printout = millis() - loop_printout_time;
  if (ms_since_last_printout > 1000) {
    Serial.print("Loop count: ");
    Serial.print(loop_count*1000/ms_since_last_printout);
    Serial.println(" loops/second.");
    loop_count = 0;
    loop_printout_time = millis();
  }
#endif
  
  unsigned loop_duration = millis() - loop_start_time;
  if (loop_duration < (1000/LOOP_FREQUENCY_HZ))
    delay((1000/LOOP_FREQUENCY_HZ) - loop_duration);
}



// Common hue management code

byte colour_theme_hue_random(void) {
  
  byte hue;
  
  switch (colour_theme) {
    
    case Electric:
      hue = electric_hues[random8(sizeof(electric_hues))];
      break;
      
    case Warm:
      hue = warm_hues[random8(sizeof(warm_hues))];
      break;
      
    default:
      hue = HUE_PURPLE;
  }
 
 return hue;
} 





/*
 *  flicker effect
 */
#define FAIRY_HUE_WOBBLE 30
#define FAIRY_SATURATION_WOBBLE 64
#define FAIRY_WAKING_BRIGHTNESS 15
#define FAIRY_DORMANT_TIME_MS (10*1000)
#define FAIRY_WAKE_TIME_MS    (4*1000)
#define FAIRY_TWINKLE_TIME_MS (9*1000)
#define FAIRY_FADE_TIME_MS    (2*1000)
#define FAIRY_DENSITY_PERCENTAGE 50

unsigned int fairy_awake_count;
unsigned int fairy_target_count;

enum fairy_state { Waking, Twinkle, Fading, Dormant };

struct {
  byte state;
  uint8_t hue;
  uint8_t brightness;
  elapsedMillis time_elapsed;
} fairies[NUM_LEDS];


void fairy_cage_setup(byte data) {

  fill_solid(leds, NUM_LEDS, CRGB::Black);
  
  fairy_awake_count = 0;
  fairy_target_count = (FAIRY_DENSITY_PERCENTAGE * NUM_LEDS) / 100;

  // All the beauties start asleep
  for (unsigned int i = 0; i < NUM_LEDS; i++) {
    fairies[i].state = Dormant;
    fairies[i].time_elapsed = 0;
    fairies[i].hue = colour_theme_hue_random();
    fairies[i].brightness = 0;
  }

  // Wake up a percentage of the fairies
  // XXX See how the loop code works first!
}


// Matches 12 breaths a minute.
//    cycle time 255 cycles @ 50 cycles a second = 5 seconds per complete fairy_cage_loop animation = 12 animations/minute.
void fairy_cage_loop() {
  
  static byte cycle = 0;

   
  for (unsigned int i = 0; i < NUM_LEDS; i++) {
    
    switch (fairies[i].state) {

      case Dormant:
        if (fairy_awake_count < fairy_target_count) {
          
          if (fairies[i].time_elapsed > FAIRY_DORMANT_TIME_MS) {

            if ( random8(100) <= FAIRY_DENSITY_PERCENTAGE ) { // XXX this may be suspect... hrmmm
              fairies[i].state = Waking;
              fairies[i].time_elapsed = 0;
              fairies[i].hue = colour_theme_hue_random();
              fairy_awake_count++;
            }
          }
        }
        break;
      
      case Waking:
      
        fairies[i].brightness = lerp8by8(FAIRY_WAKING_BRIGHTNESS, 255, 255 * fairies[i].time_elapsed / FAIRY_WAKE_TIME_MS);
        
        if (fairies[i].time_elapsed > FAIRY_WAKE_TIME_MS) {
          fairies[i].state = Twinkle;
          fairies[i].time_elapsed = 0;
          fairies[i].brightness = 255;
        }
        fairy_twinkle(i, cycle);
        break;
        
      case Twinkle:
        if (fairies[i].time_elapsed > FAIRY_TWINKLE_TIME_MS) {
          fairies[i].state = Fading;
          fairies[i].time_elapsed = 0;
        }
        fairy_twinkle(i, cycle);
        break;
        
      case Fading:

        fairies[i].brightness = lerp8by8(255, FAIRY_WAKING_BRIGHTNESS, 255 * fairies[i].time_elapsed / FAIRY_WAKE_TIME_MS);

        if (fairies[i].time_elapsed > FAIRY_FADE_TIME_MS) {
          fairies[i].state = Dormant;
          fairies[i].time_elapsed = 0;
          fairies[i].brightness = 0;
          fairy_awake_count--;
        }
         
        fairy_twinkle(i, cycle);
        break;
    }
  }

  cycle++;  
}

void fairy_twinkle(byte index, byte cycle) {
  
  byte brightness;
  
  if (fairies[index].brightness) {
    brightness = FAIRY_WAKING_BRIGHTNESS + scale8(triwave8(cycle + 19 * index), fairies[index].brightness - FAIRY_WAKING_BRIGHTNESS);
  } else {
    brightness = 0;
  }

  byte perturb = triwave8(cycle);
  leds[index] = CHSV(fairies[index].hue + scale8(perturb, FAIRY_HUE_WOBBLE),
                   255 - scale8(perturb, FAIRY_SATURATION_WOBBLE),
                   brightness); 
}
