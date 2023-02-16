#ifndef FONT_H
#define FONT_H

#include <graphics.h>
#include <string.h>

#define FONT_DEFAULT_FILE       "fonts/6x12.bdf"
#define FONT_DEFAULT_NAME       "default"
#define FONT_DEFAULT_WIDTH      5
#define FONT_DEFAULT_HEIGHT     8

#define FONT_LARGE_FILE         "fonts/7x13.bdf"
#define FONT_LARGE_NAME         "large"
#define FONT_LARGE_WIDTH        7
#define FONT_LARGE_HEIGHT       9

#define FONT_SMALL_FILE         "fonts/6x9.bdf"
#define FONT_SMALL_NAME         "small"
#define FONT_SMALL_WIDTH        5
#define FONT_SMALL_HEIGHT       7

// #define FONT_CLOCK_FILE         "fonts/6x9.bdf"
// #define FONT_CLOCK_NAME         "clock"
// #define FONT_CLOCK_WIDTH        5
// #define FONT_CLOCK_HEIGHT       7

#define FONT_CLOCK_FILE         "fonts/6x12.bdf"
#define FONT_CLOCK_NAME         "default"
#define FONT_CLOCK_WIDTH        5
#define FONT_CLOCK_HEIGHT       8


using rgb_matrix::Color;


class GirderFont
{
public:
  rgb_matrix::Font *font;
  uint8_t width = 0, height = 0;
  int8_t kerning = 0;
  char name[10];

  enum fonts{FONT_DEFAULT, FONT_LARGE, FONT_SMALL, FONT_CLOCK};

  GirderFont() {
    font = new rgb_matrix::Font;
  }

  GirderFont(fonts newFont) {
    font = new rgb_matrix::Font;
    LoadFont(newFont);
  }

  rgb_matrix::Font *GetFont() {
    return font;
  }

  void LoadFont(fonts newFont);
};

uint16_t textRenderLength(const char *text, GirderFont *font);
void drawText(uint8_t, uint8_t, Color, const char *,
              GirderFont* = NULL, bool = false, bool = false);

#endif