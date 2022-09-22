#include "dashboard.h"
#include "logger.h"
#include "widget.h"
#include "dynamicwidget.h"
#include "icons.h"
#include "mqtt.h"

#include <mosquitto.h>
#include <unistd.h>

#include <cstring>


char weatherCondition[WEATHER_MAX_LEN+1] = "";
char outdoorTemp[DASH_MAX_LEN+1] = "";

bool heating = false;
bool daytime = true;

int8_t dashOffset = 0;
int8_t weatherOffset = 64;
int8_t clockOffset = 65;
uint8_t clockWidth = 32;
uint8_t rowDayStart = 2;
uint8_t rowDateStart = rowDayStart + 10;
uint8_t rowTimeStart = rowDateStart + 10;
uint8_t rowTempStart = 26;

uint8_t colOneStart = dashOffset + 1;
uint8_t colOneEnd = dashOffset + 30;
uint8_t colTwoStart = dashOffset + 34;
uint8_t colTwoEnd = dashOffset + 63;

uint8_t rowSpacing = 3;
uint8_t rowOneStart = 1;
uint8_t rowTwoStart = rowOneStart + FONT_HEIGHT + rowSpacing;
uint8_t rowThreeStart = rowTwoStart + FONT_HEIGHT + rowSpacing;
uint8_t rowCalendarStart = rowThreeStart + (FONT_HEIGHT + rowSpacing) * 2;

// *TODO*: Wire up photocell and use to determine brightness
// Control brightness by adjusting RGB values. Brightness is a
// percentage so brightness=50 turns 250,250,250 into 125,125,125
const int8_t dimBrightness = 10;
const int8_t brightBrightness = 50;
uint8_t brightness = dimBrightness;
uint8_t iconExtraBrightness = 0;
uint8_t boldBrightnessIncrease = 100;
uint8_t numWidgets = 0;

DashboardWidget wHouseTemp("houseTemp");
DashboardWidget wHouseDewpoint("houseDewpoint");
DashboardWidget wOutdoorRainGauge("outdoorRainGauge");
DashboardWidget wOutdoorDewpoint("outdoorDewpoint");
DashboardWidget wOutdoorWind("outdoorWind");
DashboardWidget wOutdoorPM25("outdoorPM25");
DashboardWidget wOutdoorWeather("outdoorWeather");
DashboardWidget wOutdoorForecast("outdoorForecast");
DynamicDashboardWidget wCalendar("calendar");

// TODO: fix this, eg: add a widget manager
DashboardWidget *widget, *widgetCollection[MAX_WIDGETS];

rgb_matrix::Font *customFont, *smallFont;

extern uint32_t cycle;
extern bool forceRefresh;
extern uint8_t refreshActiveDelay;
extern rgb_matrix::Color colorText, colorDayText, colorNightText;
extern rgb_matrix::Color colorWhite, colorGrey, colorDarkText, colorAlert;
extern rgb_matrix::Font *defaultFont;


