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
#include "widgetmanager.h"


// Various vars for main functions
volatile bool girderRunning = true;
bool forceRefresh = true;

uint32_t cycle = 0;
uint32_t refreshCycle = 0;

extern st_mqttClient mqtt;
extern uint8_t numWidgets;
extern WidgetManager widgets;


void handleSignal(int signal)
{
    girderRunning = false;
    printf("shutting down...\n");
}

int main(int argc, char **argv)
{
  int rc; //, opt;
  uint8_t configNum = 0;

  _log("starting up");

  signal(SIGINT, handleSignal);
  signal(SIGTERM, handleSignal);
  srand((unsigned) time(NULL));

  // while ((opt = getopt(argc, argv, "abc:")) != -1)
  // {
  //   switch (opt) {
  //   case 'c':
  //     cvalue = optarg;
  //     break;
  //   case '?':
  //     if (optopt == 'c')
  //       fprintf (stderr, "Option -%c requires an argument.\n", optopt);
  //     else if (isprint (optopt))
  //       fprintf (stderr, "Unknown option `-%c'.\n", optopt);
  //     else
  //       fprintf (stderr,
  //                 "Unknown option character `\\x%x'.\n",
  //                 optopt);
  //     return 1;
  //   default:
  //     abort ();
  //   }
  // }

  for (int index = optind; index < argc; index++)
  {
    char *arg = argv[index];
    if (atoi(arg) < 1 || atoi(arg) > 2) {
      fprintf(stderr, "missing required parameter CONFIG_NUM\n");
      return 1;
    }
    else {
      configNum = atoi(arg);
      _log("using girder display config %d", configNum);
      break;
    }
  }

  // Display initialization
  if (!setupDisplay(configNum)) {
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
    // Recalculate brightness for widgets, assuming there was an
    // update to the global value if a force refresh was done
    widgets.checkResetUpdateBrightness(forceRefresh);

    // Force refresh of the display
    if (forceRefresh)
    {
      _log("forcing dashboard refresh");

      displayClock(true);
      displayDashboard();
      forceRefresh = false;
    }

    // Pause between cycles, after startup has finished
    if (clock() > 5.0 * CLOCKS_PER_SEC) {
      sleep(0.1);
    }

    // Update any dynamic widgets
    widgets.checkUpdate();
  }

  _log("closing matrix");
  shutdownDisplay();
  mqttShutdown();

  return 0;
}