#include "datetime.h"
#include "display.h"
#include "widget.h"
#include "logger.h"
#include "icons.h"

#include <cstring>
#include <algorithm>

#include <sys/stat.h>
#include <stdio.h>

#include <led-matrix.h>


uint16_t cloudsLocal[ICON_SZ], cloudsSunLocal[ICON_SZ];
uint16_t sunLocal[ICON_SZ], cloudsShowersLocal[ICON_SZ];

uint8_t refreshDelay = 5, refreshActiveDelay = 10;

extern uint8_t brightness;
extern uint8_t boldBrightnessIncrease;
extern uint32_t cycle;
extern rgb_matrix::RGBMatrix *matrix;
extern rgb_matrix::Color colorDarkGrey, colorBlack;
extern GirderFont *defaultFont;


// Convert received temperature to integer
char* tempIntHelper(char *payload)
{
  char *buffer = new char[WIDGET_TEXT_LEN];
  snprintf(buffer, WIDGET_TEXT_LEN, "%d", atoi(payload));
  return buffer;
}

// Convert received temperature to F, then integer
char* tempC2FHelper(char *payload)
{
  char *buffer = new char[WIDGET_TEXT_LEN];

  snprintf(buffer, WIDGET_TEXT_LEN, "%d%c",
    int(atof(payload) * 9 / 5 + 32), 176);

  return buffer;
}

// Limit string length of displayed value
// (if 10 or greater, just show integer value)
char* floatStrLen(char *payload)
{
  char *buffer = new char[WIDGET_TEXT_LEN];

  if (atof(payload) >= 10.0) {
    snprintf(buffer, WIDGET_TEXT_LEN, "%d", int(atof(payload)));
  } else {
    snprintf(buffer, WIDGET_TEXT_LEN, "%1.1f", atof(payload));
  }

  return buffer;
}

// Translate weather condition to matching icon, return filename
const char* weatherIconHelper(char *condition)
{
  if (strcmp(condition, "sunny") == 0) {
    return(ICON_WEATHER_SUNNY);
  }
  else if (strcmp(condition, "partlycloudy") == 0) {
    return(ICON_WEATHER_PCLOUDY);
  }
  else if (strcmp(condition, "cloudy") == 0) {
    return(ICON_WEATHER_CLOUDY);
  }
  else if (strcmp(condition, "rainy") == 0) {
    return(ICON_WEATHER_RAINY);
  }
  else if (strcmp(condition, "snowy") == 0) {
    return(ICON_WEATHER_SNOWY);
  }
  else if (strcmp(condition, "fog") == 0) {
    return(ICON_WEATHER_FOG);
  }
  else if (strcmp(condition, "clear-night") == 0) {
    return(ICON_WEATHER_CLEAR_NIGHT);
  }
  else if (strcmp(condition, "exceptional") == 0) {
    return(ICON_WEATHER_EXCEPTIONAL);
  }
  else {
    return(condition);
  }
}

// Convert a 565-encoded color to individual RGB values
void color565_2RGB(uint16_t value, uint8_t *rgb)
{
  uint8_t r = (value & 0xF800) >> 8;       // rrrrr... ........ -> rrrrr000
  uint8_t g = (value & 0x07E0) >> 3;       // .....ggg ggg..... -> gggggg00
  uint8_t b = (value & 0x1F) << 3;         // ............bbbbb -> bbbbb000

  rgb[0] = r;
  rgb[1] = g;
  rgb[2] = b;
}

/*** DashboardWidget class ***/

// Constructor
DashboardWidget::DashboardWidget(const char *wName)
{
  iBrightness = brightness;
  tBrightness = brightness;
  strncpy(iData, "", WIDGET_DATA_LEN);
  strncpy(name, wName, WIDGET_NAME_LEN);
  tFont = defaultFont;
}

// Debugging helper
void DashboardWidget::_logName() {
  _debug("widget %s:", name);
}

// Get widget width
uint8_t DashboardWidget::_getWidth() {
  return width;
}

// Get widget height
uint8_t DashboardWidget::_getHeight() {
  return height;
}

// Get widget icon size
uint16_t DashboardWidget::_getIconSize() {
  return (iWidth * iHeight);
}

/*
  ----==== [ Configuration Functions ] ====----
*/
// Set/clear debugging flag for widget
void DashboardWidget::setDebug(bool value) {
  debug = value;
}