void setupDashboard()
{
  _log("configuring dashboard");

  // Row 1
  // Living room temperature
  widget = &wHouseTemp;
  widget->setOrigin(colOneStart-1, rowOneStart);
  widget->setSize(DashboardWidget::WIDGET_SMALL);
  widget->autoTextConfig();
  widget->setIconImage(8, 7, big_house);
  widget->setIconOrigin(0, 1);
  widget->setVisibleSize(3);
  // widget->setDebug(true);

  // Living room dewpoint
  widget = &wHouseDewpoint;
  widget->setOrigin(colTwoStart, rowOneStart);
  widget->setSize(DashboardWidget::WIDGET_SMALL);
  widget->autoTextConfig();
  widget->setIconImage(8, 7, big_house_drop);
  widget->setIconOrigin(0, 1);
  widget->setVisibleSize(3);

  // Row 2
  // Rain gauge
  widget = &wOutdoorRainGauge;
  widget->setOrigin(colOneStart, rowTwoStart);
  widget->setSize(DashboardWidget::WIDGET_SMALL);
  widget->autoTextConfig();
  widget->setIconImage(8, 8, rain_gauge);
  // This metric updates on slower interval, so default to blank
  widget->updateText((char *)"--", false);
  widget->setVisibleSize(3);

  // Dewpoint
  widget = &wOutdoorDewpoint;
  widget->setOrigin(colTwoStart, rowTwoStart);
  widget->setSize(DashboardWidget::WIDGET_SMALL);
  widget->autoTextConfig();
  widget->setIconImage(8, 8, droplet);
  widget->setVisibleSize(3);

  // Row 3
  // Wind speed
  widget = &wOutdoorWind;
  widget->setOrigin(colOneStart, rowThreeStart);
  widget->setSize(DashboardWidget::WIDGET_SMALL);
  widget->autoTextConfig();
  widget->setIconImage(8, 8, wind);
  // This metric updates on slower interval, so default to blank
  widget->updateText((char *)"--", false);
  widget->setVisibleSize(3);

  // PM2.5
  widget = &wOutdoorPM25;
  widget->setOrigin(colTwoStart, rowThreeStart);
  widget->setSize(DashboardWidget::WIDGET_SMALL);
  widget->autoTextConfig();
  widget->setIconImage(7, 8, air);
  // widget->setTextConfig(WIDGET_WIDTH_SMALL, 0);
  widget->setVisibleSize(3);
  widget->setAlertLevel(20.0, colorAlert);

  // Main, current weather row (2nd display)
  customFont = new rgb_matrix::Font;
  customFont->LoadFont(FONT_FILE_2);
  widget = &wOutdoorWeather;
  widget->setOrigin(weatherOffset, 0);
  widget->setSize(DashboardWidget::WIDGET_LARGE);
  widget->setIconImage(32, 25, cloud_sun_new);
  widget->setCustomTextConfig(WIDGET_WIDTH_LARGE, rowTempStart,
    colorWhite, DashboardWidget::ALIGN_CENTER, customFont,
    FONT_WIDTH_2, FONT_HEIGHT_2);
  widget->setBounds(32, 34);
  widget->setVisibleSize(3);

  // Alternate forecast widget, to show over current weather
  smallFont = new rgb_matrix::Font;
  smallFont->LoadFont(FONT_FILE_SMALL);
  widget = &wOutdoorForecast;
  widget->setOrigin(weatherOffset, 0);
  widget->setSize(DashboardWidget::WIDGET_LARGE);
  widget->setIconImage(32, 25, cloud_sun_new);
  widget->setCustomTextConfig(WIDGET_WIDTH_LARGE+4, rowTempStart-2,
    colorText, DashboardWidget::ALIGN_CENTER, smallFont,
    FONT_WIDTH_SMALL, FONT_HEIGHT_SMALL);
  widget->setCustomTextRender(drawTextCustom);
  widget->setBounds(32, 34);
  widget->setActive(false);
  widget->setVisibleSize(5);

  // Calendar events
  widget = &wCalendar;
  widget->setOrigin(colOneStart, rowCalendarStart);
  widget->setSize(DashboardWidget::WIDGET_LONG);
  widget->setIconImage(9, 8, "icons/calendar.png");
  widget->setCustomTextConfig(WIDGET_WIDTH_LONG, 0,
    colorText, DashboardWidget::ALIGN_LEFT, smallFont,
    FONT_WIDTH_SMALL, FONT_HEIGHT_SMALL);
  widget->setCustomTextRender(drawTextCustom);
  widget->setVisibleSize(20);
  // widget->setDebug(true);

  widgetCollection[numWidgets++] = &wHouseTemp;
  widgetCollection[numWidgets++] = &wHouseDewpoint;
  widgetCollection[numWidgets++] = &wOutdoorRainGauge;
  widgetCollection[numWidgets++] = &wOutdoorDewpoint;
  widgetCollection[numWidgets++] = &wOutdoorWind;
  widgetCollection[numWidgets++] = &wOutdoorPM25;
  widgetCollection[numWidgets++] = &wOutdoorWeather;
  widgetCollection[numWidgets++] = &wOutdoorForecast;
  widgetCollection[numWidgets++] = &wCalendar;
}

void displayDashboard(bool force)
{
  // Render active widgets
  for (int i=0; i<numWidgets; i++) {
    widgetCollection[i]->render();
  }
  displayClock(force);
}

