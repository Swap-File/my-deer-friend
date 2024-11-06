#include "shika.h"

#define TIME_PER_ALARM 5000   // 5 seconds
#define TIME_PER_EFFECT 10000 // 10 seconds

#define BRIGHTNESS 255
#define NUM_LED_EFFECTS 6

#define BLEND_SPEED 5

#define NUM_LEDS 32
#define DATA_PIN_LEFT D8
#define DATA_PIN_RIGHT D1

static CRGB leds5[NUM_LEDS];
static CRGB leds4[NUM_LEDS];
static CRGB leds3[NUM_LEDS];
static CRGB leds2[NUM_LEDS];
static CRGB leds1[NUM_LEDS];
static CRGB leds0[NUM_LEDS];
static CRGB leds[NUM_LEDS];
static CRGB leds_left[NUM_LEDS / 2];
static CRGB leds_right[NUM_LEDS / 2];

static uint8_t gHue = 0;
static uint32_t led_time_offset = 0;
static int led_effect_offset = 0;
static bool intro = false;

int leds_get_effect_offset()
{
  return led_effect_offset;
}

void leds_set_intro()
{
  intro = true;
}

static int node_buck_mode = 0;
static int node_place = 0;
static int node_count = 0;
static int buck_hue = 0;
static uint32_t buck_speed = 1000;
static int buck_rainbow = 0;

void leds_set_buck(int buck_mode_, int node_place_, int node_count_, int h_, uint32_t node_speed_)
{
  node_buck_mode = buck_mode_;
  node_place = node_place_;
  node_count = node_count_;
  buck_speed = node_speed_;
  if (h_ < 0)
  {
    buck_rainbow = 1;
    h_ += 1;
    h_ = abs(h_);
  }
  else
    buck_rainbow = 0;

  buck_hue = constrain(h_, 0, 255);
}

static inline void sinelon(CRGB *array)
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy(array, NUM_LEDS, 20);
  int pos = beatsin16(13, 0, NUM_LEDS - 1);
  array[pos] += CHSV(gHue, 255, 192);
}

static inline void sinelon2(CRGB *array)
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy(array, NUM_LEDS, 20);
  int pos = beatsin16(13, 0, NUM_LEDS - 1);
  array[pos] += CHSV(gHue, 255, 192);

  pos = beatsin16(13, 0, NUM_LEDS - 1, 0, UINT16_MAX / 2);
  array[pos] += CHSV(gHue + 127, 255, 192);
}

static inline void sinelon3(CRGB *array)
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy(array, NUM_LEDS, 20);
  int pos = beatsin16(13, 0, NUM_LEDS - 1);
  array[pos] += CHSV(gHue, 255, 192);

  pos = beatsin16(13, 0, NUM_LEDS - 1, 0, UINT16_MAX / 8);
  array[pos] += CHSV(gHue + 64, 255, 192);
}

