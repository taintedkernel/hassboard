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
GirderFont *defaultFont;
GirderFont *clockFont;

extern int8_t clockOffset;
extern uint8_t rowDayStart;
extern uint8_t rowDateStart;
extern uint8_t rowTimeStart;
extern uint8_t clockWidth;

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

  // Settings for 2nd smartgirder, single 128x64 panel
  // displaySettings.hardware_mapping = "adafruit-hat-pwm";
  // displaySettings.cols = 128;
  // displaySettings.rows = 64;
  // displaySettings.row_address_type = 3;
  // displaySettings.brightness = 50;

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

/* Color hexColorBright2RGB(uint16_t hexValue, int br)
{
  unsigned r = (hexValue & 0xF800) >> 8;       // rrrrr... ........ -> rrrrr000
  unsigned g = (hexValue & 0x07E0) >> 3;       // .....ggg ggg..... -> gggggg00
  unsigned b = (hexValue & 0x1F) << 3;         // ............bbbbb -> bbbbb000
  // return display.color565(r*br/100,g*br/100,b*br/100);
  return Color(r*br/100, g*br/100 ,b*br/100);
  // return Color(r, g, b);
} */

// Non full-width glyphs are not left-justified, so
// this calculates the offset to allow for shifting
// the rendering location to compensate
int8_t vGlyphOffset(const char glyph, GirderFont *font)
{
  if (strcmp(font->name, "clock") == 0)
  {
    if (glyph == '0')
      return 1;
    else if (glyph == '1')
      return 1;
    else if (glyph == '4')
      return 0;
    else if (glyph >= '2' && glyph <= '9')
      return 1;
    else {
      return 0;
    }
  }
  else if (strcmp(font->name, "default") == 0)
  {
    if (glyph == '0') {
      return 1;
    } else if (glyph == '1') {
      return 1;
    } else if (glyph == '/') {
      return 1;
    } else {
      return 0;
    }
  }
  else {
    return 0;
  }
}

// Calculate the width of a glyph, with the spacing between them
uint8_t vGlyphWidth(const char glyph, GirderFont *font)
{
  // The approach here is to set the width to the full font
  // width by default, and adjust (shrink) the width with an
  // offset for any glyphs that are not full-width
  // int8_t offset = 0;
  int8_t offset = 1;

  // Custom glyphs
  if (glyph == '.')
    return offset + 1;
  if (glyph == ':')
    return offset + 3;
  if (glyph == '/' && strcmp(font->name, "default") == 0)
    return offset + 5;
  if (glyph == '/' && strcmp(font->name, "default") != 0)
    return offset + 3;

  // Adjust the "narrower" glyphs
  if (strcmp(font->name, "clock") == 0)
  {
    if (glyph == '0')
      offset -= 1;
    else if (glyph == '1')
      offset -= 2;
    else if (glyph == '4')
      offset -= 0;
    else if (glyph >= '2' && glyph <= '9')
      offset -= 1;
    else {
      // noop
    }
  }
  else if (strcmp(font->name, "default") == 0)
  {
    if (glyph == '0')
      offset -= 1;
    else if (glyph == '1')
      offset -= 2;
    else {
      // noop
    }
  }

  return font->width + offset;
}

// Render a variable-width glyph to the matrix
void renderGlyph(const char glyph, uint8_t x, uint8_t y,
    GirderFont *font, Color color)
{
  char buffer[2];
  buffer[0] = glyph;
  buffer[1] = '\0';

  // _debug("rendering glyph: '%s'", buffer);
  if (strcmp(font->name, "default") == 0)
  {
    if (glyph == '.') {
      matrix->SetPixel(x, y-2, color.r, color.g, color.b);
      return;
    }
    else if (glyph == ':') {
      matrix->SetPixel(x+1, y-2, color.r, color.g, color.b);
      matrix->SetPixel(x+1, y-3, color.r, color.g, color.b);
      matrix->SetPixel(x+1, y-5, color.r, color.g, color.b);
      matrix->SetPixel(x+1, y-6, color.r, color.g, color.b);
      return;
    }
    else if (glyph == '/') {
      matrix->SetPixel(x+1, y-2, color.r, color.g, color.b);
      matrix->SetPixel(x+1, y-3, color.r, color.g, color.b);
      matrix->SetPixel(x+2, y-4, color.r, color.g, color.b);
      matrix->SetPixel(x+2, y-5, color.r, color.g, color.b);
      matrix->SetPixel(x+2, y-6, color.r, color.g, color.b);
      matrix->SetPixel(x+3, y-7, color.r, color.g, color.b);
      matrix->SetPixel(x+3, y-8, color.r, color.g, color.b);
      return;
    }
  }
  else if (strcmp(font->name, "clock") == 0)
  {
    if (glyph == '.') {
      matrix->SetPixel(x, y-2, color.r, color.g, color.b);
      return;
    }
    else if (glyph == ':') {
      matrix->SetPixel(x+1, y-2, color.r, color.g, color.b);
      matrix->SetPixel(x+1, y-3, color.r, color.g, color.b);
      matrix->SetPixel(x+1, y-5, color.r, color.g, color.b);
      matrix->SetPixel(x+1, y-6, color.r, color.g, color.b);
      return;
    }
    else if (glyph == '/') {
      matrix->SetPixel(x, y-2, color.r, color.g, color.b);
      matrix->SetPixel(x, y-3, color.r, color.g, color.b);
      matrix->SetPixel(x+1, y-4, color.r, color.g, color.b);
      matrix->SetPixel(x+1, y-5, color.r, color.g, color.b);
      matrix->SetPixel(x+2, y-6, color.r, color.g, color.b);
      matrix->SetPixel(x+2, y-7, color.r, color.g, color.b);
      return;
    }
  }

  // Call upstream library to render, and adjust position
  DrawText(matrix, *font->font, x - vGlyphOffset(glyph, font),
      y-1, color, NULL, buffer, 0);
}

