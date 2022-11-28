#include "dynamicwidget.h"
#include "logger.h"

#include <time.h>
#include <string.h>

void DynamicDashboardWidget::setTextUpdatePeriod(uint16_t period) {
  textUpdatePeriod = period;
}

void DynamicDashboardWidget::checkUpdate() {
  checkTextUpdate();
}

void DynamicDashboardWidget::checkTextUpdate()
{
  // _debug("checkTextUpdate @ %ld: last update=%ld", clock(), lastUpdateTime);
  if (clock() >= lastUpdateTime +
          textUpdatePeriod / 1000 * CLOCKS_PER_SEC) {
    currentTextLine = 1 - currentTextLine;
    doTextUpdate();
  }
}

// Update will rotate through the lines of text
// (newline-delimited) stored in fullTextData
void DynamicDashboardWidget::doTextUpdate()
{
  char *token, *str, *strFree;
  int i = 0;

  if (strcmp(fullTextData, "") == 0)
    return;

  strFree = str = strdup(fullTextData);
  while ((token = strsep(&str, "\n"))) {
    if (i++ == currentTextLine) {
      // _debug("dWidget: %s: updating text to %s", name, token);
      strncpy(tData, token, WIDGET_TEXT_LEN);
      break;
    }
  }

  lastUpdateTime = clock();
  free(strFree);
  render();
}

// Set widget text
void DynamicDashboardWidget::setText(char *text)
{
  // _debug("dWidget %s: setting text to: %s", name, text);
  strncpy(fullTextData, text, WIDGET_TEXT_LEN);
  // Should we reset widget back to the first line when updating?
  currentTextLine = 0;
  doTextUpdate();
}