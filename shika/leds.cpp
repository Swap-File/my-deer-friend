#include "shika.h"

#define TIME_PER_ALARM 5000    //5 seconds
#define TIME_PER_EFFECT 10000  //10 seconds

#define BRIGHTNESS 255
#define NUM_LED_EFFECTS 6

#define NUM_LEDS 32
#define DATA_PIN_LEFT D8
#define DATA_PIN_RIGHT D1

static CRGB leds[NUM_LEDS];
static CRGB leds_left[NUM_LEDS / 2];
static CRGB leds_right[NUM_LEDS / 2];

static uint8_t gHue = 0;
static uint32_t led_time_offset = 0;
static int led_effect_offset = 0;
static bool intro = false;


int leds_get_effect_offset() {
  return led_effect_offset;
}

void leds_set_intro() {
  intro = true;
}

static inline void sinelon() {
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy(leds, NUM_LEDS, 20);
  int pos = beatsin16(13, 0, NUM_LEDS - 1);
  leds[pos] += CHSV(gHue, 255, 192);
}

static inline void sinelon2() {
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy(leds, NUM_LEDS, 20);
  int pos = beatsin16(13, 0, NUM_LEDS - 1);
  leds[pos] += CHSV(gHue, 255, 192);

  pos = beatsin16(13, 0, NUM_LEDS - 1, 0, UINT16_MAX / 2);
  leds[pos] += CHSV(gHue + 127, 255, 192);
}

static inline void sinelon3() {
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy(leds, NUM_LEDS, 20);
  int pos = beatsin16(13, 0, NUM_LEDS - 1);
  leds[pos] += CHSV(gHue, 255, 192);

  pos = beatsin16(13, 0, NUM_LEDS - 1, 0, UINT16_MAX / 8);
  leds[pos] += CHSV(gHue + 64, 255, 192);
}


static inline void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy(leds, NUM_LEDS, 40);
  uint8_t dothue = 0;
  for (int i = 0; i < 5; i++) {
    leds[beatsin16(i + 7, 0, NUM_LEDS - 1)] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

void leds_effect_change_offset(int offset) {
  led_time_offset = TIME_PER_EFFECT - (get_millisecond_timer() % TIME_PER_EFFECT);
  led_effect_offset = offset;
  led_effect_offset = led_effect_offset % NUM_LED_EFFECTS;
}


bool leds_update(int global_mode) {
  bool animation_done = false;

  static CRGB overlay_color(0, 0, 0);


  uint32_t this_tick = get_millisecond_timer();  //will this be an overflow issue?
  gHue = this_tick / 10;

  int mode = (((this_tick + led_time_offset) / TIME_PER_EFFECT) + led_effect_offset) % NUM_LED_EFFECTS;

  if (global_mode == GLOBAL_MODE_PARTY) {
    if (intro) {
      overlay_color = CRGB(255, 255, 255);
      intro = false;
    }

    if (mode == 0)
      fill_rainbow(leds, NUM_LEDS, gHue, 7);
    else if (mode == 1)
      juggle();
    else if (mode == 2)
      sinelon();
    else if (mode == 3)
      sinelon2();
    else if (mode == 4)
      sinelon3();
    else if (mode == 5)
      fill_solid(leds, NUM_LEDS, CHSV(gHue, 255, 255));
  } else if (global_mode == GLOBAL_MODE_ALARM) {
    static uint32_t alarm_start_time = 0;
    if (intro) {
      alarm_start_time = millis();
      intro = false;
    }

    const int zero_offset = 16;
    int pos = beatsin16(90, 0, 255 + zero_offset, alarm_start_time);
    pos -= zero_offset;
    if (pos < 0) pos = 0;
    fill_solid(leds, NUM_LEDS, CRGB(pos, 0, 0));

    if (pos == 0 && millis() - alarm_start_time > TIME_PER_ALARM)
      animation_done = true;
  } else {
    fill_solid(leds, NUM_LEDS, CRGB(0, 0, 0));
  }

  for (int i = 0; i < 16; i++) {
    leds_right[15 - i] = leds[i] + overlay_color;
    leds_left[15 - i] = leds[NUM_LEDS - 1 - i] + overlay_color;
  }

  overlay_color.fadeToBlackBy(20);

  FastLED.show();
  return animation_done;
}

void leds_init(void) {
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.addLeds<WS2812B, DATA_PIN_LEFT, GRB>(leds_left, NUM_LEDS / 2);
  FastLED.addLeds<WS2812B, DATA_PIN_RIGHT, GRB>(leds_right, NUM_LEDS / 2);
}