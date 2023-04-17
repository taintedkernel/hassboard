#include <graphics.h>
#include <canvas.h>
#include <led-matrix.h>

#include "font.h"
#include "logger.h"

using rgb_matrix::DrawText;

GirderFont *defaultFont, *clockFont;

extern rgb_matrix::RGBMatrix *matrix;


// Load our fonts
void GirderFont::LoadFont(fonts newFont)
{
  switch(newFont) {
  case FONT_DEFAULT:
    font->LoadFont(FONT_DEFAULT_FILE);
    strncpy(name, FONT_DEFAULT_NAME, 10);
    width = FONT_DEFAULT_WIDTH;
    height = FONT_DEFAULT_HEIGHT;
    break;
  case FONT_LARGE:
    font->LoadFont(FONT_LARGE_FILE);
    strncpy(name, FONT_LARGE_NAME, 10);
    width = FONT_LARGE_WIDTH;
    height = FONT_LARGE_HEIGHT;
    break;
  case FONT_SMALL:
    font->LoadFont(FONT_SMALL_FILE);
    strncpy(name, FONT_SMALL_NAME, 10);
    width = FONT_SMALL_WIDTH;
    height = FONT_SMALL_HEIGHT;
    break;
  case FONT_CLOCK:
    font->LoadFont(FONT_CLOCK_FILE);
    strncpy(name, FONT_CLOCK_NAME, 10);
    width = FONT_CLOCK_WIDTH;
    height = FONT_CLOCK_HEIGHT;
    break;
  default:
    _error("font %s unknown, not loading", newFont);
  }
}

// TODO: Store the information as data in files or defines, etc

// Non full-width glyphs are not left-justified, so
// this calculates the offset to allow for shifting
// the rendering location to compensate
int8_t vGlyphOffset(const char glyph, GirderFont *font)
{
  if (strcmp(font->name, FONT_DEFAULT_NAME) == 0)
  {
    // Numeric
    if (glyph == '0')      return 1;
    else if (glyph == '1') return 1;
    else if (glyph == '/') return 1;
    // Lowercase
    else if (glyph == 'i') return 1;
    else if (glyph == 'j') return 1;
    else if (glyph == 'l') return 1;
    // Uppercase
    else if (glyph == 'I') return 1;
    else if (glyph == 'J') return 1;
    else return 0;
  }
  else if (strcmp(font->name, FONT_SMALL_NAME) == 0)
  {
    // Numeric
    if (glyph == '4')      return 0;
    else if (glyph >= '0' && glyph <= '9') return 1;
    // Lowercase
    else if (glyph == 'm') return 0;
    else if (glyph == 'w') return 0;
    else if (glyph >= 'a' && glyph <= 'z') return 1;
    // Uppercase
    else if (glyph == 'A') return 0;
    else if (glyph == 'B') return 0;
    else if (glyph == 'J') return 0;
    else if (glyph == 'M') return 0;
    else if (glyph == 'O') return 0;
    else if (glyph == 'T') return 0;
    else if (glyph >= 'W' && glyph <= 'Y') return 0;
    else if (glyph >= 'A' && glyph <= 'Z') return 1;
  }

  return 0;
}

// Calculate the width of a glyph, with the spacing between them
uint8_t vGlyphWidth(const char glyph, GirderFont *font,
                    bool calcOnly = false)
{
  // The approach here is to set the width to the full font
  // width by default, and adjust (shrink) the width with an
  // offset for any glyphs that are not full-width
  // int8_t offset = 0;
  int8_t wOffset = 1;

  // Custom glyphs
  if (glyph == '.') return wOffset + 1;
  if (glyph == ':') return wOffset + 3;
  if (glyph == '/') return wOffset + 5;
  if (int(glyph) == 176) {
    if (calcOnly)
      return 0;
    else
      return wOffset + 2;
  }

  // Default font
  if (strcmp(font->name, FONT_DEFAULT_NAME) == 0)
  {
    // Numeric
    if (glyph == '0')      wOffset -= 1;
    else if (glyph == '1') wOffset -= 2;
    // Lowercase
    else if (glyph == 'i') wOffset -= 2;
    else if (glyph == 'j') wOffset -= 1;
    else if (glyph == 'l') wOffset -= 2;
    // Uppercase
    else if (glyph == 'I') wOffset -= 2;
    else if (glyph == 'J') wOffset -= 1;
    else {}
  }
  // Small font
  else if (strcmp(font->name, FONT_SMALL_NAME) == 0)
  {
    // Numeric
    if (glyph == '4')      wOffset -= 0;
    else if (glyph == '1') wOffset -= 2;
    else if (glyph >= '0' && glyph <= '9') wOffset -= 1;
    // Lowercase
    else if (glyph == 'i') wOffset -= 2;
    else if (glyph == 'l') wOffset -= 2;
    else if (glyph == 'm') wOffset -= 0;
    else if (glyph == 'w') wOffset -= 0;
    else if (glyph >= 'a' && glyph <= 'z') wOffset -= 1;
    // Uppercase
    else if (glyph == 'A') wOffset -= 0;
    else if (glyph == 'B') wOffset -= 0;
    else if (glyph == 'M') wOffset -= 0;
    else if (glyph == 'J') wOffset -= 0;
    else if (glyph == 'O') wOffset -= 0;
    else if (glyph == 'T') wOffset -= 0;
    else if (glyph >= 'W' && glyph <= 'Y') wOffset -= 0;
    else if (glyph == 'I') wOffset -= 2;
    else if (glyph >= 'A' && glyph <= 'Z') wOffset -= 1;
    // int8_t gOffset = vGlyphOffset(glyph, font);
    // _debug("vGlyphWidth(%c) = %d @ %d", glyph, font->width+ wOffset, gOffset);
  }

  return font->width + wOffset;
}

