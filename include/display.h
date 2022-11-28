#ifndef DISPLAY_H
#define DISPLAY_H

#include <graphics.h>
#include <string.h>

#include "font.h"

bool setupDisplay(uint8_t configNum);
void shutdownDisplay();
void setBrightness(uint8_t brightness);
void drawRect(uint16_t, uint16_t, uint16_t, uint16_t, Color);
void drawIcon(int, int, int, int, const uint8_t *);
void displayClock(bool = false);

#endif