#include "display.h"
#include "widget.h"
#include "logger.h"
#include "icons.h"

#include <cstring>
#include <algorithm>

#include <sys/stat.h>
#include <stdio.h>

#include <led-matrix.h>
#include <png++/png.hpp>
#include <Magick++.h>
#include <magick/image.h>


uint16_t cloudsLocal[ICON_SZ], cloudsSunLocal[ICON_SZ];
uint16_t sunLocal[ICON_SZ], cloudsShowersLocal[ICON_SZ];

uint8_t refreshDelay = 5, refreshActiveDelay = 10;

extern uint8_t brightness;
extern uint8_t boldBrightnessIncrease;
extern uint32_t cycle;
extern rgb_matrix::RGBMatrix *matrix;
extern rgb_matrix::Color colorDarkGrey, colorBlack;
extern rgb_matrix::Font *defaultFont;


// Convert received temperature to integer
void tempIntHelper(char *textData, char *payload) {
  snprintf(textData, WIDGET_TEXT_LEN, "%d", atoi(payload));
}

// Convert received temperature to F, then integer
void tempC2FHelper(char *textData, char *payload)
{
  snprintf(textData, WIDGET_TEXT_LEN, "%d",
    int(atof(payload) * 9 / 5 + 32));
}

// Limit string length of displayed value
// (if 10 or greater, just show integer value)
void floatStrLen(char *textData, char *payload)
{
  if (atof(payload) >= 10.0) {
    snprintf(textData, WIDGET_TEXT_LEN, "%d", int(atof(payload)));
  } else {
    snprintf(textData, WIDGET_TEXT_LEN, "%1.1f", atof(payload));
  }
}

// Translate weather condition to matching icon, return filename
const char* weatherIconHelper(char *condition)
{
  if (strcmp(condition, "sunny") == 0) {
    return("icons/sun-1.0.png");
  }
  else if (strcmp(condition, "partlycloudy") == 0) {
    return("icons/clouds_sun-1.0.png");
  }
  else if (strcmp(condition, "cloudy") == 0) {
    return("icons/clouds-1.0.png");
  }
  else if (strcmp(condition, "rainy") == 0) {
    return("icons/clouds_showers-1.0.png");
  }
  else {
    return(condition);
  }
}

// Debugging helper
void DashboardWidget::_logName() {
  _debug("widget %s:", this->name);
}

// Get custom widget reset bounding box size
uint8_t DashboardWidget::_getWidth() {
  return this->width;
}
uint8_t DashboardWidget::_getHeight() {
    return this->height;
}
uint16_t DashboardWidget::_getIconSize() {
  return (this->iconWidth * this->iconHeight);
}
Color DashboardWidget::color2RGB(uint8_t color[3]) {
  return Color(color[0], color[1], color[2]);
}
void DashboardWidget::_setText(char *text) {
  _debug("widget %s: setting text to: %s", this->name, text);
  strncpy(this->textData, text, WIDGET_TEXT_LEN);
}

/* // Calculate color from hex value + brightness
Color DashboardWidget::colorBright(uint16_t value, int br) {
  unsigned r = (value & 0xF800) >> 8;       // rrrrr... ........ -> rrrrr000
  unsigned g = (value & 0x07E0) >> 3;       // .....ggg ggg..... -> gggggg00
  unsigned b = (value & 0x1F) << 3;         // ............bbbbb -> bbbbb000

  return DashboardWidget::colorBright(r, g, b, br);
}
    r & 248 (0b 1111 1000) << 8
      +
    g & 252 (0b 1111 1100) << 3
      +
    b >> 3
*/

void DashboardWidget::color565_2RGB(uint16_t value, uint8_t *rgb) {
  uint8_t r = (value & 0xF800) >> 8;       // rrrrr... ........ -> rrrrr000
  uint8_t g = (value & 0x07E0) >> 3;       // .....ggg ggg..... -> gggggg00
  uint8_t b = (value & 0x1F) << 3;         // ............bbbbb -> bbbbb000

  rgb[0] = r;
  rgb[1] = g;
  rgb[2] = b;
}

DashboardWidget::DashboardWidget(const char *name)
{
  this->iconBrightness = brightness;
  this->textBrightness = brightness;
  strncpy(this->iconData, "", WIDGET_DATA_LEN);
  strncpy(this->name, name, WIDGET_NAME_LEN);
  this->textFont = defaultFont;
}

/*
  ----==== [ Configuration Functions ] ====----
*/
char* DashboardWidget::getText() {
  return this->textData;
}

