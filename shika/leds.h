#ifndef _LEDS_H
#define _LEDS_H

#include <Arduino.h>

void leds_init(void);
bool leds_update(int global_mode);
void leds_set_buck(int buck_mode_, int node_place_, int node_count_, int h_, uint32_t node_speed_);
void leds_effect_change_offset(int offset);
int leds_get_effect_offset(void);
void leds_set_intro(void);
void led_reset(void);
void leds_palette(int p);

#endif