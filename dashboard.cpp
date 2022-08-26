#include "dashboard.h"
#include "logger.h"
#include "widget.h"
#include "icons.h"
#include "mqtt.h"

#include <led-matrix.h>
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
uint8_t rowDayStart = 2;
uint8_t rowDateStart = rowDayStart + 10;
uint8_t rowTimeStart = rowDateStart + 10;

uint8_t colOneStart = dashOffset + 1;
uint8_t colOneEnd = dashOffset + 30;
uint8_t colTwoStart = dashOffset + 34;
uint8_t colTwoEnd = dashOffset + 63;

uint8_t rowSpacing = 3;
uint8_t rowOneStart = 1;
uint8_t rowTwoStart = rowOneStart + FONT_HEIGHT + rowSpacing;
uint8_t rowThreeStart = rowTwoStart + FONT_HEIGHT + rowSpacing;

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

// TODO: fix this, eg: add a widget manager
DashboardWidget *widget, *widgetCollection[MAX_WIDGETS];

rgb_matrix::Font *customFont;

extern uint32_t cycle;
extern bool forceRefresh;
extern rgb_matrix::RGBMatrix *matrix;
extern rgb_matrix::Color colorWhite;
extern rgb_matrix::Font *defaultFont;


void setupDashboard()
{
  _log("configuring dashboard");

  // Row 1
  widget = &wHouseTemp;
  widget->setOrigin(colOneStart-1, rowOneStart);
  widget->setSize(DashboardWidget::WIDGET_SMALL);
  widget->autoTextConfig();
  widget->setIconImage(8, 7, big_house);
  widget->setIconConfig(0, 1);

  widget = &wHouseDewpoint;
  widget->setOrigin(colTwoStart, rowOneStart);
  widget->setSize(DashboardWidget::WIDGET_SMALL);
  widget->autoTextConfig();
  widget->setIconImage(8, 7, big_house_drop);
  widget->setIconConfig(0, 1);

  // Row 2
  widget = &wOutdoorRainGauge;
  widget->setOrigin(colOneStart, rowTwoStart);
  widget->setSize(DashboardWidget::WIDGET_SMALL);
  widget->autoTextConfig();
  // widget->updateText((char *)"0.0", 0);
  widget->setIconImage(8, 8, rain_gauge);

  widget = &wOutdoorDewpoint;
  widget->setOrigin(colTwoStart, rowTwoStart);
  widget->setSize(DashboardWidget::WIDGET_SMALL);
  widget->autoTextConfig();
  widget->setIconImage(8, 8, droplet);

  // Row 3
  widget = &wOutdoorWind;
  widget->setOrigin(colOneStart, rowThreeStart);
  widget->setSize(DashboardWidget::WIDGET_SMALL);
  widget->autoTextConfig();
  widget->setIconImage(8, 8, wind);

  widget = &wOutdoorPM25;
  widget->setOrigin(colTwoStart, rowThreeStart);
  widget->setSize(DashboardWidget::WIDGET_SMALL);
  widget->autoTextConfig();
  widget->setIconImage(7, 8, air);
  // widget->setTextConfig(WIDGET_WIDTH_SMALL, 0);

  // Primary weather row (2nd display)
  widget = &wOutdoorWeather;
  customFont = new rgb_matrix::Font;
  customFont->LoadFont(FONT_FILE_2);
  _debug("loaded custom font @ 0x%p", customFont);
  widget->setOrigin(weatherOffset, 0);
  widget->setSize(DashboardWidget::WIDGET_XLARGE);
  widget->setIconImage(32, 25, clouds);
  widget->setCustomTextConfig(WIDGET_WIDTH_XLARGE, 26,
    colorWhite, DashboardWidget::ALIGN_CENTER, customFont);
  widget->setBounds(32, 34);

  widgetCollection[numWidgets++] = &wHouseTemp;
  widgetCollection[numWidgets++] = &wHouseDewpoint;
  widgetCollection[numWidgets++] = &wOutdoorRainGauge;
  widgetCollection[numWidgets++] = &wOutdoorDewpoint;
  widgetCollection[numWidgets++] = &wOutdoorWind;
  widgetCollection[numWidgets++] = &wOutdoorPM25;
  widgetCollection[numWidgets++] = &wOutdoorWeather;
}

