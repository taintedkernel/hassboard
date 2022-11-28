#include "datetime.h"
#include "display.h"
#include "logger.h"
#include "widget.h"

#include <canvas.h>
#include <led-matrix.h>
#include <string.h>

#include <cstring>

using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;
using rgb_matrix::Color;
using rgb_matrix::Font;

Color colorBlack      = Color(0, 0, 0);
Color colorDarkGrey   = Color(16, 16, 16);
Color colorGrey       = Color(64, 64, 64);
Color colorLightGrey  = Color(128, 128, 128);
Color colorWhite      = Color(255, 255, 255);
// Color colorDate       = Color(125, 200, 255);
// Color colorDate       = Color(96, 148, 192);
Color colorDate       = Color(96, 128, 148);
// Color colorTime     = Color(250, 80, 0);
Color colorTime       = Color(248, 96, 8);
// Color colorText     = Color(125, 200, 255);
// Color colorText     = Color(80, 160, 224);
// Color colorText     = Color(64, 148, 192);
// Color colorText     = Color(128, 148, 160);
Color colorTextDay    = Color(112, 148, 176);
Color colorTextNight  = Color(176, 148, 112);
Color colorText       = colorTextDay;
Color colorTextDark   = Color(56, 74, 88);
Color colorAlert      = Color(248, 48, 8);

rgb_matrix::RGBMatrix *matrix;
rgb_matrix::PixelMapper *mapper;


extern int8_t clockOffset;
extern uint8_t rowDayStart;
extern uint8_t rowDateStart;
extern uint8_t rowTimeStart;
extern uint8_t clockWidth;

extern GirderFont *defaultFont, *clockFont;

bool setupDisplay(uint8_t configNum)
{
  RGBMatrix::Options displaySettings;
  rgb_matrix::RuntimeOptions runtimeSettings;

   _log("initializing display");

  // Configure settings for display
  displaySettings.hardware_mapping = "adafruit-hat-pwm";

  // Settings for the primary, composite 128x64 panel
  if (configNum == 1) {
    displaySettings.cols = 64;
    displaySettings.rows = 32;
    displaySettings.chain_length = 4;
    displaySettings.parallel = 1;
    displaySettings.pixel_mapper_config = "U-mapper";
    displaySettings.brightness = 50;
    displaySettings.led_rgb_sequence = "RBG";
  }
  // Settings for a 2nd smartgirder, single 128x64 panel
  else if (configNum == 2) {
    displaySettings.cols = 128;
    displaySettings.rows = 64;
    displaySettings.row_address_type = 3;
  }
  else {
    _error("unknown config: %d", configNum);
    return false;
  }

  runtimeSettings.daemon = 0;
  runtimeSettings.drop_privileges = 1;
  runtimeSettings.gpio_slowdown = 4;

  // Initialize matrix
  matrix = RGBMatrix::CreateFromOptions(displaySettings, runtimeSettings);
  if (matrix == NULL)
  {
    _error("unable to initialize matrix, exiting");
    return false;
  }

  // Clearing matrix
  matrix->Fill(0, 0, 0);
  matrix->SetBrightness(50);

  // Load fonts
  _log("loading fonts");
  defaultFont = new GirderFont(GirderFont::FONT_DEFAULT);
  clockFont = new GirderFont(GirderFont::FONT_CLOCK);

  return true;
}

void shutdownDisplay()
{
  matrix->Clear();
  delete matrix;
}

void setBrightness(uint8_t brightness)
{
  matrix->SetBrightness(brightness);
}


/* // Calculate color from hex value + brightness
Color colorBright(uint16_t value, int br) {
  unsigned r = (value & 0xF800) >> 8;       // rrrrr... ........ -> rrrrr000
  unsigned g = (value & 0x07E0) >> 3;       // .....ggg ggg..... -> gggggg00
  unsigned b = (value & 0x1F) << 3;         // ............bbbbb -> bbbbb000

  return colorBright(r, g, b, br);
}
    r & 248 (0b 1111 1000) << 8
      +
    g & 252 (0b 1111 1100) << 3
      +
    b >> 3

Color hexColorBright2RGB(uint16_t hexValue, int br)
{
  unsigned r = (hexValue & 0xF800) >> 8;       // rrrrr... ........ -> rrrrr000
  unsigned g = (hexValue & 0x07E0) >> 3;       // .....ggg ggg..... -> gggggg00
  unsigned b = (hexValue & 0x1F) << 3;         // ............bbbbb -> bbbbb000
  // return display.color565(r*br/100,g*br/100,b*br/100);
  return Color(r*br/100, g*br/100 ,b*br/100);
  // return Color(r, g, b);
} */