// Set/clear active flag for widget
void DashboardWidget::setActive(bool value) {
  active = value;
}

// Set widget origin
void DashboardWidget::setOrigin(uint8_t x, uint8_t y) {
  widgetX = x;
  widgetY = y;
}

// Set size of widget
void DashboardWidget::setSize(widgetSizeType wsize)
{
  switch(wsize) {
    case WIDGET_SMALL:
      size = wsize;
      width = WIDGET_WIDTH_SMALL;
      height = WIDGET_HEIGHT_SMALL;
      break;
    case WIDGET_LARGE:
      size = wsize;
      width = WIDGET_WIDTH_LARGE;
      height = WIDGET_HEIGHT_LARGE;
      break;
    case WIDGET_LONG:
      size = wsize;
      width = WIDGET_WIDTH_LONG;
      height = WIDGET_HEIGHT_LONG;
      break;
    default:
      _error("unknown widget size %s, not configuring");
  }
}

// Set bounds for widget rendering box
// TODO: This should be merged with setSize() as it overwrites the values
void DashboardWidget::setBounds(uint8_t w, uint8_t h) {
  width = w;
  height = h;
}

/* ----==== [ Text Functions ] ====---- */

// Get widget text
char* DashboardWidget::getText() {
  return tData;
}

// Set widget text
void DashboardWidget::setText(char *text)
{
  _debug("widget %s: setting text to: %s", name, text);
  strncpy(tData, text, WIDGET_TEXT_LEN);
}

// Set custom font
void DashboardWidget::setFont(GirderFont *font)
{
  tFont = font;
}

// Set widget text length
void DashboardWidget::setVisibleTextLength(u_int16_t length)
{
  if (length > WIDGET_TEXT_LEN) {
    _error("setVisibleTextLength(%d) on widget %s beyond maximum, using max as value",
        length, name);
    length = WIDGET_TEXT_LEN;
  }

  tVisibleSize = length;
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
  switch(width) {
    case WIDGET_WIDTH_SMALL:
    case WIDGET_WIDTH_LARGE:
      setCustomTextConfig(width, 0, color, align);
      break;
    default:
      _error("unknown widget size %s, not configuring");
      return;
  }
}

// Set a manually-provided text configuration with locations
// in the widget, text color and font information
void DashboardWidget::setCustomTextConfig(uint8_t textX, uint8_t textY,
  Color color, textAlignType align, GirderFont *textFont)
{
  tX = textX;
  tY = textY;
  tColor = color;
  tAlign = align;
  tInit = true;

  // Use default font if not provided
  if (textFont == NULL) {
    tFont = defaultFont;
  } else {
    tFont = textFont;
  }

  // if (clearText) {
  //   strncpy(textData, "", WIDGET_TEXT_LEN);
  // }
}

// Set a custom text-rendering function
void DashboardWidget::setCustomTextRender(void (render)TEXT_RENDER_SIG)
{
  customTextRender = render;
}

// Render widget with custom variable-width font spacing
void DashboardWidget::setVariableWidth(bool vWidth)
{
  tVarWidth = vWidth;
}


// Set an alert level and color (currently on text)
// Note: This currently only supports upper-bounds levels
void DashboardWidget::setAlertLevel(float alertLevel, rgb_matrix::Color alertColor)
{
  tAlertLevel = alertLevel;
  tAlertColor = alertColor;
}

void DashboardWidget::setTextColor(rgb_matrix::Color newTextColor)
{
  tColor = newTextColor;
}

// Update text and set temporary bold brightness
void DashboardWidget::updateText(char *text, bool brighten)
{
  // Abbreviate zero/null floating-point values
  if (strcmp(text, "0.0") == 0)
    text = (char *) "--";

  // If new text is not different, don't update
  if (strncmp(text, tData, WIDGET_TEXT_LEN) == 0)
    return;

  char updateEnd[10];
  time_t ts = clock_ts() + refreshDelay;
  time_t local = time(&ts);
  tm *localtm = localtime(&local);
  snprintf(updateEnd, 9, "%02d:%02d:%02d", hour(localtm), minute(localtm), second(localtm));

  _debug("widget %s: updating to '%s' from old text: '%s', "
    "bright until %s", name, text, tData, updateEnd);

  setText(text);
  if (brighten) {
    resetTime = clock_ts() + refreshDelay;
    tempAdjustBrightness(boldBrightnessIncrease, BRIGHT_TEXT);
  }

  render();
}