void DashboardWidget::setActive(bool active) {
  this->active = active;
}

void DashboardWidget::setDebug(bool debug) {
  this->debug = debug;
}

void DashboardWidget::setOrigin(uint8_t x, uint8_t y) {
  this->x = x;
  this->y = y;
}

// Set bounds for widget rendering box
void DashboardWidget::setBounds(uint8_t w, uint8_t h) {
  this->width = w;
  this->height = h;
}

void DashboardWidget::setSize(widgetSizeType wsize)
{
  switch(wsize) {
    case WIDGET_SMALL:
      this->size = wsize;
      this->width = WIDGET_WIDTH_SMALL;
      this->height = WIDGET_HEIGHT_SMALL;
      break;
    case WIDGET_LARGE:
      this->size = wsize;
      this->width = WIDGET_WIDTH_LARGE;
      this->height = WIDGET_HEIGHT_LARGE;
      break;
    case WIDGET_LONG:
      this->size = wsize;
      this->width = WIDGET_WIDTH_LONG;
      this->height = WIDGET_HEIGHT_LONG;
      break;
    default:
      _error("unknown widget size %s, not configuring");
  }
}

void DashboardWidget::setCustomTextRender(int (render)TEXT_RENDER_SIG)
{
  this->customTextRender = render;
}

/* ----==== [ Text Functions ] ====---- */

// Set (x,y) coordinates, color and alignment for widget text
//
// Note: When right-alignment is set, the X values sets the
//   right-most coordinate (eg: bounding box) location
//
// With alignment == none, the X acts "as expected" and
//   sets the left-most coordinate
//
// We also set y = 0 here, as the standard small widget
//   fits on one "line"; eg: the Y coorinates for the text
//   and icon match up
//
// TODO: We need to configure minimum bounds here
// But we need to know font size to do so
void DashboardWidget::autoTextConfig(Color color, textAlignType align)
{
  switch(this->width) {
    case WIDGET_WIDTH_SMALL:
    case WIDGET_WIDTH_LARGE:
      this->setCustomTextConfig(this->width, 0, color, align);
      break;
    default:
      _error("unknown widget size %s, not configuring");
      return;
  }
}

// Set a manually-provided text configuration with locations
// in the widget, text color and font information
void DashboardWidget::setCustomTextConfig(uint8_t x, uint8_t y,
  Color color, textAlignType align, rgb_matrix::Font *textFont,
  uint8_t fontWidth, uint8_t fontHeight)
{
  this->textX = x;
  this->textY = y;
  this->textColor = color;
  this->textAlign = align;
  this->textInit = true;

  // Use default font if not provided
  if (textFont == NULL) {
    this->textFont = defaultFont;
    this->textFontWidth = FONT_WIDTH;
    this->textFontHeight = FONT_HEIGHT;
  } else {
    this->textFont = textFont;
    this->textFontWidth = fontWidth;
    this->textFontHeight = fontHeight;
  }

  // if (clearText) {
  //   strncpy(this->textData, "", WIDGET_TEXT_LEN);
  // }
}

// Update text and set temporary bold brightness
void DashboardWidget::updateText(char *text, bool brighten)
{
  // If new text is not different, don't update
  if (strncmp(text, this->textData, WIDGET_TEXT_LEN) == 0)
    return;

  // Abbreviate zero/null floating-point values
  if (strcmp(text, "0.0") == 0)
    text = (char *) "0";

  _debug("widget %s: updating to '%s' from old text: '%s', "
    "bright until cycle %d", this->name, text, this->textData,
    cycle + refreshDelay);

  this->_setText(text);
  if (brighten) {
    this->resetTime = clock() + refreshDelay * CLOCKS_PER_SEC;
    this->tempAdjustBrightness(boldBrightnessIncrease, BRIGHT_TEXT);
  }

  this->render();
}

// Update text with helper, then update same as above
void DashboardWidget::updateText(char *data, void(helperFunc)(char*, char*), bool brighten)
{
  char buffer[WIDGET_TEXT_LEN];

  // Dual-copy used here to prevent dupication of logic
  helperFunc(buffer, data);
  this->updateText(buffer, brighten);
}

/* ----==== [ Icon Functions ] ====---- */

// Set (x,y) origin coordinates for widget icon
void DashboardWidget::setIconConfig(uint8_t x, uint8_t y)
{
  this->iconX = x;
  this->iconY = y;
}

