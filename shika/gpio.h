#ifndef _GPIO_H
#define _GPIO_H

void gpio_init(void);
int gpio_update(int connections);
void gpio_vibe_request(int pulses, int duration);

#define GPIO_EVENT_NONE 0
#define GPIO_EVENT_ALARM 1
#define GPIO_EVENT_PARTY 2

#define GPIO_VIBE_LONG 1000
#define GPIO_VIBE_SHORT 300
#define GPIO_VIBE_COOLDOWN 200

#endif