void displayDashboard(unsigned int updatedData)
{
  // Render active widgets
  for (int i=0; i<numWidgets; i++) {
    widgetCollection[i]->render();
  }
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
  - Other TBD alerts?
  */

  // Need to fix in HASS, outdoor temp is different and currently stored in F
  if (strcmp(topic, HASS_OUT_TEMP) == 0)
  {
    showMessage(topic, payloadAsChars);
    wOutdoorWeather.updateText(payloadAsChars, tempIntHelper, cycle);
  }

  else if (strcmp(topic, HASS_OUT_DEW) == 0)
  {
    showMessage(topic, payloadAsChars);
    wOutdoorDewpoint.updateText(payloadAsChars, tempC2FHelper, cycle);
  }

  else if (strcmp(topic, HASS_OUT_PM25) == 0)
  {
    showMessage(topic, payloadAsChars);
    wOutdoorPM25.updateText(payloadAsChars, floatStrLen, cycle);
  }

  else if (strcmp(topic, HASS_LR_TEMP) == 0)
  {
    showMessage(topic, payloadAsChars);
    wHouseTemp.updateText(payloadAsChars, tempC2FHelper, cycle);
  }

  else if (strcmp(topic, HASS_LR_DEW) == 0)
  {
    showMessage(topic, payloadAsChars);
    wHouseDewpoint.updateText(payloadAsChars, tempC2FHelper, cycle);
  }

  else if (strcmp(topic, PIWEATHER_WIND) == 0)
  {
    showMessage(topic, payloadAsChars);
    wOutdoorWind.updateText(payloadAsChars, floatStrLen, cycle);
  }

  else if (strcmp(topic, PIWEATHER_RAINFALL) == 0)
  {
    showMessage(topic, payloadAsChars);
    wOutdoorRainGauge.updateText(payloadAsChars, floatStrLen, cycle);
  }

  // TODO: Animated icons?
  else if (strcmp(topic, WEATHER_CONDITION) == 0)
  {
    showMessage(topic, payloadAsChars);
    wOutdoorWeather.updateIcon(payloadAsChars, weatherIconHelper);
  }

  else if (strcmp(topic, WEATHER_SUN) == 0)
  {
    showMessage(topic, payloadAsChars);
    if (strcmp(payloadAsChars, "above_horizon") == 0)
      daytime = true;
    else if (strcmp(payloadAsChars, "below_horizon") == 0)
      daytime = false;
    else
      _error("unknown sun state received, skipping update");
    wOutdoorWeather.updateIcon(NULL, weatherIconHelper);
  }

  else if (strcmp(topic, SIGN_BRIGHTNESS) == 0)
  {
    showMessage(topic, payloadAsChars);
    brightness = atoi((char*)payload);
    if (brightness > 100)
      brightness = 100;
    matrix->SetBrightness(brightness);
    forceRefresh = true;
  }

  else if (strcmp(topic, THERMOSTAT_STATE) == 0)
  {
    showMessage(topic, payloadAsChars);
    if (strcmp(payloadAsChars, "heating") == 0) {
      wHouseTemp.setIconImage(8, 7, big_house_heat);
    } else if (strcmp(payloadAsChars, "cooling") == 0) {
      wHouseTemp.setIconImage(8, 7, big_house_cool);
    } else if (strcmp(payloadAsChars, "fan_running") == 0) {
      wHouseTemp.setIconImage(8, 7, big_house_fan);
    } else if (strcmp(payloadAsChars, "idle") == 0) {
      wHouseTemp.setIconImage(8, 7, big_house);
    }
    displayDashboard();
  }

  else if (strcmp(topic, "icon/update") == 0)
  {
    char *token;
    char delim[] = " ";

    unsigned int iconNameLen = 16, urlLen = 32;
    char iconName[iconNameLen+1], url[urlLen+1];
    // uint8_t buffer[ICON_SZ_BYTES];

    showMessage(topic, payloadAsChars);
    token = strtok(payloadAsChars, delim);
    strncpy(iconName, token, iconNameLen);
    token = strtok(NULL, delim);
    strncpy(url, token, urlLen);
    _log("retrieving new icon %s from %s", iconName, url);

    /* if (httpGet(url, buffer, ICON_SZ_BYTES))
    {
      if (strcmp(iconName, "sun") == 0) {
        memcpy(sunLocal, buffer, ICON_SZ_BYTES);
      } else if (strcmp(iconName, "clouds") == 0) {
        memcpy(cloudsLocal, buffer, ICON_SZ_BYTES);
      } else if (strcmp(iconName, "cloudssun") == 0) {
        memcpy(cloudsSunLocal, buffer, ICON_SZ_BYTES);
      } else if (strcmp(iconName, "cloudsshowers") == 0) {
        memcpy(cloudsShowersLocal, buffer, ICON_SZ_BYTES);
      } else {
        _error("unknown icon name, not updating");
        return;
      }

      wOutdoorWeather.updateIcon(NULL, weatherIconHelper);
    } */
  }

  else if (strcmp(topic, "icon/brightness") == 0)
  {
    showMessage(topic, payloadAsChars);
    iconExtraBrightness = atoi(payloadAsChars);
  }

  /* else if (strcmp(topic, "debug/color/min_red") == 0)
  {
    showMessage(topic, payloadAsChars);
    // colorRedMin = atoi(payloadAsChars);
    forceRefresh = true;
    wOutdoorWeather.updateIcon(NULL, weatherIconHelper);
}

  else if (strcmp(topic, "debug/color/min_green") == 0)
  {
    showMessage(topic, payloadAsChars);
    // colorGreenMin = atoi(payloadAsChars);
    forceRefresh = true;
    wOutdoorWeather.updateIcon(NULL, weatherIconHelper);
  }

  else if (strcmp(topic, "debug/color/min_blue") == 0)
  {
    showMessage(topic, payloadAsChars);
    // colorBlueMin = atoi(payloadAsChars);
    forceRefresh = true;
    wOutdoorWeather.updateIcon(NULL, weatherIconHelper);
  }

  else if (strcmp(topic, "debug/color/min_all") == 0)
  {
    showMessage(topic, payloadAsChars);
    // colorRedMin = atoi(payloadAsChars);
    // colorGreenMin = atoi(payloadAsChars);
    // colorBlueMin = atoi(payloadAsChars);
    forceRefresh = true;
    wOutdoorWeather.updateIcon(NULL, weatherIconHelper);
  } */
}