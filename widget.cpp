#include "display.h"
#include "widget.h"
#include "logger.h"

#include <led-matrix.h>
#include <cstring>
#include <stdio.h>


uint16_t cloudsLocal[ICON_SZ], cloudsSunLocal[ICON_SZ];
uint16_t sunLocal[ICON_SZ], cloudsShowersLocal[ICON_SZ];

uint8_t refreshDelay = 30;

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

// Determine which icon to render based upon weather conditions
const uint16_t* weatherIconHelper(char *forecast)
{
  return NULL;
/*   uint16_t *icon;

  if (strcmp(forecast, "sunny") == 0) {
    if (daytime)
      icon = sunLocal;
    else
      icon = moon;
  }
  else if (strcmp(forecast, "clear-night") == 0) {
    icon = moon;
  }
  else if (strcmp(forecast, "partlycloudy") == 0) {
    if (daytime)
      icon = cloudsSunLocal;
    else
      icon = clouds_moon;
  }
  else if (strcmp(forecast, "Cloudy") == 0)
    icon = cloudsLocal;
  // else if (strcmp(forecast, "fog") == 0)
  //   icon = mist;
  else if (strcmp(forecast, "rainy") == 0)
    icon = cloudsShowersLocal;
  // Is this condition supported?
  // else if (strcmp(forecast, "sunshower") == 0)
  //   icon = clouds_showers_sun;
  // else if (strcmp(forecast, "stormy") == 0)
  //   icon = cloud_lightning;
  else if (strcmp(forecast, "snowy") == 0)
    icon = clouds_snow;
  else
    icon = cloudsLocal;

  return icon; */
}

// Debugging helper
void DashboardWidget::_logName() {
  _debug("widget %s:", this->name);
}

// Get custom widget reset bounding box size
uint8_t DashboardWidget::_getWidth() {
  // return ((this->resetBoundX > 0) ? this->resetBoundX : widgetDefaultWidth);
  return this->width;
}
uint8_t DashboardWidget::_getHeight() {
  // return ((this->resetBoundY > 0) ? this->resetBoundY : textDefaultHeight);
    return this->height;
}
uint16_t DashboardWidget::_getIconSize() {
  return (this->iconWidth * this->iconHeight);
}
Color DashboardWidget::color2RGB(uint8_t color[3]) {
  return Color(color[0], color[1], color[2]);
}
void DashboardWidget::_setText(char *text) {
  this->_logName();
  _debug("- setting text to: %s", text);
  strncpy(this->textData, text, WIDGET_TEXT_LEN);
}

// Calculate color from rgb parameters + brightness
Color DashboardWidget::colorBright(uint8_t r,
    uint8_t g, uint8_t b, int br)
{
  // uint8_t nr = int(max(r * br/100, colorRedMin));
  // uint8_t ng = int(max(g * br/100, colorGreenMin));
  // uint8_t nb = int(max(b * br/100, colorBlueMin));
  uint8_t nr = int(r * br/100);
  uint8_t ng = int(g * br/100);
  uint8_t nb = int(b * br/100);

  // if (nr < colorRedMin)
  //   return dcBlack;
  // if (ng < colorGreenMin)
  //   return dcBlack;
  // if (nb < colorBlueMin)
  //   return dcBlack;

  // return display.color565(nr, ng, nb);
  return Color(nr, ng, nb);
}

// Calculate color from rgb char array + brightness
Color DashboardWidget::colorBright(uint8_t rgb[], int br) {
  return DashboardWidget::colorBright(rgb[0], rgb[1], rgb[2], br);
}

// Calculate color from hex value + brightness
Color DashboardWidget::colorBright(uint16_t value, int br) {
  unsigned r = (value & 0xF800) >> 8;       // rrrrr... ........ -> rrrrr000
  unsigned g = (value & 0x07E0) >> 3;       // .....ggg ggg..... -> gggggg00
  unsigned b = (value & 0x1F) << 3;         // ............bbbbb -> bbbbb000

  return DashboardWidget::colorBright(r, g, b, br);
}

/*
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
    case WIDGET_XLARGE:
      this->size = wsize;
      this->width = WIDGET_WIDTH_XLARGE;
      this->height = WIDGET_HEIGHT_XLARGE;
      break;
    default:
      _error("unknown widget size %s, not configuring");
  }
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
// TODO: We need to configure minimum bounds here
// But we need to know font size to do so
void DashboardWidget::autoTextConfig(textAlignType align, Color color)
{
  switch(this->width) {
    case WIDGET_WIDTH_SMALL:
    case WIDGET_WIDTH_XLARGE:
      this->setCustomTextConfig(this->width, 0, color, align);
      break;
    default:
      _error("unknown widget size %s, not configuring");
      return;
  }
}

void DashboardWidget::setCustomTextConfig(uint8_t x, uint8_t y,
  Color color, textAlignType align, rgb_matrix::Font *textFont)
{
  this->textX = x;
  this->textY = y;
  this->textAlign = align;
  this->textColor = color;
  if (textFont == NULL) {
    this->textFont = defaultFont;
  } else {
    this->textFont = textFont;
  }
  strncpy(this->textData, "", WIDGET_TEXT_LEN);
  this->textInit = true;
}

// Update text and set temporary bold brightness
// TODO: Stop using cycles to track duration and
// leverage the clock we have
void DashboardWidget::updateText(char *text, uint32_t cycle = 0)
{
  if (strncmp(text, this->textData, WIDGET_TEXT_LEN) == 0)
    return;

  this->_logName();
  _debug("- updating to '%s' from old text: '%s', "
    "bright until cycle %d", text, this->textData,
    cycle + refreshDelay);

  this->_setText(text);
  if (cycle > 0) {
    this->resetCycle = cycle + refreshDelay;
    this->tempAdjustBrightness(boldBrightnessIncrease, BRIGHT_TEXT);
  }

  this->render();
}

// Call helper to do some logic, then update same as above
void DashboardWidget::updateText(char *data, void(helperFunc)(char*, char*), uint32_t cycle)
{
  char buffer[WIDGET_TEXT_LEN];

  // Dual-copy used here to prevent dupication of logic
  helperFunc(buffer, data);
  this->updateText(buffer, cycle);
}

/* ----==== [ Icon Functions ] ====---- */

