#include <Arduino.h>
#include <TFT_eSPI.h>
#include "mesh.h"
#include "render.h"

// TODO OLED burn in prevention by randomly offsetting the render area on bootup
#define DWIDTH 240
#define DHEIGHT 135

extern float battery_voltage;
extern int totalsize;
extern uint32_t clients[];
extern int progress[];
extern int speed_idx;
extern int alarm_or_party;
// Library instance
TFT_eSPI tft = TFT_eSPI();

// Create two sprites for a DMA toggle buffer
TFT_eSprite spr[2] = {TFT_eSprite(&tft), TFT_eSprite(&tft)};

// Pointers to start of Sprites in RAM (these are then "image" pointers)
uint16_t *sprPtr[2];

extern int pending_effect;
extern bool pending_rainbow;
extern int palette_type;
extern int pending_color;
extern int reboot_counter;

uint16_t colors_tft_array[8] = {TFT_RED, TFT_ORANGE, TFT_YELLOW, TFT_GREEN, TFT_CYAN, TFT_BLUE, TFT_PURPLE, TFT_MAGENTA};

char color_names[9][10] = {"Red", "Orange", "Yellow", "Green", "Aqua", "Blue", "Purple", "Pink", "Rainbow"};
char palette_names[12][10] = {"HSV", "Xmas1", "Ocean", "Party", "Forest", "Rainbow1", "Heat", "Rainbow2", "Cloud", "Lava", "Xmass2"};

float mcutemp = 70;

void battery_text(char *lineOne)
{
  if (totalsize == 0)
    sprintf(lineOne, "OTA DISABLED");
  else
  {
    if (battery_voltage > 4.5)
      sprintf(lineOne, "Charging");
    else
      sprintf(lineOne, "Battery: %.2fV", battery_voltage);
  }
}

int getnodecount(void)
{
  int nodecount = 0;
  for (int i = 0; i < MAX_NODES; i++)
  {
    if (clients[i] != 0)
      nodecount++;
  }
  return nodecount;
}

void render_top_line(int page, int selection_effect, int selection_ota)
{
  spr[0].fillSprite(TFT_BLACK);
  spr[0].setTextColor(TFT_BROWN);
  if (page == PAGE_EFFECT || page == PAGE_OTA)
  {
    char lineOne[30] = ""; // First line of display

    sprintf(lineOne, "Nodes: %d of %d", mesh_nodes(), getnodecount());
    spr[0].drawString(lineOne, DWIDTH / 2, DHEIGHT / 4 - 8 - 16);

    battery_text(lineOne);
    spr[0].drawString(lineOne, DWIDTH / 2, DHEIGHT / 4 - 8);

    sprintf(lineOne, "CPU: %.1f÷F", (mcutemp * 1.8) + 32);
    spr[0].drawString(lineOne, DWIDTH / 2, DHEIGHT / 4 + 8);

    if (page == PAGE_OTA)
    {
      if (selection_ota == 1)
        sprintf(lineOne, "> %s <", palette_names[palette_type]);
      else
        sprintf(lineOne, "%s", palette_names[palette_type]);
      spr[0].drawString(lineOne, DWIDTH / 2, DHEIGHT / 4 + 8 + 16);
    }
  }
  else if (page == PAGE_LOGO)
  {
    spr[0].drawString(" \\__`\\     |'__/ ", DWIDTH / 2, DHEIGHT / 4 - 8 - 16);
    spr[0].drawString("   `_\\\\   //_'   ", DWIDTH / 2, DHEIGHT / 4 - 8);
    spr[0].drawString("   _.,:---;,._   ", DWIDTH / 2, DHEIGHT / 4 + 8);
    spr[0].drawString("   \\_:     :_/   ", DWIDTH / 2, DHEIGHT / 4 + 8 + 16);
  }
  tft.pushImageDMA(0, 0 * DHEIGHT / 2, DWIDTH, DHEIGHT / 2, sprPtr[0]);
}

char *calc_percent(int index)
{
  static char temp_str[6];

  if (clients[index] == 0)
  {
    sprintf(temp_str, "");
    return temp_str;
  }
  else
    sprintf(temp_str, "%.2f", ((float)progress[index]) / totalsize);

  if (index != 3 || index != 7 || index != 11)
    strcat(temp_str, " ");

  return temp_str;
}

