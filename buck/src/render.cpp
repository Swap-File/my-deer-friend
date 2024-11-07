#include <Arduino.h>
#include <TFT_eSPI.h>
#include "mesh.h"

// TODO OLED burn in prevention by randomly offsetting the render area on bootup
#define DWIDTH 240
#define DHEIGHT 135

extern float battery_voltage;
extern int totalsize;
extern uint32_t clients[];
extern int progress[];

// Library instance
TFT_eSPI tft = TFT_eSPI();

// Create two sprites for a DMA toggle buffer
TFT_eSprite spr[2] = {TFT_eSprite(&tft), TFT_eSprite(&tft)};

// Pointers to start of Sprites in RAM (these are then "image" pointers)
uint16_t *sprPtr[2];

extern int pending_effect;
extern bool pending_rainbow;

extern int pending_color;

uint16_t colors_tft_array[8] = {TFT_RED, TFT_ORANGE, TFT_YELLOW, TFT_GREEN, TFT_CYAN, TFT_BLUE, TFT_PURPLE, TFT_MAGENTA};

char color_names[9][10] = {"Red", "Orange", "Yellow", "Green", "Aqua", "Blue", "Purple", "Pink", "Rainbow"};

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

void render_top_line(int page, int selection)
{
  spr[0].fillSprite(TFT_BLACK);
  spr[0].setTextColor(TFT_BROWN);
  if (page == 0 || page == 1)
  {
    char lineOne[30] = ""; // First line of display

    sprintf(lineOne, "Nodes: %d of %d", mesh_nodes(), getnodecount());
    spr[0].drawString(lineOne, DWIDTH / 2, DHEIGHT / 4 - 17);

    battery_text(lineOne);
    spr[0].drawString(lineOne, DWIDTH / 2, DHEIGHT / 4);

    sprintf(lineOne, "CPU: %.1f÷F", (mcutemp * 1.8) + 32);
    spr[0].drawString(lineOne, DWIDTH / 2, DHEIGHT / 4 + 17);
  }
  else if (page == 2)
  {
    spr[0].setCursor(0, 0);
    spr[0].println("  \\__`\\     |'__/ ");
    spr[0].println("    `_\\\\   //_'   ");
    spr[0].println("    _.,:---;,._   ");
    spr[0].println("    \\_:     :_/   ");
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

void render_bottom_line(int page, int selection)
{
  // TODO Colorize to amber (or something else) to match your dash

  spr[1].fillSprite(TFT_BLACK);
  spr[1].setTextColor(TFT_BROWN);
  if (page == 0)
  {
    char lineOne[30] = ""; // First line of display

    if (selection == 1)
      sprintf(lineOne, "> Effect %d <", pending_effect);
    else
      sprintf(lineOne, "Effect %d", pending_effect);

    spr[1].drawString(lineOne, DWIDTH / 2, DHEIGHT / 4 - 17);

    if (selection == 2)
    {
      if (pending_color == 8)
        spr[1].setTextColor(colors_tft_array[(millis() >> 8) % 8]);
      else
        spr[1].setTextColor(colors_tft_array[pending_color]);

      sprintf(lineOne, "> %s <", color_names[pending_color]);
    }
    else
      sprintf(lineOne, "%s", color_names[pending_color]);

    spr[1].drawString(lineOne, DWIDTH / 2, DHEIGHT / 4);
    spr[1].setTextColor(TFT_BROWN);

    if (selection == 3)
      sprintf(lineOne, "> Speed <");
    else
      sprintf(lineOne, "Speed");
    spr[1].drawString(lineOne, DWIDTH / 2, DHEIGHT / 4 + 17);
    spr[1].setTextColor(TFT_BROWN);
  }
  else if (page == 1)
  {
    char lineOne[30] = ""; // First line of display

    strcat(lineOne, calc_percent(0));
    strcat(lineOne, calc_percent(1));
    strcat(lineOne, calc_percent(2));
    strcat(lineOne, calc_percent(3));

    spr[1].drawString(lineOne, DWIDTH / 2, DHEIGHT / 4 - 17); // top

    sprintf(lineOne, "");
    strcat(lineOne, calc_percent(4));
    strcat(lineOne, calc_percent(5));
    strcat(lineOne, calc_percent(6));
    strcat(lineOne, calc_percent(7));
    spr[1].drawString(lineOne, DWIDTH / 2, DHEIGHT / 4); // middle
    sprintf(lineOne, "");
    strcat(lineOne, calc_percent(8));
    strcat(lineOne, calc_percent(9));
    strcat(lineOne, calc_percent(10));
    strcat(lineOne, calc_percent(11));
    spr[1].drawString(lineOne, DWIDTH / 2, DHEIGHT / 4 + 17); // bottom
  }
  else if (page == 2)
  {
    spr[1].setCursor(0, 0);
    spr[1].println("      |@. .@|     ");
    spr[1].println("      |     |     ");
    spr[1].println("       \\.-./      ");
    spr[1].println("        `-'       ");
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
  render_top_line(2, 2);
  while (tft.dmaBusy())
  {
  }
  render_bottom_line(2, 2);

  mcutemp = temperatureRead();
}

bool render_update(int page, int selection)
{
  mcutemp = mcutemp * .95 + temperatureRead() * .05;

  if (!tft.dmaBusy())
  {
    static bool alternate_render_line = true;

    if (alternate_render_line)
      render_top_line(page, selection);
    else
      render_bottom_line(page, selection);

    alternate_render_line = !alternate_render_line;
    return true;
  }
  return false;
}