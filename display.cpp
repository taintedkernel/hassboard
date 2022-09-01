#include "datetime.h"
#include "display.h"
#include "logger.h"
#include "widget.h"

#include <canvas.h>
#include <led-matrix.h>
#include <string.h>

#include <cstring>

using rgb_matrix::RGBMatrix;
using rgb_matrix::DrawText;
using rgb_matrix::Canvas;
using rgb_matrix::Color;
using rgb_matrix::Font;

Color colorBlack      = Color(0, 0, 0);
Color colorDarkGrey   = Color(16, 16, 16);
Color colorGrey       = Color(64, 64, 64);
Color colorLightGrey  = Color(128, 128, 128);
Color colorWhite      = Color(255, 255, 255);
Color colorDate       = Color(125, 200, 255);
// Color colorTime     = Color(250, 80, 0);
Color colorTime       = Color(248, 96, 8);
// Color colorText     = Color(125, 200, 255);
// Color colorText     = Color(80, 160, 224);
// Color colorText     = Color(64, 148, 192);
// Color colorText     = Color(128, 148, 160);
Color colorText       = Color(112, 148, 176);
Color colorDarkText   = Color(56, 74, 88);

rgb_matrix::RGBMatrix *matrix;
rgb_matrix::Font *defaultFont;
rgb_matrix::PixelMapper *mapper;

extern int8_t clockOffset;
extern uint8_t rowDayStart;
extern uint8_t rowDateStart;
extern uint8_t rowTimeStart;

bool setupDisplay()
{
  RGBMatrix::Options displaySettings;
  rgb_matrix::RuntimeOptions runtimeSettings;

   _log("initializing display");

  // Configure settings for display
  displaySettings.hardware_mapping = "adafruit-hat-pwm";
  displaySettings.cols = 64;
  displaySettings.rows = 32;
  displaySettings.chain_length = 4;
  displaySettings.parallel = 1;
  displaySettings.pixel_mapper_config = "U-mapper";
  displaySettings.brightness = 50;
  displaySettings.led_rgb_sequence = "RBG";

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

  // Load font
  _debug("loading font");
  defaultFont = new rgb_matrix::Font;
  defaultFont->LoadFont(FONT_FILE);

  return true;
}

void shutdownDisplay()
{
  matrix->Clear();
  delete matrix;
}

/* Color hexColorBright2RGB(uint16_t hexValue, int br)
{
  unsigned r = (hexValue & 0xF800) >> 8;       // rrrrr... ........ -> rrrrr000
  unsigned g = (hexValue & 0x07E0) >> 3;       // .....ggg ggg..... -> gggggg00
  unsigned b = (hexValue & 0x1F) << 3;         // ............bbbbb -> bbbbb000
  // return display.color565(r*br/100,g*br/100,b*br/100);
  return Color(r*br/100, g*br/100 ,b*br/100);
  // return Color(r, g, b);
} */

// Render text in color at (x,y)
// int drawText(uint8_t x, uint8_t y, Font font, Color color, const char *text)
int drawText(uint8_t x, uint8_t y, Color color, const char *text, Font *font)
{
  if (font == NULL) {
    font = defaultFont;
  }
  // _debug("drawText x,y,msg: %d,%d,\"%s\"", x, y, text);

  if (strchr(text, '.') != NULL && strlen(text) != 3) {
      _warn("unable to autoparse text for custom rendering, using default");
  }
  // TODO: Make this generic and able to handle strings beyond only "d.d" format
  else if (strchr(text, '.') != NULL && strlen(text) == 3)
  {
    uint8_t len = strlen(text);

    // _debug("strlen=%d", len);
    // _debug("%c", *(text+len-1));

    // Create buffer to store single character
    char digit[2];
    // digit[0] = *(text+len-1); // Grab the last character/digit
    digit[0] = *(text+2);     // Grab the last character/digit
    digit[1] = '\0';          // Terminate string

    // Render last character
    DrawText(matrix, *font, x + 6*2, y+FONT_HEIGHT, color, NULL, digit, 0);

    // Make a copy of our existing string
    char *newText = new char[len];
    strncpy(newText, text, len);

    u_int8_t customRender = 3;
    if (customRender == 1)
    {
      // Small period
      matrix->SetPixel(x + 10, y+FONT_HEIGHT-1, color.r, color.g, color.b);
      newText[len-2] = '\0';
      x += 4;
    }
    else if (customRender == 2)
    {
      // Larger/default period
      // Replace last digit with null to remove from string
      newText[len-1] = '\0';

      // Advance x to set spacing
      x += 1;
    }
    else if (customRender == 3)
    {
      // Larger custom period with 2 adjustments and custom spacing
      // Adjust spacing for "skinny" digits
      if (digit[0] == '0' || digit[0] == '1') {
        x++;
      }

      // // Render decimal point
      // digit[0] = '.';   // *(text+len-2);
      // DrawText(matrix, *font, x + 6*1 + 2, y+FONT_HEIGHT, color, NULL, digit, 0);
      matrix->SetPixel(x + 10, y+FONT_HEIGHT-1, color.r, color.g, color.b);

      digit[0] = *(text);

      // Render first digit
      return DrawText(matrix, *font, x + 4, y+FONT_HEIGHT, color, NULL, digit, 0);
    }

    return DrawText(matrix, *font, x, y+FONT_HEIGHT, color, NULL, newText, 0);
  }

  // Library references bottom-left corner as origin (instead of top)
  //
  // Add a fixed-offset to compensate, the benefit here is easily
  // tweaking the vertical position/placement
  //
  // Using font.height() resulted in too large of gap
  return DrawText(matrix, *font, x, y+FONT_HEIGHT, color, NULL, text, 0);
}