// Callback after message arriving on topic
void mqttOnMessage(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
  char *topic, *payload;
  int length;

  topic = msg->topic;
  length = msg->payloadlen;
  payload = (char *)msg->payload;

  char payloadAsChars[length];
  for (int i = 0; i < length; i++) {
    payloadAsChars[i] = (char)payload[i];
  }
  payloadAsChars[length] = '\0';

  /*
  To add:
  - Indoor PM (need to build)
  - Indoor VOC (need to build)
  - Outdoor AQI? (eh, PM is there and covers it)
  - Pressure?
  - Weather alerts
  - Calendar notifications
  - Garage door open?
  - Chores/reminders
  - Other TBD alerts?
  */

  // Home Assistant: Outdoor temperature
  if (strcmp(topic, HASS_OUT_TEMP) == 0)
  {
    showMessage(topic, payloadAsChars);
    wOutdoorWeather.updateText(payloadAsChars, tempIntHelper);
  }

  // Home Assistant: Outdoor dewpoint
  else if (strcmp(topic, HASS_OUT_DEW) == 0)
  {
    showMessage(topic, payloadAsChars);
    wOutdoorDewpoint.updateText(payloadAsChars, tempC2FHelper);
  }

  // Home Assistant: Outdoor PM2.5
  else if (strcmp(topic, HASS_OUT_PM25) == 0)
  {
    showMessage(topic, payloadAsChars);
    wOutdoorPM25.updateText(payloadAsChars, floatStrLen);
  }

  // Home Assistant: Living room temperature
  else if (strcmp(topic, HASS_LR_TEMP) == 0)
  {
    showMessage(topic, payloadAsChars);
    wHouseTemp.updateText(payloadAsChars, tempC2FHelper);
  }

  // Home Assistant: Living room dewpoint
  else if (strcmp(topic, HASS_LR_DEW) == 0)
  {
    showMessage(topic, payloadAsChars);
    wHouseDewpoint.updateText(payloadAsChars, tempC2FHelper);
  }

  // RPi Weather Station: Wind
  else if (strcmp(topic, PIWEATHER_WIND) == 0)
  {
    showMessage(topic, payloadAsChars);
    wOutdoorWind.updateText(payloadAsChars, floatStrLen);
  }

  // RPi Weather Station: Rainfall
  else if (strcmp(topic, PIWEATHER_RAINFALL) == 0)
  {
    showMessage(topic, payloadAsChars);
    wOutdoorRainGauge.updateText(payloadAsChars, floatStrLen);
  }

  // Weather: Current conditions/state
  // TODO: Animated icons?
  else if (strcmp(topic, WEATHER_NOW_STATE) == 0)
  {
    showMessage(topic, payloadAsChars);
    wOutdoorWeather.updateIcon(payloadAsChars, weatherIconHelper);
  }

  // Weather: Condition/state forecast
  //
  // TODO: Need to decouple recording of forecast data internally
  // and rendering to screen; eg: an update of forecast topic should
  // not automatically trigger display and rendering of it.  We need
  // a timer to show this at some interval, rather then whenever
  // new MQTT data is posted to the relevant topics
  //
  else if (strcmp(topic, WEATHER_FC_STATE) == 0)
  {
    showMessage(topic, payloadAsChars);
    wOutdoorForecast.updateIcon(payloadAsChars, weatherIconHelper);
  }

  // Weather: Temperature forecast
  // TODO: See above note for forecast, applies here as well
  else if (strcmp(topic, WEATHER_FC_TEMP) == 0)
  {
    showMessage(topic, payloadAsChars);

    // Note: Currently when rendering an inactive widget nothing is done
    // (eg: clearing not performed).  This allows us to avoid race
    // conditions here, where outdoorWeather is set back to active
    // before outdoorForecast is deactivated (and thus cleared,
    // resulting in a blank widget)
    wOutdoorForecast.setResetActiveTime(clock() + refreshActiveDelay * CLOCKS_PER_SEC);
    wOutdoorForecast.updateText(payloadAsChars);
    wOutdoorForecast.setActive(true);
    wOutdoorForecast.render();
    wOutdoorWeather.setActive(false);
    wOutdoorWeather.setResetActiveTime(clock() + refreshActiveDelay * CLOCKS_PER_SEC);
  }

  // "Weather": Sun position
  else if (strcmp(topic, WEATHER_SUN) == 0)
  {
    showMessage(topic, payloadAsChars);
    if (strcmp(payloadAsChars, "above_horizon") == 0)
    {
      daytime = true;
      colorText = colorDayText;
      for (int i=0; i<numWidgets; i++) {
        widgetCollection[i]->setTextColor(colorDayText);
      }

      setBrightness(50);
      displayDashboard(true);
    }
    else if (strcmp(payloadAsChars, "below_horizon") == 0)
    {
      daytime = false;
      colorText = colorNightText;
      for (int i=0; i<numWidgets; i++) {
        widgetCollection[i]->setTextColor(colorNightText);
      }

      setBrightness(25);
      displayDashboard(true);
    }
    else
      _error("unknown sun state received, skipping update");

    // Don't update icon for now, as we don't have night icons currently
    // wOutdoorWeather.updateIcon(NULL, weatherIconHelper);
  }

  // Home Assistant: HVAC state
  else if (strcmp(topic, THERMOSTAT_STATE) == 0)
  {
    showMessage(topic, payloadAsChars);
    if (strcmp(payloadAsChars, "heating") == 0)
      wHouseTemp.setIconImage(8, 7, big_house_heating);
    else if (strcmp(payloadAsChars, "cooling") == 0)
      wHouseTemp.setIconImage(8, 7, big_house_cooling);
    else if (strcmp(payloadAsChars, "idle (heat)") == 0)
      wHouseTemp.setIconImage(8, 7, big_house_mode_heat);
    else if (strcmp(payloadAsChars, "idle (cool)") == 0)
      wHouseTemp.setIconImage(8, 7, big_house_mode_cool);
    else if (strcmp(payloadAsChars, "fan_running") == 0)
      wHouseTemp.setIconImage(8, 7, big_house_fan);
    else if (strcmp(payloadAsChars, "off") == 0)
      wHouseTemp.setIconImage(8, 7, big_house);

    displayDashboard();
  }

  // Home Assistant: Calendar event
  else if (strcmp(topic, CALENDAR_EVENT) == 0)
  {
    showMessage(topic, payloadAsChars);
    wCalendar.updateText(payloadAsChars);
    displayDashboard();
  }

  // Sign: Change brightness
  else if (strcmp(topic, SIGN_BRIGHTNESS) == 0)
  {
    showMessage(topic, payloadAsChars);
    brightness = atoi((char*)payload);
    if (brightness > 100)
      brightness = 100;
    setBrightness(brightness);
    displayDashboard(true);
  }
}