// Set widget icon and size (old format)
// This invocation will convert an existing 16-bit 565-encoded RGB
// image into the new format needed for the library to render natively
void DashboardWidget::setIconImage(uint8_t w, uint8_t h, const uint16_t *img)
{
  // Allocate storage for new image format
  uint8_t *rgb = new uint8_t[3];
  uint8_t *newImg = (uint8_t *)malloc(w * h * 3 * sizeof(uint8_t));

  if (newImg == NULL || rgb == NULL) {
    _error("unable to allocate buffer for icon, aborting");
    return;
  }

  // Read old image, convert and write to new buffer
  for (int src=0, dst=0; src < w*h;) {
      color565_2RGB(img[src++], rgb);
      newImg[dst++] = rgb[0];
      newImg[dst++] = rgb[1];
      newImg[dst++] = rgb[2];
  }

  this->setIconImage(w, h, newImg);
}

// Set widget icon and size
void DashboardWidget::setIconImage(uint8_t w, uint8_t h, const uint8_t *img)
{
  this->iconWidth = w;
  this->iconHeight = h;
  this->iconImage = img;
  this->iconInit = true;
}

// Set widget icon and size from a PNG image
void DashboardWidget::setIconImage(uint8_t w, uint8_t h, const char* iconFile)
{
  png::image<png::rgb_pixel> image;

  struct stat buffer;
  if (stat(iconFile, &buffer) == 0) {
    image.read(iconFile);
  } else {
    _error("image %s not found, using default", iconFile);
    image.read("icons/clouds_sun-new.png");
  }

  if (w > image.get_width() || h > image.get_height()) {
    _error("setIconImage() dimension arguments larger then image size, aborting");
    return;
  }
  if (w != image.get_width() || h != image.get_height()) {
    _warn("setIconImage() dimensions smaller then image size, rendering may not match");
  }

  // Allocate storage for new image format
  uint8_t *newImg = (uint8_t *)malloc(w * h * 3 * sizeof(uint8_t));
  uint16_t idx = 0;

  if (newImg == NULL) {
    _error("unable to allocate buffer for icon, aborting");
    return;
  }

  // Convert PNG pixel data to a RGB pixel array
  for (size_t y = 0; y < h; y++) {
    for (size_t x = 0; x < w; x++) {
      png::rgb_pixel pixel = image.get_pixel(x, y);
      newImg[idx++] = pixel.red;
      newImg[idx++] = pixel.green;
      newImg[idx++] = pixel.blue;
    }
  }

  this->setIconImage(w, h, newImg);
}

// Call helper to determine icon, then render
// Note: No brightness logic similar to updateText()
void DashboardWidget::updateIcon(char *data, const char* (helperFunc)(char*))
{
  if (data != NULL) {
    strncpy(this->iconData, data, WIDGET_DATA_LEN);
  }
  _debug("setting icon to %s", this->iconData);

  this->setIconImage(this->iconWidth, this->iconHeight, helperFunc(this->iconData));
  this->render();
}

/*
  ----==== [ Rendering Functions ] ====----
*/

// TODO: Render an text-specific black clearing box
int DashboardWidget::renderText()
{
  int16_t offset;
  u_int16_t textLength;

  if (!this->textInit) {
    _error("renderText(%s) called without config, aborting", this->name);
    return 0;
  }

  if (!this->active)
    return 0;

  textLength = strlen(this->textData);
  if (textLength > WIDGET_TEXT_LEN)
  {
    textLength = WIDGET_TEXT_LEN;
    this->textData[textLength-1] = '\0';
  }

  if (this->textAlign == ALIGN_RIGHT) {
    offset = this->textX - (textLength * FONT_WIDTH + 1);
  } else if (this->textAlign == ALIGN_CENTER) {
    offset = (this->textX / 2) - textLength * FONT_WIDTH / 2;
  } else {
    _error("unknown text alignment %d, not rendering", this->textAlign);
    return 0;
  }
  if (offset < 0) {
    _warn("text length exceeds limits, may be truncated");
    offset = 0;
  }

  Color tColor = Color(this->textColor);
  tColor.r = std::min(tColor.r + this->textTempBrightness, 255);
  tColor.g = std::min(tColor.g + this->textTempBrightness, 255);
  tColor.b = std::min(tColor.b + this->textTempBrightness, 255);

  if (this->debug)
  {
    _debug("renderText(%s) = %s", this->name, this->textData);
    _debug("- x,textX,len,offset = %d, %d, %d, %d",
      this->x, this->textX, textLength, offset);
    // _debug("- color,newColor = %d,%d,%d %d,%d,%d", this->textColor.r,
    // this->textColor.g, this->textColor.b, tColor.r, tColor.g, tColor.b);
  }

  if (this->customTextRender)
    return this->customTextRender(this->x + offset, this->y +
        this->textY, tColor, this->textData, this->textFont,
        this->textFontWidth, this->textFontHeight);
  else
    return drawText(this->x + offset, this->y + this->textY, tColor,
        this->textData, this->textFont);
}

