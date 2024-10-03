#ifndef _LEDS_H
#define _LEDS_H

void leds_init(void);
bool leds_update(int global_mode);
void leds_effect_change_offset(int offset);
int leds_get_effect_offset(void);
void leds_set_intro(void);
void led_reset(void);
#endif