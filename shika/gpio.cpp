#include <Arduino.h>
#include "gpio.h"

#define BUTTON_DEBOUNCE_COUNT 3

#define BUTTON_STATE_INIT 0
#define BUTTON_STATE_DOWN 1
#define BUTTON_STATE_RELEASED 2
#define BUTTON_STATE_DOWN2 3
#define BUTTON_STATE_RELEASED2 4
#define BUTTON_STATE_LONG_COMPLETE 5

#define VIBE_PIN D7
#define TAP_PIN D9

void gpio_init(void) {
  pinMode(VIBE_PIN, OUTPUT);
  digitalWrite(VIBE_PIN, LOW);
  pinMode(TAP_PIN, INPUT_PULLDOWN);
  pinMode(LED_BUILTIN, OUTPUT);
}

static int queued_vibe_duration = 0;
static int queued_vibe_pulses = 0;

void gpio_vibe_request(int pulses, int duration) {
  queued_vibe_duration = duration;
  queued_vibe_pulses = pulses;
}


static inline void vibe_update(void) {
  static uint32_t vibe_time_duration = 0;
  static uint32_t vibe_time_event = 0;
  static bool vibe = false;

  if (queued_vibe_pulses > 0) {
    if (millis() - vibe_time_event > vibe_time_duration) {
      if (vibe == false) {
        //start vibing
        vibe = true;
        digitalWrite(VIBE_PIN, HIGH);
        vibe_time_event = millis();
        vibe_time_duration = queued_vibe_duration;

      } else {
        //stop vibing
        queued_vibe_pulses--;
        vibe = false;
        digitalWrite(VIBE_PIN, LOW);
        vibe_time_event = millis();
        vibe_time_duration = GPIO_VIBE_COOLDOWN;
      }
    }
  }
}


static inline int button_update(void) {

  int retval = GPIO_EVENT_NONE;

  static uint32_t button_pressed_time = 0;
  static uint32_t button_released_time = 0;

  bool button_state = digitalRead(TAP_PIN);

  static int button_pressed_counter = 0;
  static int button_released_counter = 0;

  static bool debounced_button = false;
  static bool debounced_button_previous = false;

  if (!button_state && button_released_counter < BUTTON_DEBOUNCE_COUNT) {
    button_pressed_counter = 0;
    button_released_counter++;

    if (button_released_counter == BUTTON_DEBOUNCE_COUNT) debounced_button = false;
  }

  if (button_state && button_pressed_counter < BUTTON_DEBOUNCE_COUNT) {
    button_released_counter = 0;
    button_pressed_counter++;

    if (button_pressed_counter == BUTTON_DEBOUNCE_COUNT) debounced_button = true;
  }

  static int button_debounced_state = BUTTON_STATE_INIT;
  //Serial.println(button_debounced_state);

  if (debounced_button && !debounced_button_previous) {
    button_pressed_time = millis();
    if (button_debounced_state == BUTTON_STATE_INIT)
      button_debounced_state = BUTTON_STATE_DOWN;

    if (button_debounced_state == BUTTON_STATE_RELEASED)
      button_debounced_state = BUTTON_STATE_DOWN2;
  }


  if (!debounced_button && debounced_button_previous) {
    button_released_time = millis();
    if (button_debounced_state == BUTTON_STATE_DOWN)
      button_debounced_state = BUTTON_STATE_RELEASED;
    if (button_debounced_state == BUTTON_STATE_DOWN2) {
      retval = GPIO_EVENT_PARTY;
      button_debounced_state = BUTTON_STATE_INIT;
    }
    if (button_debounced_state == BUTTON_STATE_LONG_COMPLETE)
      button_debounced_state = BUTTON_STATE_INIT;
  }


  if (debounced_button) {
    if (millis() - button_pressed_time > 1000 && button_debounced_state != BUTTON_STATE_LONG_COMPLETE) {  //how long for a hold
      retval = GPIO_EVENT_ALARM;
      button_debounced_state = BUTTON_STATE_LONG_COMPLETE;
    }
  } else {
    if (millis() - button_released_time > 500) {  //how long between presses
      button_debounced_state = BUTTON_STATE_INIT;
    }
  }

  debounced_button_previous = debounced_button;
  return retval;
}

int gpio_update(int connections) {

  if (connections > 1)
    digitalWrite(LED_BUILTIN, 0x01 & (millis() >> 6));
  else
    digitalWrite(LED_BUILTIN, LOW);

  vibe_update();
  return button_update();
}