// Update text with helper, then update same as above
void DashboardWidget::updateText(char *data, char*(helperFunc)(char*), bool brighten)
{
  // char buffer[WIDGET_TEXT_LEN];

  // Dual-copy used here to prevent dupication of logic
  char *updatedText = helperFunc(data);
  updateText(updatedText, brighten);
  free(updatedText);
}

/* ----==== [ Icon Functions ] ====---- */

// Set (x,y) origin coordinates for widget icon
void DashboardWidget::setIconOrigin(uint8_t x, uint8_t y)
{
  iX = x;
  iY = y;
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
  for (int src=0, dst=0; src < w*h;)
  {
    color565_2RGB(img[src++], rgb);
    newImg[dst++] = rgb[0];
    newImg[dst++] = rgb[1];
    newImg[dst++] = rgb[2];
  }

  setIconImage(w, h, newImg);
}

// Set widget icon and size
void DashboardWidget::setIconImage(uint8_t w, uint8_t h, const uint8_t *img)
{
  iWidth = w;
  iHeight = h;
  iImage = img;
  iInit = true;
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

  setIconImage(w, h, newImg);
}

// Call helper to determine icon, then render
// Note: No brightness logic similar to updateText()
void DashboardWidget::updateIcon(char *data, const char* (helperFunc)(char*))
{
  if (data != NULL) {
    strncpy(iData, data, WIDGET_DATA_LEN);
  }
  _debug("setting icon to %s", iData);

  setIconImage(iWidth, iHeight, helperFunc(iData));
  render();
}

/*
  ----==== [ Rendering Functions ] ====----
*/

// Clear the rendering bounds of our widget
void DashboardWidget::clear(bool force)
{
  // If we are inactive and no force-clear set then return
  if (!active && !force)
    return;

  // if (debug)
  //   drawRect(widgetX, widgetY, width, height, colorDarkGrey);
  // else
  drawRect(widgetX, widgetY, width, height+1, colorBlack);
}

// Render our widget
// TODO: Break clear widget logic into text and icon-specific
// sections and move to those rendering functions
void DashboardWidget::render()
{
  if (!active)
    return;

  // Clear widget
  // _debug("clearing widget %s", name);
  clear();

  // Render widget assets
  renderIcon();
  renderText();

  // Debugging bounding box for widget
  // Calculated from icon height & fixed width
  if (debug)
  {
    matrix->SetPixel(widgetX, widgetY, 255,0,0);
    matrix->SetPixel(widgetX+width-1, widgetY, 0,255,0);
    matrix->SetPixel(widgetX, widgetY+height-1, 0,0,255);
    matrix->SetPixel(widgetX+width-1, widgetY+height-1, 255,255,255);
  }
}

// TODO: Render an text-specific black clearing box
int DashboardWidget::renderText()
{
  Color color;
  int16_t offset;
  bool localDebug = true;

  // Verify initialization & active status
  if (!tInit) {
    _error("renderText(%s) called without config, aborting", name);
    return 0;
  }
  else if (!active) {
    return 0;
  }

  /* Old text scroll code lived here */
  uint8_t textLen = strlen(tData);
  uint16_t renderLen;

  if (tVarWidth) {
    renderLen = textRenderLength(tData, tFont);
  } else {
    // Font width set to width of rendered glyph
    // without any padding, account for this
    renderLen = textLen * (tFont->width + 1) - 1;
  }

  // Calculate positioning of text based on alignment
  if (tAlign == ALIGN_RIGHT) {
    offset = tX - renderLen - 2;
  }
  else if (tAlign == ALIGN_CENTER) {
    offset = (tX / 2) - (renderLen / 2);
  }
  else if (tAlign == ALIGN_LEFT) {
    offset = iWidth + WIDGET_ICON_TEXT_GAP;
  }
  else {
    _error("unknown text alignment %d, not rendering", tAlign);
    return 0;
  }

  if (offset < 0) {
    _warn("text length during alignment exceeds limits, may be truncated");
    offset = 0;
  }

  // Calculate color, apply upper bound on channels
  // Note: This currently only supports upper-bounds levels
  if (tAlertLevel > 0.00000001 && atof(tData) > tAlertLevel) {
    color = Color(tAlertColor);
  }
  else {
    color = Color(tColor);
  }
  color.r = std::min(color.r + tTempBrightness, 255);
  color.g = std::min(color.g + tTempBrightness, 255);
  color.b = std::min(color.b + tTempBrightness, 255);

  if (debug && localDebug)
  {
    _debug("renderText(%s) = [%s]", name, tData);
    _debug("- x,textX,len,renderLen,offset = %d, %d, %d, %d, %d",
      widgetX, tX, textLen, renderLen, offset);

    // yellow top-left
    matrix->SetPixel(widgetX + offset, widgetY, 64, 64, 0);
    matrix->SetPixel(widgetX + offset, widgetY + tFont->height-1,
      0, 64, 64); // cyan bottom-left
    matrix->SetPixel(widgetX + offset + renderLen - 1, widgetY,
      64, 0, 64); // violet top-right
    matrix->SetPixel(widgetX + offset + renderLen - 1,
      widgetY + tFont->height-1, 64, 32, 64); // pink bottom-right
  }

  // Call the custom text renderer, if set
  if (customTextRender)
    customTextRender(widgetX + offset, widgetY +
        tY, color, tData, tFont, tVarWidth);
  else
    drawText(widgetX + offset, widgetY + tY, color,
        tData, tFont, tVarWidth, debug);

  return 0;
}