// Calculate what the actual rendered length of a string will be
uint16_t textRenderLength(const char *text, GirderFont *font)
{
  uint16_t length = 0;

  for (size_t i=0; i<strlen(text); i++) {
    length += vGlyphWidth(text[i], font);
  }

  // Remove trailing space
  return length - 1;
}

// Render text in color at (x,y)
int drawText(uint8_t x, uint8_t y, Color color, const char *text,
        GirderFont *font, bool vWidth)
{
  rgb_matrix::Font *mFont;

  if (font == NULL) {
    font = defaultFont;
  }
  mFont = font->GetFont();

  // _debug("drawText x,y,msg: %d,%d,\"%s\"", x, y, text);

  if (strchr(text, '.') != NULL && strlen(text) != 3) {
    _warn("unable to autoparse text for custom rendering, using default");
  }

  if (vWidth)
  {
    uint16_t xStart = x;
    uint8_t textLen = strlen(text);
    uint16_t renderLen = textRenderLength(text, font);

    _debug("vstring='%s' len=%d renderLen=%d", text, textLen,
        renderLen);

    char glyph;
    for (uint8_t idx=0; idx<textLen; idx++)
    {
      glyph = *(text+idx);
      // _debug("glyph: '%c'", glyph);
      renderGlyph(glyph, xStart, y+font->height, font, color);
      xStart += vGlyphWidth(glyph, font);
      // _debug("new xStart: %d", xStart);
    }

    return 0;
  }

  // "x.y" strings with variable width not enabled
  else if (strchr(text, '.') != NULL && strlen(text) == 3 && !vWidth)
  {
    uint8_t textLen = strlen(text);
    uint16_t renderLen = textRenderLength(text, font);

    _debug("string='%s' len=%d renderLen=%d", text, textLen,
        renderLen);

    // Create buffer to store single character
    char digit[2];
    digit[0] = *(text+2);     // Grab the last character/digit
    digit[1] = '\0';          // Terminate string

    // Render last character
    DrawText(matrix, *mFont, x+(font->width+1)*2,
        y+font->height, color, NULL, digit, 0);

    // Make a copy of our existing string
    char *newText = new char[textLen];
    strncpy(newText, text, textLen);

    u_int8_t customRender = 3;
    if (customRender == 1)
    {
      // Small period
      matrix->SetPixel(x + 10, y+font->height-1, color.r, color.g, color.b);
      newText[textLen-2] = '\0';
      x += 4;

      return DrawText(matrix, *mFont, x, y+font->height, color,
          NULL, newText, 0);
    }
    else if (customRender == 2)
    {
      // Larger/default period
      // Replace last digit with null to remove from string
      newText[textLen-1] = '\0';

      // Advance x to set spacing
      x += 1;

      return DrawText(matrix, *mFont, x, y+font->height, color,
          NULL, newText, 0);
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
      // DrawText(matrix, *font, x + 6*1 + 2,
      //    y+FONT_DEFAULT_HEIGHT, color, NULL, digit, 0);
      matrix->SetPixel(x + 10, y+font->height-1, color.r,
          color.g, color.b);

      digit[0] = *(text);

      if (digit[0] == '1') {
        x++;
      }

      // Render first digit
      return DrawText(matrix, *mFont, x+4, y+font->height,
          color, NULL, digit, 0);
    }
  }
  else
  {
    // Library references bottom-left corner as origin (instead of top)
    //
    // Add a fixed y-offset to compensate, the benefit here is easily
    // tweaking the vertical position/placement
    //
    // Using font.height() resulted in too large of gap
    return DrawText(matrix, *font->GetFont(), x, y+font->height,
        color, NULL, text, 0);
  }

  // Should not reach here, set only to clear warnings
  return 0;
}

// Render text with a custom formatting/profile
// Right now, this is very basic and just apply a fixed negative kerning offset
// But this is to be expanded to custom variable-width character rendering
int drawTextCustom(uint8_t x, uint8_t y, Color color, const char *text, GirderFont *font, bool vWidth)
{
  return DrawText(matrix, *font->GetFont(), x, y+font->height,
      color, NULL, text, -1);
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