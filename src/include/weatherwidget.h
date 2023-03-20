#ifndef WEATHERWIDGET_H
#define WEATHERWIDGET_H

#include "dynamicwidget.h"
#include "datetime.h"
#include "weather.h"
#include "logger.h"

#include <time.h>
#include <string.h>
#include <random>
#include <string>


using rgb_matrix::Color;
using std::vector;
using std::string;

uint16_t imgIndex(uint8_t x, uint8_t y, uint8_t width);
Color getRandomColor(vector<string>);


/*
 weather conditions/types:
 - sunny, cloudy, rainy, snowy, etc

 animation types:
 - drop animation:
   - rain, snow
 - sun/ray animation: (idea)
   - sunny
 - wind animation: (idea)
   - windy
 - other animations?

 additional weather condition metadata:
 - icon filename
 - animation type
 - color palette
*/

struct Bounds {
  uint8_t xTop, yTop;
  uint8_t xBot, yBot;
  Bounds() : xTop(0), yTop(0), xBot(0), yBot(0) {}
  Bounds(uint8_t xT, uint8_t yT, uint8_t xB, uint8_t yB) :
    xTop(xT), yTop(yT), xBot(xB), yBot(yB) {}
};

struct Pixel {
  uint8_t x, y;
  uint8_t num;
  Color c;
  Pixel() : x(0), y(0), num(0), c(Color(0,0,0)) {}
  Pixel(uint8_t x, uint8_t y, uint8_t n, Color c) :
    x(x), y(y), num(n), c(c) {}
};

struct AnimatedConfig {
  uint8_t *image;
  const uint8_t *origImage;
  uint8_t width;
  Bounds *bounds;
  weatherType weather;
};

class GenericDrop
{
private:
  uint8_t x, y, id;
  vector<Pixel> pixels;
  vector<string> colors;
  AnimatedConfig *config;

public:
  GenericDrop(uint8_t x, uint8_t y, vector<string> colors) :
      x(x), y(y), colors(colors) {
    init(3);
  }
  GenericDrop(uint8_t x, uint8_t y, uint8_t size,
      uint8_t id, AnimatedConfig *newConfig) :
      x(x), y(y), id(id), config(newConfig) {
    colors = weatherColorsLookup(config->weather);
    init(size);
  }
  auto size() {
    return pixels.size();
  }

  // Build a stacked set of several pixels,
  // starting at our drop origin and growing downwards
  void init(uint8_t dropSize)
  {
    auto yP = y;

    // Use a fixed size of 3 pixels for now
    // Pull a random color from our color list
    for (auto i=0; i<dropSize; i++) {
      // _debug("new pixel %d (count before=%d):", size(), i);
      Pixel pixel(x, yP--, i, getRandomColor(colors));
      pixels.push_back(pixel);
    }

    render();
  }

  void render() {
    for (vector<Pixel>::iterator pxl = pixels.begin();
      pxl != pixels.end(); pxl++)
    {
      if (pxl->y < config->bounds->yTop ||
          pxl->y > config->bounds->yBot) continue;
      uint16_t index = imgIndex(pxl->x, pxl->y, config->width);
      config->image[index] = pxl->c.r;
      config->image[index+1] = pxl->c.g;
      config->image[index+2] = pxl->c.b;
    }
  }

  // Shift all pixels down a row, removing ones
  // outside of our bounds
  void move()
  {
    uint16_t index;
    // _debug("GenericDrop::move()");

    // Clear pixels (copy data from original image)
    for (vector<Pixel>::iterator pxl = pixels.begin();
        pxl != pixels.end(); pxl++)
    {
      index = imgIndex(pxl->x, pxl->y, config->width);
      // _debug("drop #%d: pixel #%d (%6d @ %d,%d) r=%3d, "\
      //   "g=%3d, b=%3d", id, pxl->num, index, pxl->x, pxl->y,
      //   config->image[index], config->image[index+1], 
      //   config->image[index+2]);
      config->image[index] = config->origImage[index];
      config->image[index+1] = config->origImage[index+1];
      config->image[index+2] = config->origImage[index+2];
    }

    // Check bounds, move and render at new location
    for (vector<Pixel>::iterator pxl = pixels.begin();
        pxl != pixels.end();)
    {
      // Remove the pixel if it lies outside our bounds
      if (pxl->y >= config->bounds->yBot)
      {
        pxl = pixels.erase(pxl);
        // pixels.erase(--(pxl.base()));
        // _debug("  pixel outside bounds "\
        //     "(%d,%d)-(%d,%d), removing", config->bounds->xTop,
        //     config->bounds->yTop, config->bounds->xBot,
        //     config->bounds->yBot);
      }
      else
      {
        // Update the position, then set the pixel color
        // in the new location
        pxl->y += 1;
        index = imgIndex(pxl->x, pxl->y, config->width);

        if (pxl->y >= config->bounds->yTop &&
            pxl->y <= config->bounds->yBot)
        {
          config->image[index] = pxl->c.r;
          config->image[index+1] = pxl->c.g;
          config->image[index+2] = pxl->c.b;
        }

        // _debug("  pixel moved (%6d @ %d,%d) r=%3d, g=%3d, "\
        //     "b=%3d", index, pxl->x, pxl->y,
        //     config->image[index], config->image[index+1], 
        //     config->image[index+2]);
        
        pxl++;
      }
    }

  }
};


// [===--- DropWidget ---===]
class DropWidget
{
private:
  AnimatedConfig *config;
  std::random_device rd;
  std::mt19937 gen;
  vector<GenericDrop> drops;
  uint8_t dropId = 0;
  std::uniform_int_distribution<> *xCloud,
      *yUnderCloud, *yInCloud, *dropSize;

public:
  DropWidget() {
    gen.seed(rd());
  }