// Render text with a custom formatting/profile
// Right now, this is very basic and just apply a fixed negative kerning offset
// But this is to be expanded to custom variable-width character rendering
int drawTextCustom(uint8_t x, uint8_t y, Color color, const char *text, Font *font, uint8_t hSpacing, uint8_t fontHeight)
{
  return DrawText(matrix, *font, x, y+fontHeight, color, NULL, text, -1);
}

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
  SetImage(matrix, x, y, image, width * height * 3, width, height, false);
}

// Show the day of week, date and time
void displayClock(bool force)
{
  char dateText[6], timeText[6];
  static uint8_t lMinute = 0;

  time_t local = time(0);
  tm *localtm = localtime(&local);

  /* thisHour = hour(localtm);
  if ((thisHour == dimHour) and not (brightness == dimBrightness)) {
    brightness = dimBrightness;
    forceRefresh = true;
  }
  else if ((thisHour == brightHour) and not (brightness == brightBrightness)) {
    brightness = brightBrightness;
    forceRefresh = true;;
  } */

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

  int xOffset = clockOffset;
  drawRect(xOffset + 32, 0, 32, 32, colorBlack);

  // Render the day of week
  int dayPos = (32 - (strlen(s_weekday(localtm)) * 6)) / 2;
  int xDay = xOffset + 32 + dayPos;
  drawText(xDay, rowDayStart, colorDate, s_weekday(localtm));

  // Render the date
  uint8_t dateType = 2;
  if (dateType == 1)
  {
    snprintf(dateText, 6, "%2d/%02d", month(localtm), day(localtm));
    drawText(xOffset + 32, rowDateStart, colorDate, dateText);
  }
  else if (dateType == 2)
  {
    snprintf(dateText, 3, "%2d", month(localtm));
    drawText(xOffset + 32, rowDateStart, colorDate, dateText);
    snprintf(dateText, 3, "%02d", day(localtm));
    drawText(xOffset + 32 + 6*3, rowDateStart, colorDate, dateText);
    matrix->SetPixel(xOffset + 32 + 6*2 + 3,
      rowDateStart+1, colorDate.r, colorDate.g, colorDate.b);
    matrix->SetPixel(xOffset + 32 + 6*2 + 3,
      rowDateStart+2, colorDate.r, colorDate.g, colorDate.b);
    matrix->SetPixel(xOffset + 32 + 6*2 + 2,
      rowDateStart+3, colorDate.r, colorDate.g, colorDate.b);
    matrix->SetPixel(xOffset + 32 + 6*2 + 2,
      rowDateStart+4, colorDate.r, colorDate.g, colorDate.b);
    matrix->SetPixel(xOffset + 32 + 6*2 + 2,
      rowDateStart+5, colorDate.r, colorDate.g, colorDate.b);
    matrix->SetPixel(xOffset + 32 + 6*2 + 1,
      rowDateStart+6, colorDate.r, colorDate.g, colorDate.b);
    matrix->SetPixel(xOffset + 32 + 6*2 + 1,
      rowDateStart+7, colorDate.r, colorDate.g, colorDate.b);
  }

  // Render the time
  uint8_t timeType = 2;
  if (timeType == 1)
  {
    snprintf(timeText, 6, "%02d:%02d", hour(localtm), minute(localtm));
    drawText(xOffset + 32, rowTimeStart, colorTime, timeText);
  }
  else if (timeType == 2)
  {
    snprintf(timeText, 3, "%02d", hour(localtm));
    drawText(xOffset + 32+2, rowTimeStart, colorTime, timeText);
    snprintf(timeText, 3, "%02d", minute(localtm));
    drawText(xOffset + 32+6*3, rowTimeStart, colorTime, timeText);
    matrix->SetPixel(xOffset + 32 + 6*2 + 3,
      rowTimeStart+2, colorTime.r, colorTime.g, colorTime.b);
    matrix->SetPixel(xOffset + 32 + 6*2 + 3,
      rowTimeStart+3, colorTime.r, colorTime.g, colorTime.b);
    matrix->SetPixel(xOffset + 32 + 6*2 + 3,
      rowTimeStart+5, colorTime.r, colorTime.g, colorTime.b);
    matrix->SetPixel(xOffset + 32 + 6*2 + 3,
      rowTimeStart+6, colorTime.r, colorTime.g, colorTime.b);
  }
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