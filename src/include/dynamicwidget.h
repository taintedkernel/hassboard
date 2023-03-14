#ifndef DYNAMICWIDGET_H
#define DYNAMICWIDGET_H

#include "widget.h"
#include "display.h"

#include <graphics.h>
#include <time.h>

#define TEXT_UPDATE_PERIOD_MS    5000
#define FRAME_UPDATE_PERIOD_MS   1000

// Sub-class to implement a widget that can change
// the display of rendered contents.  Currently only
// supports updating text by cycling through seperate
// lines.
class MultilineWidget : public DashboardWidget
{
private:
  time_t lastUpdateTime = 0;
  uint16_t textUpdatePeriod = TEXT_UPDATE_PERIOD_MS / 1000;
  uint8_t currentTextLine = 0;
  char fullTextData[WIDGET_TEXT_LEN+1];

  virtual void setText(char *);
  void doTextUpdate();

public:
  MultilineWidget(const char *name):DashboardWidget(name) {}

  // void updateText(char *text, bool brighten = true);

  void setTextUpdatePeriod(uint16_t period);
  void checkTextUpdate();
  void checkUpdate();
};

class AnimatedWidget : public DashboardWidget
{
private:
  time_t lastImageTime = 0;
  uint16_t imageUpdatePeriod = FRAME_UPDATE_PERIOD_MS / 1000;
  // uint8_t currentFrame = 0;

  void doImageUpdate();

public:
  AnimatedWidget(const char *name):DashboardWidget(name) {}

  void setImageUpdatePeriod(uint16_t period);
  void checkImageUpdate();
  void checkUpdate();
};

// Clock widget
// Dynamic that it updates
// But needs no internal data storage
// Custom rendering

#endif