// Render a variable-width glyph to the matrix
void renderGlyph(const char glyph, uint8_t x, uint8_t y,
                 GirderFont *font, Color color)
{
  char buffer[2] = {glyph, '\0'};

  // _debug("rendering glyph: '%s'", buffer);
  if (strcmp(font->name, FONT_DEFAULT_NAME) == 0)
  {
    if (glyph == '.') {
      matrix->SetPixel(x, y - 1, color.r, color.g, color.b);
      return;
    }
    else if (glyph == ':')
    {
      matrix->SetPixel(x + 1, y - 2, color.r, color.g, color.b);
      matrix->SetPixel(x + 1, y - 3, color.r, color.g, color.b);
      matrix->SetPixel(x + 1, y - 5, color.r, color.g, color.b);
      matrix->SetPixel(x + 1, y - 6, color.r, color.g, color.b);
      return;
    }
    else if (glyph == '/')
    {
      matrix->SetPixel(x + 1, y - 1, color.r, color.g, color.b);
      matrix->SetPixel(x + 1, y - 2, color.r, color.g, color.b);
      matrix->SetPixel(x + 2, y - 3, color.r, color.g, color.b);
      matrix->SetPixel(x + 2, y - 4, color.r, color.g, color.b);
      matrix->SetPixel(x + 2, y - 5, color.r, color.g, color.b);
      matrix->SetPixel(x + 3, y - 6, color.r, color.g, color.b);
      matrix->SetPixel(x + 3, y - 7, color.r, color.g, color.b);
      return;
    }
    else if (int(glyph) == 176)
    {
      matrix->SetPixel(x, y - 6, color.r, color.g, color.b);
      matrix->SetPixel(x, y - 7, color.r, color.g, color.b);
      matrix->SetPixel(x + 1, y - 6, color.r, color.g, color.b);
      matrix->SetPixel(x + 1, y - 7, color.r, color.g, color.b);
      return;
    }
  }
  else if (strcmp(font->name, FONT_SMALL_NAME) == 0)
  {
    if (glyph == '.') {
      matrix->SetPixel(x, y - 1, color.r, color.g, color.b);
      return;
    }
    else if (glyph == ':')
    {
      matrix->SetPixel(x + 1, y - 2, color.r, color.g, color.b);
      matrix->SetPixel(x + 1, y - 3, color.r, color.g, color.b);
      matrix->SetPixel(x + 1, y - 5, color.r, color.g, color.b);
      matrix->SetPixel(x + 1, y - 6, color.r, color.g, color.b);
      return;
    }
    else if (glyph == '/') {
      matrix->SetPixel(x + 1, y - 1, color.r, color.g, color.b);
      matrix->SetPixel(x + 1, y - 2, color.r, color.g, color.b);
      matrix->SetPixel(x + 2, y - 3, color.r, color.g, color.b);
      matrix->SetPixel(x + 2, y - 4, color.r, color.g, color.b);
      matrix->SetPixel(x + 3, y - 5, color.r, color.g, color.b);
      matrix->SetPixel(x + 3, y - 6, color.r, color.g, color.b);
      return;
    }
  }

  // Call upstream library to render, and adjust position
  DrawText(matrix, *font->font, x - vGlyphOffset(glyph, font),
           y, color, NULL, buffer, font->kerning);
}

// Calculate what the actual rendered length of a string will be
uint16_t textRenderLength(const char *text, GirderFont *font)
{
  uint16_t length = 0;

  for (size_t i = 0; i < strlen(text); i++) {
    length += vGlyphWidth(text[i], font, true);
  }

  // Remove trailing space
  return length - 1;
}

// Render text in color at (x,y)
void drawText(uint8_t x, uint8_t y, Color color, const char *text,
              GirderFont *font, bool vWidth, bool debug)
{
  if (font == NULL) {
    font = defaultFont;
  }
  // _debug("drawText x,y,msg: %d,%d,\"%s\"", x, y, text);

  if (strchr(text, '.') != NULL && strlen(text) != 3) {
    _warn("unable to autoparse text for custom rendering, using default");
  }

  if (vWidth)
  {
    uint16_t xStart = x;
    uint8_t textLen = strlen(text);
    // uint16_t renderLen = textRenderLength(text, font);
    // _debug("vstring='%s' len=%d renderLen=%d", text, textLen, renderLen);

    char glyph;
    for (uint8_t idx = 0; idx < textLen; idx++)
    {
      glyph = *(text + idx);
      // _debug("glyph: '%c'", glyph);
      renderGlyph(glyph, xStart, y + font->height, font, color);
      if (debug)
      {
        int8_t gOffset = vGlyphOffset(glyph, font);
        matrix->SetPixel(xStart, y+font->height, 192, 0, 0);
        if (gOffset != 0) {
          matrix->SetPixel(xStart + vGlyphOffset(glyph, font), y+font->height, 0, 192, 0);
        }
      }
      xStart += vGlyphWidth(glyph, font);
      if (debug) {
        matrix->SetPixel(xStart, y+font->height, 0, 0, 192);
        // _debug("new xStart: %d", xStart);
      }
    }
  }
  else
  {
    // Library references bottom-left corner as origin
    // (instead of top)
    //
    // Add a fixed y-offset to compensate, the benefit here
    // is easily tweaking the vertical position/placement
    //
    // Using font.height() resulted in too large of gap
    DrawText(matrix, *font->GetFont(), x, y + font->height,
             color, NULL, text, font->kerning);
  }
}