#include <Arduino.h>
#include "render.h"
#include "mesh.h"

float battery_voltage = 0;
static int page = 0;
static int selection = -1;
#define BUTTON1_PIN 35
#define BUTTON2_PIN 0
#define BATTERY_PIN 34
#define ADC_EN 14 // ADC_EN is the ADC detection enable port

#define BUTTON_MAX 1

int pending_effect = 0;
bool pending_rainbow = true;
int pending_color = 0;

uint32_t sent_time = 0;

uint8_t colors_fastled_array[8] = {0, 32, 64, 96, 128, 160, 192, 224};

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
    static bool ready = false;
    static int button1_counter = 0;
    static int button2_counter = 0;

    if (!digitalRead(BUTTON2_PIN))
    {
        if (button1_counter < BUTTON_MAX)
            button1_counter++;
    }
    else
    {
        if (button1_counter > -BUTTON_MAX)
            button1_counter--;
    }

    if (!digitalRead(BUTTON1_PIN))
    {
        if (button2_counter < BUTTON_MAX)
            button2_counter++;
    }
    else
    {
        if (button2_counter > -BUTTON_MAX)
            button2_counter--;
    }

    static bool button1_debounced = false;
    static bool button2_debounced = false;

    if (button1_counter == -BUTTON_MAX)
        button1_debounced = false;
    if (button1_counter == BUTTON_MAX)
        button1_debounced = true;

    if (button2_counter == -BUTTON_MAX)
        button2_debounced = false;
    if (button2_counter == BUTTON_MAX)
        button2_debounced = true;

    static bool button1_debounced_previous = false;
    static bool button2_debounced_previous = false;

    if (button1_debounced && !button1_debounced_previous)
    {
        if (!ready)
            ready = true;
        else
        {
            Serial.println("Press 1");
            if (page == 1)
            {
                page = 0;
            }
            else if (page == 0)
            {
                if (selection == -1)
                    page = 1;
                if (selection == 1)
                {
                    pending_effect++;
                    if (pending_effect > 5)
                        pending_effect = 0;
                }
                if (selection == 2)
                {
                    pending_color++;
                    if (pending_color > 7)
                        pending_color = 0;
                }
                if (selection == 3)
                {
                    pending_rainbow = !pending_rainbow;
                }

                if (selection == 0)
                {
                    char serial_text[50];

                    int mode = 50000;

                    int data_to_send = colors_fastled_array[pending_color];

                    if (pending_rainbow)
                    {
                        data_to_send = pending_color * -1 - 1;
                    }
                    sent_time = millis();
                    sprintf(serial_text, "%d %d", pending_effect + mode, data_to_send);
                    mesh_announce_buck(serial_text);
                }
            }
        }
    }
    button1_debounced_previous = button1_debounced;

    if (button2_debounced && !button2_debounced_previous)
    {
        if (!ready)
            ready = true;
        else
        {
            Serial.println("Press 2");
            if (page == 1)
                page = 0;
            else if (page == 0)
            {
                selection++;
                if (selection > 4)
                    selection = -1;
            }
        }
    }

    button2_debounced_previous = button2_debounced;

    battery_voltage = get_voltage() * .02 + battery_voltage * .98;

    if (ready)
        render_update(page, selection);
}