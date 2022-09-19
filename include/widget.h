#ifndef WIDGET_H
#define WIDGET_H

#include "display.h"

#include <png++/png.hpp>
#include <graphics.h>
#include <time.h>

#define WIDGET_NAME_LEN         32
#define WIDGET_TEXT_LEN         256
#define WIDGET_DATA_LEN         32

#define WIDGET_WIDTH_SMALL      28
#define WIDGET_HEIGHT_SMALL     FONT_HEIGHT

#define WIDGET_WIDTH_LARGE      32
#define WIDGET_HEIGHT_LARGE     32

#define WIDGET_WIDTH_LONG       128
#define WIDGET_HEIGHT_LONG      FONT_HEIGHT

#define WIDGET_ICON_TEXT_GAP    4

#define ICON_SZ                 800
#define ICON_SZ_BYTES           ICON_SZ * sizeof(uint16_t)

#define TEXT_RENDER_SIG         (uint8_t x, uint8_t y, Color color, const char *text, \
    rgb_matrix::Font *font, uint8_t hSpacing, uint8_t fontHeight)

using rgb_matrix::Color;

void tempIntHelper(char *, char *);
void tempC2FHelper(char *, char *);
void floatStrLen(char *, char *);
const char* weatherIconHelper(char *);

extern Color colorText;


class DashboardWidget
{
public:
  // Constants, defaults, enums, etc
  enum widgetSizeType{WIDGET_SMALL, WIDGET_LARGE, WIDGET_LONG};
  enum brightType{BRIGHT_ICON, BRIGHT_TEXT, BRIGHT_BOTH};
  enum textAlignType{ALIGN_RIGHT, ALIGN_CENTER, ALIGN_LEFT};
  static const uint8_t textDefaultWidth = FONT_WIDTH;

protected:
  /*** ATTRIBUTES ***/
  char name[WIDGET_NAME_LEN+1];   // Name of widget
  bool active = true;
  bool debug = false;

  uint8_t widgetX = 0;
  uint8_t widgetY = 0;
  uint8_t width = 0;
  uint8_t height = 0;
  widgetSizeType size = WIDGET_SMALL;

  // Text config/data
  bool tInit = false;
  uint8_t tX = 0;
  uint8_t tY = 0;
  uint8_t tWidth = 0;
  uint8_t tHeight = 0;
  uint8_t tBrightness = 0;
  uint8_t tTempBrightness = 0;
  int (*customTextRender)TEXT_RENDER_SIG;
  char tData[WIDGET_TEXT_LEN+1];

  // Text fonts/colors/data/etc
  rgb_matrix::Font *tFont;
  rgb_matrix::Color tColor;

  uint8_t tAlign;
  uint8_t tFontWidth = 0;
  uint8_t tFontHeight = 0;
  uint8_t tVisibleSize = 0;       // Visible text length on widget

  float tAlertLevel;
  rgb_matrix::Color tAlertColor;

  // Icon config/data
  bool iInit = false;
  int8_t iX = 0;
  int8_t iY = 0;
  uint8_t iWidth = 0;
  uint8_t iHeight = 0;
  uint8_t iBrightness = 0;
  uint8_t iTempBrightness = 0;
  const uint8_t *iImage = NULL;
  char iData[WIDGET_DATA_LEN+1];

  // Initialization / config
  clock_t resetTime;        // Track when temp brightness resets
  clock_t resetActiveTime;  // Track when active toggles

  /*** FUNCTIONS ***/
  // Getters/setters/helpers
  void      _logName();
  uint8_t   _getWidth();
  uint8_t   _getHeight();
  uint16_t  _getIconSize();

public:
  // Init / config
  DashboardWidget(const char *);

  void setDebug(bool);
  void setActive(bool);
  void setOrigin(uint8_t x, uint8_t y);
  void setSize(widgetSizeType);
  void setBounds(uint8_t width, uint8_t height);

  // Functions - Text
  char* getText();

protected:
  virtual void setText(char *);

public:
  void setVisibleSize(u_int16_t);
  void autoTextConfig(Color = colorText, textAlignType = ALIGN_RIGHT);
  void setCustomTextConfig(uint8_t textX, uint8_t textY, Color color = colorText,
    textAlignType = ALIGN_RIGHT, rgb_matrix::Font *textFont = NULL,
    uint8_t fontWidth = 0, uint8_t fontHeight = 0);
    // bool clearText = true);
  void setCustomTextRender(int (render)TEXT_RENDER_SIG);
  void setAlertLevel(float, rgb_matrix::Color);
  void updateText(char *text, bool brighten = true);
  void updateText(char *text, void(helperFunc)(char*, char*), bool brighten = true);

  // Functions - Icon
  void setIconOrigin(uint8_t x, uint8_t y);
  void setIconImage(uint8_t width, uint8_t height, const char* iconFile);
  void setIconImage(uint8_t width, uint8_t height, const uint16_t *image);
  void setIconImage(uint8_t width, uint8_t height, const uint8_t *image);
  void updateIcon(char *iconData, const char*(helperFunc)(char*));

  // Functions - Rendering
  void clear(bool = false);
  void render();
protected:
  virtual int renderText();
  void renderIcon();

public:
  // Functions - Brightness adjustments
  void resetBrightness(brightType);
  void updateBrightness();
  void checkResetBrightness();
  void tempAdjustBrightness(uint8_t tempBright, brightType);

  // Functions - "Active-ness" adjustments
  void checkResetActive();
  void setResetActiveTime(clock_t time);

  // Functions - Generic periodic updates
  virtual void checkUpdate();
};

#endif