// TODO: Render an icon-specific black clearing box
void DashboardWidget::renderIcon()
{
  if (!this->iconInit || this->iconImage == NULL) {
    _error("renderIcon(%s) called without config and/or image, aborting", this->name);
    return;
  }

  if (!this->active)
    return;

  drawIcon(this->x + this->iconX, this->y + this->iconY, this->iconWidth, this->iconHeight, this->iconImage);
}

// Render our widget
// TODO: Break clear widget logic into text and icon-specific
// sections and move to those rendering functions
void DashboardWidget::render()
{
  if (!this->active)
    return;

  // Clear widget
  // _debug("clearing widget %s", this->name);
  this->clear();

  // Render widget assets
  this->renderIcon();
  this->renderText();

  // Debugging bounding box for widget
  // Calculated from icon height & fixed width
  if (this->debug)
  {
    matrix->SetPixel(this->x, this->y, 255,0,0);
    matrix->SetPixel(this->x+this->width-1, this->y, 0,255,0);
    matrix->SetPixel(this->x, this->y+this->height-1, 0,0,255);
    matrix->SetPixel(this->x+this->width-1, this->y+this->height-1, 255,255,255);
  }
}

// Clear the rendering bounds of our widget
void DashboardWidget::clear(bool force)
{
  // If we are inactive and no force-clear set then return
  if (!this->active && !force)
    return;

  if (this->debug)
    drawRect(this->x, this->y, this->width, this->height, colorDarkGrey);
  else
    drawRect(this->x, this->y, this->width, this->height, colorBlack);
}

/*
  ----==== [ Color/brightness Functions ] ====----
*/

// Resets brightness to current global value
void DashboardWidget::resetBrightness(brightType bType = BRIGHT_TEXT)
{
  if (bType != BRIGHT_TEXT)
    this->iconBrightness = brightness;
  if (bType != BRIGHT_ICON)
    this->textBrightness = brightness;
}

/*
  Recalculate brightness based upon (a presumed) new global value
  and if different from current, redraw text and/or icon.

  This logic is different from other brightness functions in that
  we also render assets.  The intent is to use this solely for
  updating a widget based upon some new global brightness.

  Some optimizations taken here: we do not clear/blank the widget,
  we are assuming the text & icons have not changed.  If the internal
  data has changed, but not rendered it will show a garbled display.
*/
void DashboardWidget::updateBrightness()
{
  if (this->iconBrightness != brightness + this->iconTempBrightness) {
    this->iconBrightness = brightness + this->iconTempBrightness;
    this->renderIcon();
  }
  if (this->textBrightness != brightness + this->textTempBrightness) {
    this->textBrightness = brightness + this->textTempBrightness;
    this->renderText();
  }
}

// Checks if temporary brightness duration has elapsed and reset if necessary
void DashboardWidget::checkResetBrightness()
{
  if (this->resetTime > 0 && clock() >= this->resetTime)
  {
    this->_logName();
    _log("- resetting brightness");
    this->resetTime = 0;
    this->resetBrightness(BRIGHT_BOTH);
    this->iconTempBrightness = this->textTempBrightness = 0;
    this->render();
  }
}

// Sets brightness to current global value + some custom offset
void DashboardWidget::tempAdjustBrightness(uint8_t tempBright, brightType bType = BRIGHT_TEXT)
{
  if (bType != BRIGHT_TEXT) {
    this->iconTempBrightness = tempBright;
    this->iconBrightness = brightness + tempBright;
  }
  if (bType != BRIGHT_ICON) {
    this->textTempBrightness = tempBright;
    this->textBrightness = brightness + tempBright;
  }
}

// Check if temporary active duration has elapsed and reset if necessary
void DashboardWidget::checkResetActive()
{
  if (this->resetActiveTime > 0 && clock() >= this->resetActiveTime)
  {
    this->_logName();
    _log("- resetting active to: %d", !(this->active));
    this->resetActiveTime = 0;
    this->setActive(!(this->active));
    this->clear();
    this->render();
  }
}

// Set the time to reset the active state
void DashboardWidget::setResetActiveTime(clock_t time)
{
  this->resetActiveTime = time;
}
