#ifndef WIDGET_H
#define WIDGET_H

#include "display.h"

#include <png++/png.hpp>
#include <graphics.h>
#include <time.h>

#define WIDGET_NAME_LEN         32
#define WIDGET_TEXT_LEN         32
#define WIDGET_DATA_LEN         32

#define WIDGET_WIDTH_SMALL      28
#define WIDGET_HEIGHT_SMALL     FONT_HEIGHT

#define WIDGET_WIDTH_LARGE      32
#define WIDGET_HEIGHT_LARGE     32

#define WIDGET_WIDTH_LONG       128
#define WIDGET_HEIGHT_LONG      FONT_HEIGHT

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
  enum widgetSizeType{WIDGET_SMALL, WIDGET_LARGE, WIDGET_LONG};
  enum brightType{BRIGHT_ICON, BRIGHT_TEXT, BRIGHT_BOTH};
  enum textAlignType{ALIGN_RIGHT, ALIGN_CENTER};

private:
  char name[WIDGET_NAME_LEN+1];   // Name of widget
  uint8_t x = 0;
  uint8_t y = 0;
  uint8_t width = 0;
  uint8_t height = 0;
  widgetSizeType size = WIDGET_SMALL;

  bool active = true;
  bool debug = false;

  // Icon config/data
  int8_t iconX = 0;
  int8_t iconY = 0;
  uint8_t iconWidth = 0;
  uint8_t iconHeight = 0;
  uint8_t iconBrightness = 0;
  uint8_t iconTempBrightness = 0;
  const uint8_t *iconImage = NULL;
  char iconData[WIDGET_DATA_LEN+1];

  // Text config/data
  uint8_t textX = 0;
  uint8_t textY = 0;
  uint8_t textWidth = 0;
  uint8_t textHeight = 0;
  uint8_t textBrightness = 0;
  uint8_t textTempBrightness = 0;
  uint8_t textAlign;
  int (*customTextRender)TEXT_RENDER_SIG;

  rgb_matrix::Font *textFont;
  rgb_matrix::Color textColor;
  uint8_t textFontWidth = 0;
  uint8_t textFontHeight = 0;
  char textData[WIDGET_TEXT_LEN+1];

  // Initialization / config
  clock_t resetTime;        // Track when temp brightness resets
  clock_t resetActiveTime;  // Track when active toggles
  bool iconInit = false;
  bool textInit = false;

  // Functions - Getters/setters/helpers
  void      _logName();
  uint8_t   _getWidth();
  uint8_t   _getHeight();
  uint16_t  _getIconSize();
public:
  void      _setText(char *);
private:
  Color     color2RGB(uint8_t[3]);

  // Functions - Rendering
  int renderText();
  void renderIcon();

public:
  // Functions - Constants, defaults, enums, etc
  // static const uint8_t widgetDefaultWidth = WIDGET_DEF_WIDTH;
  // static const uint8_t textDefaultHeight = FONT_HEIGHT;
  static const uint8_t textDefaultWidth = FONT_WIDTH;

  // Functions - Color helper functions
  // static Color colorBright(uint8_t, uint8_t, uint8_t, int);
  // static Color colorBright(uint8_t[], int);
  // static Color colorBright(uint16_t, int);
  static void color565_2RGB(uint16_t, uint8_t *);

  // Functions - Init / config
  DashboardWidget(const char *);
  char* getText();
  void setDebug(bool);
  void setActive(bool);
  void setOrigin(uint8_t x, uint8_t y);
  void setSize(widgetSizeType);
  void setBounds(uint8_t width, uint8_t height);
  void setCustomTextRender(int (render)TEXT_RENDER_SIG);

  // Functions - Text
  void autoTextConfig(Color = colorText, textAlignType = ALIGN_RIGHT);
  void setCustomTextConfig(uint8_t x, uint8_t y, Color color = colorText,
    textAlignType = ALIGN_RIGHT, rgb_matrix::Font *textFont = NULL,
    uint8_t fontWidth = 0, uint8_t fontHeight = 0);
    // bool clearText = true);
  void updateText(char *text, bool brighten = true);
  void updateText(char *text, void(helperFunc)(char*, char*), bool brighten = true);

  // Functions - Icon
  void setIconConfig(uint8_t x, uint8_t y);
  void setIconImage(uint8_t width, uint8_t height, const char* iconFile);
  void setIconImage(uint8_t width, uint8_t height, const uint16_t *image);
  void setIconImage(uint8_t width, uint8_t height, const uint8_t *image);
  void updateIcon(char *iconData, const char*(helperFunc)(char*));

  // Functions - Rendering
  void render();
  void clear(bool = false);

  // Functions - Brightness adjustments
  void resetBrightness(brightType);
  void updateBrightness();
  void checkResetBrightness();
  void tempAdjustBrightness(uint8_t tempBright, brightType);

  // Functions - "Active-ness" adjustments
  void checkResetActive();
  void setResetActiveTime(clock_t time);
};

#endif