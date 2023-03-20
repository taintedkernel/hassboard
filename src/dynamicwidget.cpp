#include "dynamicwidget.h"
#include "datetime.h"
#include "logger.h"

#include <time.h>
#include <string.h>


void MultilineWidget::setTextUpdatePeriod(uint16_t period) {
  textUpdatePeriod = period;
}

void MultilineWidget::checkUpdate() {
  checkTextUpdate();
}

void MultilineWidget::checkTextUpdate()
{
  // _debug("checkTextUpdate @ %ld: last update=%ld", clock_ts(), lastUpdateTime);
  if (clock_ts() >= lastUpdateTime + textUpdatePeriod) {
    currentTextLine = 1 - currentTextLine;
    doTextUpdate();
    lastUpdateTime = clock_ts();
  }
}

// Update will rotate through the lines of text
// (newline-delimited) stored in fullTextData
void MultilineWidget::doTextUpdate()
{
  char *token, *str, *strFree;
  int i = 0;

  // Abort if no text is set
  if (strcmp(fullTextData, "") == 0)
    return;

  // Parse our text data, searching for newlines and track count
  // When we find desired line, copy contents to our widget text
  strFree = str = strdup(fullTextData);
  while ((token = strsep(&str, "\n"))) {
    if (i++ == currentTextLine) {
      // _debug("dWidget: %s: updating text to %s", name, token);
      strncpy(tData, token, WIDGET_TEXT_LEN);
      break;
    }
  }

  free(strFree);
  render();
}

// Set widget text
void MultilineWidget::setText(char *text)
{
  // _debug("dWidget %s: setting text to: %s", name, text);
  strncpy(fullTextData, text, WIDGET_TEXT_LEN);
  // Should we reset widget back to the first line when updating?
  currentTextLine = 0;
  doTextUpdate();
}

/*** AnimatedWidget ***/

void AnimatedWidget::setImageUpdatePeriod(uint16_t period) {
  imageUpdatePeriod = period;
}

void AnimatedWidget::checkUpdate() {
  checkImageUpdate();
}

void AnimatedWidget::checkImageUpdate()
{
  // _debug("checkTextUpdate @ %ld: last update=%ld", clock_ts(), lastUpdateTime);
  if (clock_ts() >= lastImageTime + imageUpdatePeriod) {
    doImageUpdate();
    lastImageTime = clock_ts();
  }
}

void AnimatedWidget::doImageUpdate() {}