static inline void juggle(CRGB *array)
{
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy(array, NUM_LEDS, 40);
  uint8_t dothue = 0;
  for (int i = 0; i < 5; i++)
  {
    array[beatsin16(i + 7, 0, NUM_LEDS - 1)] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

void leds_effect_change_offset(int offset)
{
  led_time_offset = TIME_PER_EFFECT - (get_millisecond_timer() % TIME_PER_EFFECT);
  led_effect_offset = offset;
  led_effect_offset = led_effect_offset % NUM_LED_EFFECTS;
}

void led_reset()
{
  led_effect_offset = 0;

  led_time_offset = 0;
}

bool leds_update(int global_mode)
{
  bool animation_done = false;

  static CRGB overlay_color(0, 0, 0);

  uint32_t this_tick = get_millisecond_timer();
  gHue = this_tick / 10;

  int mode = (((this_tick + led_time_offset) / TIME_PER_EFFECT) + led_effect_offset) % NUM_LED_EFFECTS;
  static uint8_t mix_array[NUM_LED_EFFECTS];

  if (intro)
  {
    // set overlay to a flash effect and then immediately change to the chosen effect
    overlay_color = CRGB(255, 255, 255);
    intro = false;
    for (int i = 0; i < NUM_LED_EFFECTS; i++)
    {
      if (mode == i)
        mix_array[mode] = 0;
      else
        mix_array[i] = 255;
    }
  }

  if (global_mode == GLOBAL_MODE_PARTY)
  {
    // calculate all effects
    fill_rainbow(leds0, NUM_LEDS, gHue, 7);
    juggle(leds1);
    sinelon(leds2);
    sinelon2(leds3);
    sinelon3(leds4);
    fill_solid(leds5, NUM_LEDS, CHSV(gHue, 255, 255));

    // adjust the mixer
    for (int i = 0; i < NUM_LED_EFFECTS; i++)
    {
      if (mode == i)
        mix_array[mode] = qsub8(mix_array[mode], BLEND_SPEED);
      else
        mix_array[i] = qadd8(mix_array[i], BLEND_SPEED);
    }

    // combine all effects
    for (int i = 0; i < NUM_LEDS; i++)
    {
      leds[i] = CRGB(0, 0, 0);
      CRGB temp;
      temp = leds0[i];
      leds[i] += temp.fadeToBlackBy(mix_array[0]);
      temp = leds1[i];
      leds[i] += temp.fadeToBlackBy(mix_array[1]);
      temp = leds2[i];
      leds[i] += temp.fadeToBlackBy(mix_array[2]);
      temp = leds3[i];
      leds[i] += temp.fadeToBlackBy(mix_array[3]);
      temp = leds4[i];
      leds[i] += temp.fadeToBlackBy(mix_array[4]);
      temp = leds5[i];
      leds[i] += temp.fadeToBlackBy(mix_array[5]);
    }
  }
  else if (global_mode == GLOBAL_MODE_ALARM)
  {
    static uint32_t alarm_start_time = 0;
    static uint32_t alarm_fx_timebase = 0;

    if (intro)
    {
      alarm_start_time = millis();                 // change this to use the transmitted time?
      alarm_fx_timebase = get_millisecond_timer(); // if we do this, on alarm extension we need to not update values
      intro = false;
    }

    const int zero_offset = 16;
    int pos = beatsin16(90, 0, 255 + zero_offset, alarm_fx_timebase, 3 * UINT16_MAX / 4);

    pos -= zero_offset;
    if (pos < 0)
      pos = 0;
    fill_solid(leds, NUM_LEDS, CRGB(pos, 0, 0));

    if (pos == 0 && millis() - alarm_start_time > TIME_PER_ALARM)
      animation_done = true;
  }
  else if (global_mode == GLOBAL_MODE_BUCK)
  {
    int active_node = (((this_tick) / buck_speed) % node_count) + 1;

    if (node_buck_mode == 0) // debug mode, call out place in the herd
    {
      fill_solid(leds, NUM_LEDS, CRGB(0, 0, 0));
      leds[node_place - 1] = CHSV(buck_hue, 255, 255);
      leds[node_count] = CHSV(buck_hue + (gHue * buck_rainbow), 255, 255);
    }
    if (node_buck_mode == 1) // solid color jumping from head to head
    {
      fill_solid(leds, NUM_LEDS, CRGB(0, 0, 0));
      if (active_node == node_place)
      {
        fill_solid(leds, NUM_LEDS, CHSV(buck_hue + (gHue * buck_rainbow), 255, 255));
      }
    }
    if (node_buck_mode == 2) // animated fill jumping from head to head
    {
      fill_solid(leds, NUM_LEDS, CRGB(0, 0, 0));
      static int loc = 0;
      if (active_node == node_place)
      {
        for (int i = 0; i < 16; i++)
        {
          if (i < loc)
          {
            leds[16 + i] = CHSV(buck_hue + (gHue * buck_rainbow), 255, 255);
            leds[15 - i] = CHSV(buck_hue + (gHue * buck_rainbow), 255, 255);
          }
        }
        loc = qadd8(loc, 1);
      }
      else
        loc = 0;
    }
    else if (node_buck_mode == 3) // animated dot jumping from head to head
    {
      fill_solid(leds, NUM_LEDS, CRGB(0, 0, 0));
      static int loc = 0;
      if (active_node == node_place)
      {
        for (int i = 0; i < 16; i++)
        {
          if (i == loc)
          {
            leds[16 + i] = CHSV(buck_hue + (gHue * buck_rainbow), 255, 255);
            leds[15 - i] = CHSV(buck_hue + (gHue * buck_rainbow), 255, 255);
          }
        }
        loc = qadd8(loc, 1);
      }
      else
        loc = 0;
    }
    else if (node_buck_mode == 4) // animated fill / flush jumping from head to head
    {
      fill_solid(leds, NUM_LEDS, CRGB(0, 0, 0));
      static int loc = 0;
      if (active_node == node_place)
      {
        for (int i = 0; i < 16; i++)
        {
          if (loc < 16)
          {
            if (i < loc)
            {
              leds[16 + i] = CHSV(buck_hue + (gHue * buck_rainbow), 255, 255);
              leds[15 - i] = CHSV(buck_hue + (gHue * buck_rainbow), 255, 255);
            }
          }
          else
          {

            int temp_i = (loc - 16);
            if (i >= temp_i)
            {
              leds[16 + i] = CHSV(buck_hue + (gHue * buck_rainbow), 255, 255);
              leds[15 - i] = CHSV(buck_hue + (gHue * buck_rainbow), 255, 255);
            }
          }
        }
        loc = qadd8(loc, 1);
      }
      else
        loc = 0;
    }
    else if (node_buck_mode == 5) // solid color everywhere
    {
      fill_solid(leds, NUM_LEDS, CHSV(buck_hue + (gHue * buck_rainbow), 255, 255));
    }
  }
  else
  {
    fill_solid(leds, NUM_LEDS, CRGB(0, 0, 0));
  }

  for (int i = 0; i < 16; i++)
  {
    leds_right[15 - i] = leds[i] + overlay_color;
    leds_left[15 - i] = leds[NUM_LEDS - 1 - i] + overlay_color;
  }

  overlay_color.fadeToBlackBy(20);

  FastLED.show();
  return animation_done;
}

void leds_init(void)
{
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.addLeds<WS2812B, DATA_PIN_LEFT, GRB>(leds_left, NUM_LEDS / 2);
  FastLED.addLeds<WS2812B, DATA_PIN_RIGHT, GRB>(leds_right, NUM_LEDS / 2);
}