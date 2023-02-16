#ifndef WIDGETMANAGER_H
#define WIDGETMANAGER_H

#include "widget.h"
#include "dashboard.h"

#include <vector>

using namespace std;

class WidgetManager
{
private:
  uint8_t numWidgets = 0;
  vector<DashboardWidget *> widgets;

public:
  WidgetManager();
  DashboardWidget* operator[](uint16_t);
  uint16_t size(void);

  void addWidget(DashboardWidget *widget);
  void checkUpdate(void);
  void checkResetUpdateBrightness(bool force);
  void displayDashboard(void);
};

#endif