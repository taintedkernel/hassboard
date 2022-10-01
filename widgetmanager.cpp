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

void WidgetManager::checkUpdate(void) {
  for (uint16_t i = 0; i < widgets.size(); i++) {
    widgets[i]->checkUpdate();
  }
}

void WidgetManager::checkResetUpdateBrightness(bool force)
{
  for (uint16_t i = 0; i < widgets.size(); i++)
  {
    widgets[i]->checkResetBrightness();
    widgets[i]->checkResetActive();

    if (force)
      widgets[i]->updateBrightness();
  }
}

void WidgetManager::displayDashboard(void) {
  for (uint16_t i = 0; i < widgets.size(); i++) {
    widgets[i]->render();
  }
}