// Set (x,y) origin coordinates for widget icon
void DashboardWidget::setIconConfig(uint8_t x, uint8_t y)
{
  this->iconX = x;
  this->iconY = y;
}

// Set size and image for widget icon (old format)
// This invocation will convert an existing 16-bit 565-encoded RGB
// image into the new format needed for the library to render natively
void DashboardWidget::setIconImage(uint8_t w, uint8_t h, const uint16_t *img)
{
  // Used for color565_2RGB pass-by-reference
  uint8_t *rgb = new uint8_t[3];

  // Allocate storage for new image format
  uint8_t *newImg = (uint8_t *)malloc(w * h * 3 * sizeof(uint8_t));

  // Read old image, convert and write to new buffer
  for (int src=0, dst=0; src < w*h;)
  {
      color565_2RGB(img[src++], rgb);
      newImg[dst++] = rgb[0];
      newImg[dst++] = rgb[1];
      newImg[dst++] = rgb[2];
  }

  this->setIconImage(w, h, newImg);
}

// Set size and image for widget icon (via new format)
void DashboardWidget::setIconImage(uint8_t w, uint8_t h, const uint8_t *img)
{
  this->iconWidth = w;
  this->iconHeight = h;
  this->iconImage = img;
  this->iconInit = true;
}

// Call helper to determine icon, then render
// No brightness logic similar to updateText()
// TODO: Move weather-specific logic into separate class
void DashboardWidget::updateIcon(char *forecast, const uint16_t*(helperFunc)(char*))
{
  if (forecast != NULL) {
    strncpy(this->iconData, forecast, WIDGET_DATA_LEN);
  }
  _debug("setting icon to %s", this->iconData);

  // this->_setIcon(helperFunc(this->data));
  this->render();
}

/*
  ----==== [ Rendering Functions ] ====----
*/

// TODO: Render an text-specific black clearing box
void DashboardWidget::renderText(bool debug) // char *text = NULL)
{
  uint16_t offset, offsetCalc;

  if (!this->textInit) {
    _error("renderText(%s) called without config, aborting", this->name);
    return;
  }
  // if (text != NULL) {
  //   this->_setText(text);
  // }

  if (this->textAlign == ALIGN_RIGHT)
  {
    offsetCalc = strlen(this->textData);
    offset = this->x + this->textX - offsetCalc * (FONT_WIDTH + 1);
  }
  else if (this->textAlign == ALIGN_CENTER)
  {
    offsetCalc = strlen(this->textData);
    offset = this->x + (this->textX / 2) - offsetCalc * (FONT_WIDTH + 1) / 2;
  }
  else {
    _error("unknown text alignment %d, not rendering", this->textAlign);
    return;
  }

  if (debug) {
    _debug("renderText(%s) = %s", this->name, this->textData);
    _debug("renderText x,textX = %d, %d", this->x, this->textX);
    _debug("renderText calc,offset = %d, %d", offsetCalc, offset);
  }

  drawText(offset, this->y + this->textY, this->textColor, this->textData, this->textFont);
  // drawText(offset, this->y + this->textY, this->textColor, this->textData);
}

// TODO: Render an icon-specific black clearing box
void DashboardWidget::renderIcon(bool debug)
{
  if (!this->iconInit) {
    _error("renderIcon(%s) called without config, aborting", this->name);
    return;
  }

  // if (icon != NULL)
  //   this->_setIcon(icon);
  drawIcon(this->x + this->iconX, this->y + this->iconY, this->iconWidth, this->iconHeight, this->iconImage);
}

// Render our widget
// TODO: Break clear widget logic into text and icon-specific
// sections and move to those rendering functions
void DashboardWidget::render(bool debug)
{
  // Clear widget
  _debug("clearing widget %s", this->name);

  if (!debug)
    drawRect(this->x, this->y, this->width, this->height, colorBlack);
  else
    drawRect(this->x, this->y, this->width, this->height, colorDarkGrey);

  // Render widget assets
  this->renderIcon();
  this->renderText();

  // Debugging bounding box for widget
  // Calculated from icon height & fixed width
  if (debug)
  {
    matrix->SetPixel(this->x, this->y, 255,0,0);
    matrix->SetPixel(this->x+this->width-1, this->y, 0,255,0);
    matrix->SetPixel(this->x, this->y+this->height-1, 0,0,255);
    matrix->SetPixel(this->x+this->width-1, this->y+this->height-1, 255,255,255);
  }
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
  if (this->resetCycle > 0 && cycle >= this->resetCycle)
  {
    this->_logName();
    _log("- resetting brightness");
    this->resetCycle = 0;
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