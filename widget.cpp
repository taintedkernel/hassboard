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
  else if (strcmp(condition, "fog") == 0) {
    return("icons/fog-1.0.png");
  }
  else if (strcmp(condition, "clear-night") == 0) {
    return("icons/moon-1.0.png");
  }
  else {
    return(condition);
  }
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
*/

// Convert a 565-encoded color to individual RGB values
void color565_2RGB(uint16_t value, uint8_t *rgb) {
  uint8_t r = (value & 0xF800) >> 8;       // rrrrr... ........ -> rrrrr000
  uint8_t g = (value & 0x07E0) >> 3;       // .....ggg ggg..... -> gggggg00
  uint8_t b = (value & 0x1F) << 3;         // ............bbbbb -> bbbbb000

  rgb[0] = r;
  rgb[1] = g;
  rgb[2] = b;
}

/*** DashboardWidget class ***/

// Constructor
DashboardWidget::DashboardWidget(const char *name)
{
  this->iconBrightness = brightness;
  this->textBrightness = brightness;
  strncpy(this->iconData, "", WIDGET_DATA_LEN);
  strncpy(this->name, name, WIDGET_NAME_LEN);
  this->textFont = defaultFont;
}

// Debugging helper
void DashboardWidget::_logName() {
  _debug("widget %s:", this->name);
}

// Get widget width
uint8_t DashboardWidget::_getWidth() {
  return this->width;
}

// Get widget height
uint8_t DashboardWidget::_getHeight() {
    return this->height;
}

// Get widget icon size
uint16_t DashboardWidget::_getIconSize() {
  return (this->iconWidth * this->iconHeight);
}

/*
  ----==== [ Configuration Functions ] ====----
*/
// Set/clear debugging flag for widget
void DashboardWidget::setDebug(bool debug) {
  this->debug = debug;
}

// Set/clear active flag for widget
void DashboardWidget::setActive(bool active) {
  this->active = active;
}

// Set widget origin
void DashboardWidget::setOrigin(uint8_t x, uint8_t y) {
  this->widgetX = x;
  this->widgetY = y;
}

// Set size of widget
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

// Set bounds for widget rendering box
// TODO: This should be merged with setSize() as it overwrites the values
void DashboardWidget::setBounds(uint8_t w, uint8_t h) {
  this->width = w;
  this->height = h;
}

/* ----==== [ Text Functions ] ====---- */

// Get widget text
char* DashboardWidget::getText() {
  return this->textData;
}

// Set widget text
void DashboardWidget::setText(char *text)
{
  _debug("widget %s: setting text to: %s", this->name, text);
  strncpy(this->textData, text, WIDGET_TEXT_LEN);
  this->textScrollStart = 0;
}

