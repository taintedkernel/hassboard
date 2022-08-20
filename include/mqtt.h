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
#define PIWEATHER_RAINFALL  "piweather/rainfall_rate_mm"
#define WEATHER_CONDITION   "weather/weather"
#define WEATHER_SUN         "weather/sun"
#define THERMOSTAT_STATE    "thermostat/state"
#define SIGN_BRIGHTNESS     "sign/brightness"

#define ICON_HOST           "10.4.3.10"

#define MQTT_CLIENT_DEFAULT "girder"
#define MQTT_CLIENT_ID_LEN  64
#define MQTT_CONNECT_WAIT   5


extern volatile bool girderRunning;
extern char mqtt_server[];
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