// TODO: Render an icon-specific black clearing box
void DashboardWidget::renderIcon()
{
  if (!iInit || iImage == NULL) {
    _error("renderIcon(%s) called without config and/or image, aborting", name);
    return;
  }

  if (!active)
    return;

  drawIcon(widgetX + iX, widgetY + iY, iWidth,
      iHeight, iImage);
}

/*
  ----==== [ Color/Brightness Functions ] ====----
*/

// Resets brightness to current global value
void DashboardWidget::resetBrightness(brightType bType = BRIGHT_TEXT)
{
  if (bType != BRIGHT_TEXT)
    iBrightness = brightness;
  if (bType != BRIGHT_ICON)
    tBrightness = brightness;
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
  if (iBrightness != brightness + iTempBrightness) {
    iBrightness = brightness + iTempBrightness;
    renderIcon();
  }
  if (tBrightness != brightness + tTempBrightness) {
    tBrightness = brightness + tTempBrightness;
    renderText();
  }
}

// Checks if temporary brightness duration has elapsed and reset if necessary
void DashboardWidget::checkResetBrightness()
{
  if (resetTime > 0 && clock_ts() >= resetTime)
  {
    _logName();
    _log("- resetting brightness");
    resetTime = 0;
    resetBrightness(BRIGHT_BOTH);
    iTempBrightness = tTempBrightness = 0;
    render();
  }
}

// Sets brightness to current global value + some custom offset
void DashboardWidget::tempAdjustBrightness(uint8_t tempBright, brightType bType = BRIGHT_TEXT)
{
  if (bType != BRIGHT_TEXT) {
    iTempBrightness = tempBright;
    iBrightness = brightness + tempBright;
  }
  if (bType != BRIGHT_ICON) {
    tTempBrightness = tempBright;
    tBrightness = brightness + tempBright;
  }
}

// Check if temporary active duration has elapsed and reset if necessary
void DashboardWidget::checkResetActive()
{
  if (resetActiveTime > 0 && clock_ts() >= resetActiveTime)
  {
    _logName();
    _log("- resetting active to: %d", !(active));
    resetActiveTime = 0;
    setActive(!(active));
    clear();
    render();
  }
}

// Set the time to reset the active state
void DashboardWidget::setResetActiveTime(time_t time)
{
  resetActiveTime = time;

  char updateEnd[10];
  tm *localtm = localtime(&time);
  snprintf(updateEnd, 9, "%02d:%02d:%02d", hour(localtm), minute(localtm), second(localtm));

  _debug("widget %s: setting active until %s", name, updateEnd);
}

// Set the time to reset the active state to now
void DashboardWidget::setResetActiveTime(uint16_t delay)
{
  resetActiveTime = clock_ts() + delay;

  char updateEnd[10];
  tm *localtm = localtime(&resetActiveTime);
  snprintf(updateEnd, 9, "%02d:%02d:%02d", hour(localtm), minute(localtm), second(localtm));

  _debug("widget %s: setting active until %s", name, updateEnd);
}

void DashboardWidget::checkUpdate() {}