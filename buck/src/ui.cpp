#include <Arduino.h>
#include "render.h"
#include "mesh.h"

float battery_voltage = 0;
static int page = 2;
static int selection = 1;
#define BUTTON1_PIN 35
#define BUTTON2_PIN 0
#define BATTERY_PIN 34
#define ADC_EN 14 // ADC_EN is the ADC detection enable port

#define BUTTON_MAX 1

#define LONG_PRESS_TIME_MS 400

int pending_effect = 0;
int pending_color = 8;

static int return_page = 0;
static uint32_t send_time = 0;
static bool sending = false;

static uint8_t colors_fastled_array[8] = {0, 32, 64, 96, 128, 160, 192, 224};

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

    bool button_1_short_press = false;
    bool button_2_short_press = false;
    bool button_1_long_press = false;
    bool button_2_long_press = false;
    static uint32_t button1_press_time = 0;
    static uint32_t button2_press_time = 0;
    static bool check_button1_long = false;
    static bool check_button2_long = false;

    if (button1_debounced && !button1_debounced_previous)
    {
        check_button1_long = true;
        button1_press_time = millis();
    }

    if (button2_debounced && !button2_debounced_previous)
    {
        button2_press_time = millis();
        check_button2_long = true;
    }

    if (check_button1_long)
    {
        if (millis() - button1_press_time > LONG_PRESS_TIME_MS)
        {
            check_button1_long = false;
            button_1_long_press = true;
        }
    }

    if (check_button2_long)
    {
        if (millis() - button2_press_time > LONG_PRESS_TIME_MS)
        {
            check_button2_long = false;
            button_2_long_press = true;
        }
    }

    if (!button1_debounced && button1_debounced_previous)
    {
        check_button1_long = false;
        if (millis() - button1_press_time <= LONG_PRESS_TIME_MS)
            button_1_short_press = true;
    }

    if (!button2_debounced && button2_debounced_previous)
    {
        check_button2_long = false;
        if (millis() - button2_press_time <= LONG_PRESS_TIME_MS)
            button_2_short_press = true;
    }

    button1_debounced_previous = button1_debounced;
    button2_debounced_previous = button2_debounced;

    if (button_1_short_press || button_2_short_press)
    {
        if (page == 2)
            page = 0;
    }

    if (button_1_short_press)
    {
        if (page == 1)
        {
            send_time = millis();
            sending = true;
            page = 2;
            return_page = 1;
            char data[] = "0";
            mesh_announce_buck('P', data);
        }

        if (page == 0)
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
                if (pending_color > 8)
                    pending_color = 0;
            }
        }
    }

    if (button_2_short_press)
    {
        if (page == 0)
        {
            selection++;
            if (selection > 3)
                selection = 1;
        }
    }

    if (button_2_long_press)
    {
        if (page == 2)
            page = 0;
        if (page == 0)
            page = 1;
        else if (page == 1)
            page = 0;
    }

    if (button_1_long_press)
    {
        if (page == 1)
        {
            send_time = millis();
            sending = true;
            page = 2;
            return_page = 1;
             char data[] = "0";
            mesh_announce_buck('A', data);
        }

        if (page == 0)
        {
            char serial_text[50];

            int mode = 50000;

            int data_to_send;

            if (pending_color < 8)
                data_to_send = colors_fastled_array[pending_color];
            else
                data_to_send = -1;

            send_time = millis();
            sending = true;
            page = 2;
            return_page = 0;
            sprintf(serial_text, "%d %d", pending_effect + mode, data_to_send);
            mesh_announce_buck('B', serial_text);
        }
    }

    battery_voltage = get_voltage() * .02 + battery_voltage * .98;

    if (sending)
    {
        if (millis() - send_time > LONG_PRESS_TIME_MS)
        {
            sending = false;
            page = return_page;
        }
    }
    render_update(page, selection);
}