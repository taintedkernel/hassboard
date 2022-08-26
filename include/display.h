#ifndef DISPLAY_H
#define DISPLAY_H

#include <graphics.h>

#define MATRIX_WIDTH  128
#define MATRIX_HEIGHT 32
#define BIT_DEPTH     6

#define MIN_GREEN_BR  10
#define MIN_BLUE_BR   10
#define MIN_RED_BR    10

// #define FONT_FILE     "fonts/7x13.bdf"
#define FONT_FILE       "fonts/6x12.bdf"
#define FONT_HEIGHT     8     // Vertical positioning of font
#define FONT_WIDTH      5     // Calculations of right-aligned text

#define FONT_FILE_2     "fonts/7x13.bdf"
#define FONT_HEIGHT_2   9     // Vertical positioning of font
#define FONT_WIDTH_2    6     // Calculations of right-aligned text

using rgb_matrix::Color;

bool setupDisplay();
void shutdownDisplay();
int drawText(uint8_t, uint8_t, Color, const char *, rgb_matrix::Font* = NULL);
void drawRect(uint16_t, uint16_t, uint16_t, uint16_t, Color);
void drawIcon(int, int, int, int, const uint8_t *);
void displayClock(bool = false);

#endif