// Set widget text length
void DashboardWidget::setVisibleSize(u_int16_t length)
{
  if (length > WIDGET_TEXT_LEN) {
    _error("setVisibleSize(%d) on widget %s beyond maximum, using max as value",
        length, this->name);
    length = WIDGET_TEXT_LEN;
  }

  this->textVisibleSize = length;
}

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
void DashboardWidget::setCustomTextConfig(uint8_t textX, uint8_t textY,
  Color color, textAlignType align, rgb_matrix::Font *textFont,
  uint8_t fontWidth, uint8_t fontHeight)
{
  this->textX = textX;
  this->textY = textY;
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

// Set a custom text-rendering function
void DashboardWidget::setCustomTextRender(int (render)TEXT_RENDER_SIG)
{
  this->customTextRender = render;
}

// Set an alert level and color (currently on text)
// Note: This currently only supports upper-bounds levels
void DashboardWidget::setAlertLevel(float alertLevel, rgb_matrix::Color alertColor)
{
  this->textAlertLevel = alertLevel;
  this->textAlertColor = alertColor;
}

// Set/clear scroll setting for widget
void DashboardWidget::setTextScollable(bool scrollable)
{
  this->textScrollable = scrollable;
}

// Update text and set temporary bold brightness
void DashboardWidget::updateText(char *text, bool brighten)
{
  // Abbreviate zero/null floating-point values
  if (strcmp(text, "0.0") == 0)
    text = (char *) "--";

  // If new text is not different, don't update
  if (strncmp(text, this->textData, WIDGET_TEXT_LEN) == 0)
    return;

  _debug("widget %s: updating to '%s' from old text: '%s', "
    "bright until cycle %d", this->name, text, this->textData,
    cycle + refreshDelay);

  this->setText(text);
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
void DashboardWidget::setIconOrigin(uint8_t x, uint8_t y)
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
    image.read("icons/sun-1.0.png");
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

// Clear the rendering bounds of our widget
void DashboardWidget::clear(bool force)
{
  // If we are inactive and no force-clear set then return
  if (!this->active && !force)
    return;

  if (this->debug)
    drawRect(this->widgetX, this->widgetY, this->width, this->height, colorDarkGrey);
  else
    drawRect(this->widgetX, this->widgetY, this->width, this->height, colorBlack);
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
    matrix->SetPixel(this->widgetX, this->widgetY, 255,0,0);
    matrix->SetPixel(this->widgetX+this->width-1, this->widgetY, 0,255,0);
    matrix->SetPixel(this->widgetX, this->widgetY+this->height-1, 0,0,255);
    matrix->SetPixel(this->widgetX+this->width-1, this->widgetY+this->height-1, 255,255,255);
  }
}

// TODO: Render an text-specific black clearing box
int DashboardWidget::renderText()
{
  Color tColor;
  int16_t offset;
  u_int16_t textDataLength, textRenderLength;

  // Verify initialization & active status
  if (!this->textInit) {
    _error("renderText(%s) called without config, aborting", this->name);
    return 0;
  }
  else if (!this->active) {
    return 0;
  }

  // Calculate string length
  // Use an offset to allow for scrolling of longer content
  // Trim string to visible display length (if set)
  textDataLength = strlen(this->textData);
  if (textDataLength == 0)
    return 0;
/*   if (this->debug) {
    _debug("text=%s, length=%d", this->textData, textDataLength);
  } */
  textRenderLength = textDataLength;
  if (this->textVisibleSize > 0 &&
        textDataLength > this->textVisibleSize)
  {
    textRenderLength = this->textVisibleSize;
    if (!(this->textScrollable))
      _warn("text length exceeds limits, truncating");
  }

/*   if (this->debug) {
    _debug("visibleSize=%d, renderLength=%d, scrollStart=%d",
        textVisibleSize, textRenderLength, this->textScrollStart);
  } */

  // Copy result into separate buffer for rendering
  char *textBuffer = new char(textDataLength+textRenderLength);
/*   if (this->debug) {
    _debug("strncpy(textBuffer, this->textData=%d + (this->textScrollStart %% textDataLength)=%d",
        this->textData, (this->textScrollStart % textDataLength));
    _debug("  std::min(int(this->textVisibleSize=%d), textDataLength-textScrollStart=%d));",
        this->textVisibleSize, textDataLength-textScrollStart);
  } */
  char *textSrc = this->textData +
                      (this->textScrollStart % textDataLength);
  int16_t textLen = std::min(int(this->textVisibleSize),
                        textDataLength-textScrollStart);
  strncpy(textBuffer, textSrc, textLen);
  // if (this->debug) {
  //   _debug("textBuffer='%s', len=%d", textBuffer, strlen(textBuffer));
  // }
  int16_t clearCount = this->textScrollStart-textDataLength+textVisibleSize;
  /* if (this->textScrollStart > textDataLength-textRenderLength)
  {
    if (this->debug)
    {
      _debug("memset(textBuffer+textVisibleSize-clearCount=%d, ' ', this->textScrollStart-textDataLength+textVisibleSize=%d);",
      textVisibleSize-clearCount, clearCount);
    }

    memset(textBuffer+textVisibleSize-clearCount, ' ', clearCount);
  } */
  textBuffer[textVisibleSize-std::max(0, int(clearCount))] = '\0';
/*   if (this->debug) {
    _debug("textBuffer='%s', len=%d", textBuffer, strlen(textBuffer));
  }
  if (this->textScrollStart>=4) {
    _debug("foo!");
  } */

  // Calculate positioning of text based on alignment
  if (this->textAlign == ALIGN_RIGHT) {
    offset = this->textX - (textDataLength * this->textFontWidth) - 2;
  } else if (this->textAlign == ALIGN_CENTER) {
    offset = (this->textX / 2) - (textDataLength * this->textFontWidth / 2);
  } else {
    _error("unknown text alignment %d, not rendering", this->textAlign);
    return 0;
  }
  if (offset < 0) {
    if (!(this->textScrollable)) {
      _warn("text length during alignment exceeds limits, may be truncated");
    }
    offset = 0;
  }

  // Calculate color, apply upper bound on channels
  // Note: This currently only supports upper-bounds levels
  if (this->textAlertLevel > 0.001 &&
        atof(textBuffer) > this->textAlertLevel) {
    tColor = Color(this->textAlertColor);
  }
  else {
    tColor = Color(this->textColor);
  }
  tColor.r = std::min(tColor.r + this->textTempBrightness, 255);
  tColor.g = std::min(tColor.g + this->textTempBrightness, 255);
  tColor.b = std::min(tColor.b + this->textTempBrightness, 255);

  if (this->debug)
  {
    _debug("renderText(%s) = %s", this->name, textBuffer);
    _debug("- x,textX,len,offset = %d, %d, %d, %d",
      this->widgetX, this->textX, textDataLength, offset);
    // _debug("- color,newColor = %d,%d,%d %d,%d,%d", this->textColor.r,
    // this->textColor.g, this->textColor.b, tColor.r, tColor.g, tColor.b);
  }

  // Call the custom text renderer, if set
  int render;
  if (this->customTextRender)
    render = this->customTextRender(this->widgetX + offset, this->widgetY +
        this->textY, tColor, textBuffer, this->textFont,
        this->textFontWidth, this->textFontHeight);
  else
    render = drawText(this->widgetX + offset, this->widgetY + this->textY, tColor,
        textBuffer, this->textFont);

  free(textBuffer);
  return render;
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

  drawIcon(this->widgetX + this->iconX, this->widgetY + this->iconY, this->iconWidth,
      this->iconHeight, this->iconImage);
}

/*
  ----==== [ Color/Brightness Functions ] ====----
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

// Scroll the text one character position
void DashboardWidget::scrollText()
{
  if (this->textVisibleSize == 0 || !this->textScrollable)
    return;

  this->textScrollStart = (this->textScrollStart + 1) %
      strlen(this->textData);
  this->render();
}


/*

3 * 2 + 1 = 7

length = 3

 0123456789
          ---

7: 789
8: 89_
9: 9__

6: 6789
7: 789_
8: 89__
9: 9___

 12345
    ---

0: 12 2
1: 23 2
2: 34 2
3: 45 2
4: 5_ 1
5: 12 2

DL-VS 5-3=2

       cpylen
0: 123  3 5 : DL-SS 5-0=5
1: 234  3 4 : 5-1=4
2: 345  3 3 : 5-2=3
3: 45_  2 2 : 5-3=2
4: 5__  1 1 : 5-4=1
5: 123  3 0 : 5-5=0

DL=5
VS=3

       setlen
0: 123 -2  SS-VS+1  0-3+1 = -2
1: 234 -1  1-3+1 = -1
2: 345  0  2-3+1 =  0
3: 45_  1  3-3+1 =  1
4: 5__  2  4-3+1 =  2
5: 123

DL=5,VS=3

0: 123 -2  0-5+3 = -2
1: 234 -1  1-5+3 = -1
2: 345  0  2-5+3 =  0
3: 45_  1  3-5+3 =  1
4: 5__  2  
5: 123


VS -
10 -

DL-VS = 31-10 = 21
SS-DL+VS?
              
20: _123456789  -1  20-31+10
          1234
25: 567890-_-_   4  25-31+10  10-4=6
26: 67890_-_-_   5  26-31+10  10-5=5
27: 7890U_-_-_   6  27-31+10  10-6=4
28: 890UU_-_-_




*/