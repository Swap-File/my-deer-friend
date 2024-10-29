#include <Arduino.h>
#include "render.h"

float battery_voltage = 0;

#define BUTTON1_PIN 35
#define BUTTON2_PIN 0
#define BATTERY_PIN 34
#define ADC_EN 14 // ADC_EN is the ADC detection enable port

static float get_voltage(void)
{
    return (((float)(analogRead(BATTERY_PIN)) / 4095.0) * 2.0 * 3.3 * (1100 / 1000.0));
}

void ui_init(void)
{
    pinMode(BUTTON1_PIN, INPUT);
    pinMode(BUTTON2_PIN, INPUT_PULLUP);
    pinMode(BATTERY_PIN, INPUT);
    pinMode(ADC_EN, OUTPUT);
    digitalWrite(ADC_EN, HIGH);
    render_init();
    battery_voltage = get_voltage();
}

void ui_update(void)
{
    battery_voltage = get_voltage() * .02 + battery_voltage * .98;
    render_update();
}