void render_bottom_line(int page, int selection_effect, int selection_ota)
{
  // TODO Colorize to amber (or something else) to match your dash

  spr[1].fillSprite(TFT_BLACK);
  spr[1].setTextColor(TFT_BROWN);

  if (page == PAGE_EFFECT)
  {
    char lineOne[30] = ""; // First line of display
    if (reboot_counter < 10)
    {
      if (selection_effect == 1)
        sprintf(lineOne, "> Effect %d <", pending_effect);
      else
        sprintf(lineOne, "Effect %d", pending_effect);

      spr[1].drawString(lineOne, DWIDTH / 2, DHEIGHT / 4 - 8);

      if (selection_effect == 2)
      {
        if (pending_color == 8)
          spr[1].setTextColor(colors_tft_array[(millis() >> 8) % 8]);
        else
          spr[1].setTextColor(colors_tft_array[pending_color]);

        sprintf(lineOne, "> %s <", color_names[pending_color]);
      }
      else
        sprintf(lineOne, "%s", color_names[pending_color]);

      spr[1].drawString(lineOne, DWIDTH / 2, DHEIGHT / 4 + 8);

      spr[1].setTextColor(TFT_BROWN);

      if (selection_effect == 3)
        sprintf(lineOne, "> Speed %d<", speed_idx);
      else
        sprintf(lineOne, "Speed %d", speed_idx);
      spr[1].drawString(lineOne, DWIDTH / 2, DHEIGHT / 4 + 8 + 16);
      spr[1].setTextColor(TFT_BROWN);
    }
    else
    {
      sprintf(lineOne, "REBOOT?", speed_idx);
      spr[1].drawString(lineOne, DWIDTH / 2, DHEIGHT / 4 + 8 + 16);
      spr[1].setTextColor(TFT_BROWN);
    }
  }
  else if (page == PAGE_OTA)
  {
    char lineOne[30] = ""; // First line of display

    if (selection_ota == 2)
    {
      if (alarm_or_party == 0)
        sprintf(lineOne, "> Party <", alarm_or_party);
      else if (alarm_or_party == 1)
        sprintf(lineOne, "> Alarm <", alarm_or_party);
    }
    else
    {
      if (alarm_or_party == 0)
        sprintf(lineOne, "Party", alarm_or_party);
      else if (alarm_or_party == 1)
        sprintf(lineOne, "Alarm", alarm_or_party);
    }

    spr[1].drawString(lineOne, DWIDTH / 2, DHEIGHT / 4 - 8 - 16); // top

    sprintf(lineOne, "");
    strcat(lineOne, calc_percent(0));
    strcat(lineOne, calc_percent(1));
    strcat(lineOne, calc_percent(2));
    strcat(lineOne, calc_percent(3));

    spr[1].drawString(lineOne, DWIDTH / 2, DHEIGHT / 4 - 8); // top

    sprintf(lineOne, "");
    strcat(lineOne, calc_percent(4));
    strcat(lineOne, calc_percent(5));
    strcat(lineOne, calc_percent(6));
    strcat(lineOne, calc_percent(7));
    spr[1].drawString(lineOne, DWIDTH / 2, DHEIGHT / 4 + 8); // middle
    sprintf(lineOne, "");
    strcat(lineOne, calc_percent(8));
    strcat(lineOne, calc_percent(9));
    strcat(lineOne, calc_percent(10));
    strcat(lineOne, calc_percent(11));
    spr[1].drawString(lineOne, DWIDTH / 2, DHEIGHT / 4 + 8 + 16); // bottom
  }
  else if (page == PAGE_LOGO)
  {
    spr[1].drawString("     |@. .@|     ", DWIDTH / 2, DHEIGHT / 4 - 8 - 16);
    spr[1].drawString("     |     |     ", DWIDTH / 2, DHEIGHT / 4 - 8);
    spr[1].drawString("      \\.-./      ", DWIDTH / 2, DHEIGHT / 4 + 8);
    spr[1].drawString("       `-'       ", DWIDTH / 2, DHEIGHT / 4 + 8 + 16);
  }
  tft.pushImageDMA(0, 1 * DHEIGHT / 2, DWIDTH, DHEIGHT / 2, sprPtr[1]);
}

void render_init(void)
{
  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(1); // USB port on right
  tft.initDMA();
  // Create the 2 sprites, each is half the size of the screen
  sprPtr[0] = (uint16_t *)spr[0].createSprite(DWIDTH, DHEIGHT / 2);
  sprPtr[1] = (uint16_t *)spr[1].createSprite(DWIDTH, DHEIGHT / 2);
  spr[0].setTextSize(2);
  spr[1].setTextSize(2);
  spr[0].setTextDatum(MC_DATUM);
  spr[1].setTextDatum(MC_DATUM);
  tft.startWrite(); // TFT chip select held low permanently

  while (tft.dmaBusy())
  {
  }
  render_top_line(PAGE_LOGO, 1, 1);
  while (tft.dmaBusy())
  {
  }
  render_bottom_line(PAGE_LOGO, 1, 1);

  mcutemp = temperatureRead();
}

bool render_update(int page, int selection_effect, int selection_ota)
{
  mcutemp = mcutemp * .95 + temperatureRead() * .05;

  if (!tft.dmaBusy())
  {
    static bool alternate_render_line = true;

    if (alternate_render_line)
      render_top_line(page, selection_effect, selection_ota);
    else
      render_bottom_line(page, selection_effect, selection_ota);

    alternate_render_line = !alternate_render_line;
    return true;
  }
  return false;
}