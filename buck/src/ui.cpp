#include <Arduino.h>
#include "render.h"
#include "mesh.h"

float battery_voltage = 0;
static int page = PAGE_LOGO;
static int selection_ota = 1;
static int selection_effect = 1;

#define BUTTON1_PIN 35
#define BUTTON2_PIN 0
#define BATTERY_PIN 34
#define ADC_EN 14 // ADC_EN is the ADC detection enable port

#define BUTTON_MAX 1

#define LONG_PRESS_TIME_MS 400
int reboot_counter = 0;
int pending_effect = 0;
int pending_color = 8;
int palette_type = 0;
int alarm_or_party = 0;
static int return_page = 0;
static uint32_t send_time = 0;
static bool sending = false;

static uint8_t colors_fastled_array[8] = {0, 32, 64, 96, 128, 160, 192, 224};
static int speed_array[8] = {20, 50, 100, 200, 450, 650, 800, 1000};

int speed_idx = 0;

static float get_voltage(void)
{
    return (((float)(analogRead(BATTERY_PIN)) / 4095.0) * 2.0 * 3.3 * (1100 / 1000.0));
}
static void mesh_reboot(void)
{
    send_time = millis();
    sending = true;
    page = PAGE_LOGO;
    return_page = PAGE_EFFECT;
    char data[10];
    sprintf(data, "66 0");
    mesh_announce_buck('C', data);
    reboot_counter = 0;
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
        if (page == PAGE_LOGO)
            page = return_page;
    }

    if (button_1_short_press)
    {
        reboot_counter = 0;
        
        if (page == PAGE_OTA)
        {

            if (selection_ota == 1)
            {
                palette_type++;
                if (palette_type > 10)
                    palette_type = 0;
            }
            else if (selection_ota == 2)
            {
                alarm_or_party++;
                if (alarm_or_party > 1)
                    alarm_or_party = 0;
            }
        }

        if (page == PAGE_EFFECT)
        {

            if (selection_effect == 1)
            {
                pending_effect++;
                if (pending_effect > 6)
                    pending_effect = 0;
                // default speeds
                if (pending_effect == 1 || pending_effect == 2 || pending_effect == 3)
                    speed_idx = 4;
                if (pending_effect == 4)
                    speed_idx = 5;
            }
            else if (selection_effect == 2)
            {
                pending_color++;
                if (pending_color > 8)
                    pending_color = 0;
            }
            else if (selection_effect == 3)
            {
                speed_idx--;
                if (speed_idx < 0)
                    speed_idx = 7;
            }
        }
    }

    if (button_2_short_press)
    {

        if (page == PAGE_EFFECT)
        {
            reboot_counter++;
            selection_effect++;
            if (selection_effect > 3)
                selection_effect = 1;
        }
        if (page == PAGE_OTA)
        {
            selection_ota++;
            if (selection_ota > 2)
                selection_ota = 1;
        }
    }

    if (button_2_long_press)
    {
        reboot_counter = 0;
        if (page == PAGE_LOGO)
            page = PAGE_EFFECT;
        if (page == PAGE_EFFECT)
            page = PAGE_OTA;
        else if (page == PAGE_OTA)
            page = PAGE_EFFECT;
    }

    if (button_1_long_press)
    {

        if (reboot_counter >= 10)
            mesh_reboot();

        reboot_counter = 0;

        if (page == PAGE_OTA)
        {
            if (selection_ota == 1)
            {
                send_time = millis();
                sending = true;
                page = PAGE_LOGO;
                return_page = PAGE_OTA;
                char data[10];
                sprintf(data, "%d 0", palette_type);
                mesh_announce_buck('C', data);
            }
            if (selection_ota == 2)
            {
                if (alarm_or_party == 0)
                {

                    send_time = millis();
                    sending = true;
                    page = PAGE_LOGO;
                    return_page = PAGE_OTA;
                    char data[] = "0";

                    static int party_rotate = 0;
                    sprintf(data, "%d", party_rotate++);
                    if (party_rotate > 5) // this needs to match the effects in the device
                        party_rotate = 0;
                    mesh_announce_buck('P', data);
                }
                if (alarm_or_party == 1)
                {
                    send_time = millis();
                    sending = true;
                    page = PAGE_LOGO;
                    return_page = PAGE_OTA;
                    char data[] = "0";
                    mesh_announce_buck('A', data);
                }
            }
        }

        if (page == PAGE_EFFECT)
        {
            char serial_text[50];

            int mode = speed_array[speed_idx] * 100;

            int data_to_send;

            if (pending_color < 8)
                data_to_send = colors_fastled_array[pending_color];
            else
                data_to_send = -1;

            send_time = millis();
            sending = true;
            page = PAGE_LOGO;
            return_page = PAGE_EFFECT;
            sprintf(serial_text, "%d %d", pending_effect + mode, data_to_send); // implement effect advancing
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
    render_update(page, selection_effect, selection_ota);
}