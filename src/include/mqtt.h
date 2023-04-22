#ifndef MQTT_H
#define MQTT_H

#include <mosquitto.h>

// MQTT topics
#define HASS_OUT_TEMP       "homeassistant/sensor/outdoor_temperature/state"
#define HASS_OUT_DEW        "homeassistant/sensor/outdoor_dew_point/state"
#define HASS_OUT_PM25       "homeassistant/sensor/outdoor_pm_25m/state"
#define HASS_LR_TEMP        "homeassistant/sensor/living_room_temperature/state"
#define HASS_LR_DEW         "homeassistant/sensor/living_room_dew_point/state"
#define PIWEATHER_WIND      "piweather/wind_speed_mph"
#define PIWEATHER_MAX_WIND  "piweather/max_wind_speed_mph"
#define PIWEATHER_RAINFALL  "piweather/rainfall_last_hour"
#define WEATHER_FC_TEMP     "weather/forecast/temperature"
#define WEATHER_FC_STATE    "weather/forecast/state"
#define WEATHER_NOW_STATE   "weather/current/state"
#define WEATHER_SUN         "weather/sun"
#define WEATHER_ALERT       "weather/alert"
#define THERMOSTAT_STATE    "thermostat/state"
#define CALENDAR_EVENT      "calendar/event"
#define SIGN_BRIGHTNESS     "sign/brightness"
#define DEBUG_WIDGET        "debug/widget"
#define DEBUG_SCROLL_DELAY  "debug/scroll/delay"
#define DEBUG_SCROLL_STATE  "debug/scroll/state"

#define MQTT_HOST           "10.4.5.2"

#define MQTT_CLIENT_DEFAULT     "girder"
#define MQTT_CLIENT_ID_LEN      64
#define MQTT_CONNECT_WAIT       1
#define MQTT_CONNECT_WAIT_MAX   60


extern volatile bool girderRunning;
extern char mqtt_username[];
extern char mqtt_password[];


// Struct to store MQTT connection/config
struct st_mqttClient {
  mosquitto *client;
  bool connected;
  char *server;
  unsigned int port;
  unsigned int keepalive;
  char clientId[MQTT_CLIENT_ID_LEN];
};

// Prototype defs
void mqttOnMessage(struct mosquitto *, void *, const struct mosquitto_message *);
void showMessage(char *, char *);
int createMqttClient();
int mqttConnect();
void mqttShutdown();

#endif