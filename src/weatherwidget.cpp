#include "weatherwidget.h"

#include <random>
#include <string>
#include <vector>

/*
Raindrop Palette Colors:

  Snow Palette Colors:
  b9b9b9 444444 626262 393939 484848
  cdcdcd 
  9e9e9e
  919191
  858585
  a0a0a0
  d6d6d6
  6f6f6f
  6d6d6d
  565656
*/

// Calculate offset into an RGB 8-bit raw image buffer
uint16_t imgIndex(uint8_t x, uint8_t y, uint8_t width) {
  return 3 * (y * width + x);
}

// Get a random color from a list of hex color
// strings and return a rgb_matrix color
rgb_matrix::Color getRandomColor(std::vector<string> colors)
{
  uint32_t colorNum;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distrib(0, colors.size()-1);

  auto idx = distrib(gen);
  sscanf(colors[idx].c_str(), "%x", &colorNum);

  auto red = (colorNum & 0xff0000) >> 0x10;
  auto green = (colorNum & 0x00ff00) >> 0x08;
  auto blue = (colorNum & 0x0000ff);

  return rgb_matrix::Color(red, green, blue);
}
