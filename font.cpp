#include <graphics.h>
#include <canvas.h>
#include <led-matrix.h>

#include "font.h"
#include "logger.h"

using rgb_matrix::DrawText;

GirderFont *defaultFont, *clockFont;

extern rgb_matrix::RGBMatrix *matrix;


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
      matrix->SetPixel(x, y-1, color.r, color.g, color.b);
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
      matrix->SetPixel(x+1, y-1, color.r, color.g, color.b);
      matrix->SetPixel(x+1, y-2, color.r, color.g, color.b);
      matrix->SetPixel(x+2, y-3, color.r, color.g, color.b);
      matrix->SetPixel(x+2, y-4, color.r, color.g, color.b);
      matrix->SetPixel(x+2, y-5, color.r, color.g, color.b);
      matrix->SetPixel(x+3, y-6, color.r, color.g, color.b);
      matrix->SetPixel(x+3, y-7, color.r, color.g, color.b);
      return;
    }
  }
  else if (strcmp(font->name, "clock") == 0)
  {
    if (glyph == '.') {
      matrix->SetPixel(x, y-1, color.r, color.g, color.b);
      return;
    }
    else if (glyph == ':') {
      matrix->SetPixel(x+1, y-2, color.r, color.g, color.b);
      matrix->SetPixel(x+1, y-3, color.r, color.g, color.b);
      matrix->SetPixel(x+1, y-5, color.r, color.g, color.b);
      matrix->SetPixel(x+1, y-6, color.r, color.g, color.b);
      return;
    }
    // else if (glyph == '/') {
    //   matrix->SetPixel(x, y-2, color.r, color.g, color.b);
    //   matrix->SetPixel(x, y-3, color.r, color.g, color.b);
    //   matrix->SetPixel(x+1, y-4, color.r, color.g, color.b);
    //   matrix->SetPixel(x+1, y-5, color.r, color.g, color.b);
    //   matrix->SetPixel(x+2, y-6, color.r, color.g, color.b);
    //   matrix->SetPixel(x+2, y-7, color.r, color.g, color.b);
    //   return;
    // }
  }

  // Call upstream library to render, and adjust position
  DrawText(matrix, *font->font, x - vGlyphOffset(glyph, font),
      y, color, NULL, buffer, 0);
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
void drawText(uint8_t x, uint8_t y, Color color, const char *text,
        GirderFont *font, bool vWidth)
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
  }
  else
  {
    // Library references bottom-left corner as origin (instead of top)
    //
    // Add a fixed y-offset to compensate, the benefit here is easily
    // tweaking the vertical position/placement
    //
    // Using font.height() resulted in too large of gap
    DrawText(matrix, *font->GetFont(), x, y+font->height,
        color, NULL, text, 0);
  }
}

// Render text with a custom formatting/profile
// Right now, this is very basic and just apply a fixed negative kerning offset
// But this is to be expanded to custom variable-width character rendering
void drawTextCustom(uint8_t x, uint8_t y, Color color, const char *text, GirderFont *font, bool vWidth)
{
  DrawText(matrix, *font->GetFont(), x, y+font->height,
      color, NULL, text, -1);
}