#include "widgetmanager.h"

WidgetManager::WidgetManager() {}

DashboardWidget* WidgetManager::operator[](uint16_t index) {
  return widgets.at(index);
}

uint16_t WidgetManager::size(void) {
  return widgets.size();
}

void WidgetManager::addWidget(DashboardWidget *widget) {
  widgets.push_back(widget);
}

// Check to see any widgets need updating
// Called in the main application loop
void WidgetManager::checkUpdate(void) {
  for (auto i = 0; i < widgets.size(); i++) {
    widgets[i]->checkUpdate();
  }
}

void WidgetManager::checkResetUpdateBrightness(bool force)
{
  for (auto i = 0; i < widgets.size(); i++)
  {
    widgets[i]->checkResetBrightness();
    widgets[i]->checkResetActive();

    if (force)
      widgets[i]->updateBrightness();
  }
}

void WidgetManager::displayDashboard(void) {
  for (auto i = 0; i < widgets.size(); i++) {
    widgets[i]->render();
  }
}