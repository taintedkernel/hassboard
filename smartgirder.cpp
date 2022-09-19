#include <mosquitto.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>

#include <string>
#include <cerrno>

#include "logger.h"
#include "display.h"
#include "dashboard.h"
#include "mqtt.h"
// #include "datetime.h"
#include "widget.h"


// Various vars for main functions
volatile bool girderRunning = true;
bool forceRefresh = true;

uint32_t cycle = 0;
uint32_t refreshCycle = 0;

extern st_mqttClient mqtt;
extern uint8_t numWidgets;
extern DashboardWidget* widgetCollection[];


void handleSignal(int signal)
{
    girderRunning = false;
    printf("shutting down...\n");
}

int main(int argc, char **argv)
{
  int rc;

  _log("starting up");

  signal(SIGINT, handleSignal);
  signal(SIGTERM, handleSignal);
  srand((unsigned) time(NULL));

  // Display initialization
  if (!setupDisplay()) {
    _error("failed to initialize display, exiting");
    return 1;
  }
  setupDashboard();

  // MQTT initialization
  // drawIcon(weatherOffset+32+3, 0+3, 25, 25, mqtt);
  mqtt.connected = false;
  if (!createMqttClient())
  {
    _error("unable to build client, exiting");
    girderRunning = false;
  }
  else
  {
    mosquitto_lib_init();
    mqttConnect();
  }

  /*
    ----==== [ Main Loop ] ====----
  */
  while (girderRunning)
  {
    // TODO: Stop tracking durations this way, use the actual clock
    cycle++;

    // Connect to MQTT if necessary
    mqttConnect();

    // MQTT loop to pick up messages
    // _debug("calling mqtt_loop");
    rc = mosquitto_loop(mqtt.client, 200, 1);
    if (rc)
    {
      mqtt.connected = false;

      _error("MQTT connection error, rc=%d", rc);
      if (rc == MOSQ_ERR_ERRNO)
      {
        _error("got errno %d on system call", errno);
        girderRunning = false;
        continue;
      }

      _error("reconnecting");
      sleep(MQTT_CONNECT_WAIT);
      mosquitto_reconnect(mqtt.client);
      continue;
    }

    // Show the clock and update it as needed
    displayClock();

    // Reset temporary brightness for widgets, if necessary
    for (int i=0; i<numWidgets; i++) {
      widgetCollection[i]->checkResetBrightness();
      widgetCollection[i]->checkResetActive();
    }

    // Force refresh of the display
    if (forceRefresh)
    {
      _log("forcing dashboard refresh");
      // _debug("widget count: %d", numWidgets);

      // Recalculate brightness for widgets, assuming there was an
      // update to the global value if a force refresh was done
      for (int i=0; i<numWidgets; i++) {
        widgetCollection[i]->updateBrightness();
      }

      displayClock(true);
      displayDashboard();
      forceRefresh = false;
    }

    // Pause between cycles, after startup has finished
    if (clock() > 5.0 * CLOCKS_PER_SEC) {
      sleep(0.1);
    }

    // Scroll text, if necessary
    for (int i=0; i<numWidgets; i++) {
    }
  }

  _log("closing matrix");
  shutdownDisplay();
  mqttShutdown();

  return 0;
}