// Draw a filled rectangle at (x,y) with width, height and color
void drawRect(uint16_t x_start, uint16_t y_start,
  uint16_t width, uint16_t height, Color color)
{
  // _debug("drawRect x,y,w,h: %d,%d,%d,%d", x_start, y_start, width, height);
  for (uint16_t x = x_start; x < x_start + width; x++) {
    for (uint16_t y = y_start; y < y_start + height; y++) {
      matrix->SetPixel(x, y, color.r, color.g, color.b);
    }
  }
}

// Draw an image of width, height at (x,y)
void drawIcon(int x, int y, int width, int height, const uint8_t *image)
{
  SetImage(matrix, x, y, image, width * height * 3, width,
      height, false);
}

// Show the day of week, date and time
void displayClock(bool force)
{
  char buffer[6];
  static uint8_t lMinute = 0;

  time_t local = time(0);
  tm *localtm = localtime(&local);

  //
  // Skip if no change and not forced
  //
  // This assumes/relies that every clock field update
  // will include a change in the "minute", which is
  // correct unless we include seconds in the clock
  //
  if (minute(localtm) == lMinute && !force) {
    return;
  }
  else {
    lMinute = minute(localtm);
  }

  // Clear the widget
  uint8_t clockX = clockOffset;
  uint8_t dateFontWidth = FONT_CLOCK_WIDTH;
  uint8_t renderLen;
  int8_t offset;
  drawRect(clockX, 0, clockWidth, 32, colorBlack);

  // Render the day of week
  offset = (clockWidth - (strlen(s_weekday(localtm)) * dateFontWidth)) / 2;
  drawText(clockX + offset, rowDayStart, colorDate, s_weekday(localtm), clockFont);

  // Render the date
  snprintf(buffer, 6, "%d/%d", month(localtm), day(localtm));
  renderLen = textRenderLength(buffer, clockFont);
  offset = (clockWidth / 2) - (renderLen / 2);
  drawText(clockX + offset, rowDateStart, colorDate, buffer, clockFont, true);

  // Render the time
  snprintf(buffer, 6, "%d:%02d", hour(localtm), minute(localtm));
  renderLen = textRenderLength(buffer, clockFont);
  offset = (clockWidth / 2) - (renderLen / 2);
  drawText(clockX + offset, rowTimeStart, colorTime, buffer, clockFont, true);
}

// Debugging routine to draw some rainbow stripes
/* void _debugRainbowStripes()
{
  uint16_t red = display.color565(255, 0, 0);
  uint16_t yellow = display.color565(255, 255, 0);
  uint16_t green = display.color565(0, 255, 0);
  uint16_t cyan = display.color565(0, 255, 255);
  uint16_t blue = display.color565(0, 0, 255);
  uint16_t purple = display.color565(255, 0, 255);
  uint16_t white = display.color565(255, 255, 255);

  uint8_t x = 0;
  uint16_t c = red;
  display.drawPixel(x,0,c);
  display.drawPixel(x,1,c);
  display.drawPixel(x,2,c);
  display.drawPixel(x,3,c);

  x = 1; c = yellow;
  display.drawPixel(x,0,c);
  display.drawPixel(x,1,c);
  display.drawPixel(x,2,c);
  display.drawPixel(x,3,c);

  x = 2; c = green;
  display.drawPixel(x,0,c);
  display.drawPixel(x,1,c);
  display.drawPixel(x,2,c);
  display.drawPixel(x,3,c);

  x = 3; c = cyan;
  display.drawPixel(x,0,c);
  display.drawPixel(x,1,c);
  display.drawPixel(x,2,c);
  display.drawPixel(x,3,c);

  x = 4; c = blue;
  display.drawPixel(x,0,c);
  display.drawPixel(x,1,c);
  display.drawPixel(x,2,c);
  display.drawPixel(x,3,c);

  x = 5; c = purple;
  display.drawPixel(x,0,c);
  display.drawPixel(x,1,c);
  display.drawPixel(x,2,c);
  display.drawPixel(x,3,c);

  x = 6; c = white;
  display.drawPixel(x,0,c);
  display.drawPixel(x,1,c);
  display.drawPixel(x,2,c);
  display.drawPixel(x,3,c);
} */