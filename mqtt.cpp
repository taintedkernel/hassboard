#include "mqtt.h"
#include "logger.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


// MQTT client & ID string
struct st_mqttClient mqtt;
char *clientId;


void showMessage(char *topic, char *payload)
{
  _log("message arrived on %s: %s", topic, payload);
}

// Callback after MQTT connection
void mqttOnConnect(struct mosquitto *mqttConnect, void *obj, int rc)
{
  _log("MQTT connected: rc=%d (%s)", rc, mosquitto_connack_string(rc));
  if (rc != 0) {
    _error("- exiting");
    exit(-1);
  }

  _debug("connected, subscribing to MQTT topics");
  // _debug("struct MQTT @ %p", mqttConnect);

  mqtt.connected = true;

  // Subscribe to topics for data
  mosquitto_subscribe(mqtt.client, NULL, HASS_OUT_TEMP, 0);
  mosquitto_subscribe(mqtt.client, NULL, HASS_OUT_DEW, 0);
  mosquitto_subscribe(mqtt.client, NULL, HASS_OUT_PM25, 0);
  mosquitto_subscribe(mqtt.client, NULL, HASS_LR_TEMP, 0);
  mosquitto_subscribe(mqtt.client, NULL, HASS_LR_DEW, 0);
  mosquitto_subscribe(mqtt.client, NULL, PIWEATHER_MAX_WIND, 0);
  mosquitto_subscribe(mqtt.client, NULL, PIWEATHER_RAINFALL, 0);
  mosquitto_subscribe(mqtt.client, NULL, "thermostat/#", 0);
  mosquitto_subscribe(mqtt.client, NULL, "weather/#", 0);
  mosquitto_subscribe(mqtt.client, NULL, "sign/#", 0);
  mosquitto_subscribe(mqtt.client, NULL, "icon/#", 0);
  mosquitto_subscribe(mqtt.client, NULL, "debug/#", 0);
  mosquitto_subscribe(mqtt.client, NULL, "calendar/#", 0);
}

// Create a new MQTT client
int createMqttClient()
{
  int rc;

  mqtt.port = 1883;
  mqtt.keepalive = 60;
  mqtt.server = MQTT_HOST;
  snprintf(mqtt.clientId, MQTT_CLIENT_ID_LEN, "%s-%d", MQTT_CLIENT_DEFAULT, rand() % 65536);

  _debug("New MQTT connect client ID: %s", mqtt.clientId);
  mqtt.client = mosquitto_new(mqtt.clientId, true, NULL);
  if (mqtt.client == NULL)
  {
    _error("unable to initialize new MQTT client, aborting");
    return false;
  }

  rc = mosquitto_username_pw_set(mqtt.client, mqtt_username, mqtt_password);
  if (rc)
  {
    _error("unable to configure MQTT credentials, aborting: rc=%d (%s)", rc, mosquitto_strerror(rc));
    return false;
  }

  mosquitto_connect_callback_set(mqtt.client, mqttOnConnect);
  mosquitto_message_callback_set(mqtt.client, mqttOnMessage);

  return true;
}

// Connect to a local MQTT server that provides all the data
// This will retry forever until connected or interrupted
int mqttConnect()
{
  int rc, mqRetries = 1;
  int connectWait = MQTT_CONNECT_WAIT;

  // Loop while we're not connected to MQTT and system is running
  while (!mqtt.connected && girderRunning)
  {
    _debug("MQTT connect attempt %d", mqRetries);
    rc = mosquitto_connect(mqtt.client, mqtt.server, mqtt.port, mqtt.keepalive);
    if (rc == MOSQ_ERR_SUCCESS)
    {
      // We set connected = true here in order to prevent the main loop
      // from reconnecting; if the actual connection isn't established
      // (on_connect callback) then an error will be caught and connection
      // state will reset to false
      mqtt.connected = true;
      return true;
    }

    _error("MQTT connection failed, attempt %d: rc=%d (%s)", mqRetries++, rc, mosquitto_strerror(rc));
    sleep(connectWait);

    // TODO: In event of repeated failures, try to create new client and
    // reset libraries/connections/etc from scratch
    if (connectWait < MQTT_CONNECT_WAIT_MAX)
      connectWait++;
  }

  return true;
}

void mqttShutdown()
{
  mosquitto_loop_stop(mqtt.client, true);
  mosquitto_disconnect(mqtt.client);
  mosquitto_destroy(mqtt.client);
  mosquitto_lib_cleanup();
}