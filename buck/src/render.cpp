#include <Arduino.h>
#include <TFT_eSPI.h>

// TODO OLED burn in prevention by randomly offsetting the render area on bootup
#define DWIDTH 240
#define DHEIGHT 135

extern float battery_voltage;

// Library instance
TFT_eSPI tft = TFT_eSPI();

// Create two sprites for a DMA toggle buffer
TFT_eSprite spr[2] = {TFT_eSprite(&tft), TFT_eSprite(&tft)};

// Pointers to start of Sprites in RAM (these are then "image" pointers)
uint16_t *sprPtr[2];

void render_top_line(void)
{
  spr[0].fillSprite(TFT_BLACK);
  // This function creates the signal strength meter
  // as a character array to be shown on the display.

 
  char lineOne[16] = ""; // First line of display
  spr[0].setTextSize(4);


  sprintf(lineOne,"%f",battery_voltage);
 
  spr[0].drawString(lineOne, DWIDTH / 2, DHEIGHT / 4);
  tft.pushImageDMA(0, 0 * DHEIGHT / 2, DWIDTH, DHEIGHT / 2, sprPtr[0]);
}

void render_bottom_line(void)
{
  //TODO Colorize to amber (or something else) to match your dash

  spr[1].fillSprite(TFT_BLACK);


  static char k = 0;
  char lineTwo[16] = ""; // First line of display
  spr[0].setTextSize(4);


  sprintf(lineTwo,"%c",k);
  k--;

  spr[1].setTextSize(4);
  spr[1].drawString(lineTwo, DWIDTH / 2, DHEIGHT / 4);


  tft.pushImageDMA(0, 1 * DHEIGHT / 2, DWIDTH, DHEIGHT / 2, sprPtr[1]);
}

void render_init(void)
{
  tft.init();
  tft.initDMA();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(3); // USB port on left

  // Create the 2 sprites, each is half the size of the screen
  sprPtr[0] = (uint16_t *)spr[0].createSprite(DWIDTH, DHEIGHT / 2);
  sprPtr[1] = (uint16_t *)spr[1].createSprite(DWIDTH, DHEIGHT / 2);

  // Define text datum for each Sprite
  spr[0].setTextDatum(MC_DATUM);

  tft.startWrite(); // TFT chip select held low permanently
}

bool render_update(void)
{

  if (!tft.dmaBusy())
  {
    static bool alternate_render_line = true;

    if (alternate_render_line)
      render_top_line();
    else
      render_bottom_line();

    alternate_render_line = !alternate_render_line;
    return true;
  }
  return false;
}