  void configDrop(AnimatedConfig *animConfig)
  {
    drops.clear();
    config = animConfig;
    xCloud = new std::uniform_int_distribution<>(config->bounds->xTop, config->bounds->xBot);
    yUnderCloud = new std::uniform_int_distribution<>(config->bounds->yTop, config->bounds->yBot);
    yInCloud = new std::uniform_int_distribution<>(config->bounds->yTop-5, config->bounds->yTop);
    dropSize = new std::uniform_int_distribution<>(1,3);
  }

  // Create a drop at a random location within our bounds
  void addDrop(bool underCloud = false)
  {
    if (!config) {
      _error("addDrop() called without config set, aborting!");
      return;
    }

    auto x = (uint8_t)(*xCloud)(gen);
    auto y = (underCloud) ? (uint8_t)(*yUnderCloud)(gen) : (uint8_t)(*yInCloud)(gen);
    auto size = (uint8_t)(*dropSize)(gen);

    // _debug("DropWidget::addDrop(%d,%d).start; drops.size=%d",
    //   x, y, drops.size());
    GenericDrop *drop = new GenericDrop(x, y, size, dropId++, config);
    drops.push_back(*drop);
    // _debug("DropWidget::addDrop(%d,%d).end; drops.size=%d",
    //   x, y, drops.size());
    // _debug("");
  }

  // Called via checkUpdate in AnimatedWidget parent class
  void updateAnimation()
  {
    if (!config) {
      _error("updateAnimation() called without config set,"\
        "aborting!");
      return;
    }
    
    // _debug("DropWidget::updateAnimation() enter drops size=%d", drops.size());

    // Iterate through our drops
    uint8_t removedDrops = 0;
    for (vector<GenericDrop>::iterator drop = drops.begin();
      drop != drops.end();)
    {
      // Move the drop, removing pixels that
      // lie outside our bounds
      drop->move();

      // If no pixels are left in our drop, then remove it
      if (drop->size() == 0)
      {
        drop = drops.erase(drop);
        removedDrops++;
        // _debug("all drop pixels removed, removing drop");
      }
      else drop++;
    }
    // _debug("%d drops removed (now at %d), adding new",
    //     removedDrops, drops.size());

    for (auto i=0; i<removedDrops; i++)
      addDrop();

    // _debug("  exit drops size=%d", drops.size());
  }
};

class SunWidget
{
private:
  uint8_t width;
  vector<int> rays;
};

// [===--- RainWidget ---===]
class RainWidget : public DropWidget
{
public:
  static const uint8_t numDrops = 16;
  Bounds bounds = Bounds(7, 15, 28, 23);

  // Prepare an animation
  void config(AnimatedConfig *animConfig)
  {
    _debug("RainWidget::config()");
    animConfig->bounds = &bounds;
    configDrop(animConfig);

    // Create some drops
    for (auto i=0; i<numDrops; i++) {
      addDrop(true);
    }
  }
};

// [===--- WeatherWidget ---===]
class WeatherWidget : public AnimatedWidget
{
public:
  enum weatherMotionType{MOTION_NONE, MOTION_RAIN, MOTION_SNOW};

private:
  const uint8_t *iOrigImage;
  weatherType currentWeather;
  AnimatedConfig animConfig;

  time_t lastImageTime = 0;
  uint16_t imageUpdatePeriod = FRAME_UPDATE_PERIOD_MS / 1000;

  // std::map<weatherType, CustomWeatherWidget> weatherWidgetMap;
  RainWidget rainWidget;
  SunWidget sunWidget;

public:
  WeatherWidget(const char *name) : AnimatedWidget(name) {
    // weatherWidgetMap[WEATHER_RAINY] = rainWidget;
  }

  // Find our weather enum type from a NWS condition
  // string and update our widget based on that 
  void updateWeather(const char *nws, bool daytime)
  {
    // _debug("WeatherWidget::updateWeather()");
    // _debug("nws=%s", nws);
    weatherType weather = nwsWeatherTypeLookup(nws,
        (daytime) ? DAY_TIME : NIGHT_TIME);
    updateWeather(weather);
  }

  // Lookup our icon filename from our
  // weather type and load the icon
  void updateWeather(weatherType weather)
  {
    currentWeather = weather;
    string iconName = weatherIconLookup(weather);

    // _debug("updateWeather(%s)", printWeather(weather));
    // _debug("new icon=%s", iconName.c_str());
    updateIcon(iconName.c_str());
    initAnimation();

    // iImage & iWidth set by updateIcon() above
    // iOrigImage created by initAnimation() above
    animConfig = {(uint8_t *)iImage, iOrigImage, iWidth,
        NULL, currentWeather};

    switch (currentWeather) {
    case WEATHER_RAINY:
      rainWidget.config(&animConfig);
      break;
    default:
      break;
    }
  }

  // Prepare an animation
  // Must be called after setting icon
  void initAnimation()
  {
    _debug("WeatherWidget::initAnimation()");

    if (!iInit)
      return;

    // Make a copy of our icon image
    // This is referenced as the background
    // layer when we do our animation effects
    iOrigImage = new uint8_t[iWidth * iHeight * 3];
    if (!iOrigImage)
      return;
    memcpy((void *)iOrigImage, iImage, iWidth * iHeight * 3);

    // initAnimation();
    aInit = true;
  }

  void doImageUpdate()
  {
    // _debug("WeatherWidget::doImageUpdate()");
    if (!aInit) return;

    switch (currentWeather) {
    case WEATHER_RAINY:
      rainWidget.updateAnimation();
      break;
    default:
      break;
    }

    render();
  }
};

#endif