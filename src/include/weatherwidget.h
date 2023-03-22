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
#include <functional>
#include <any>


using rgb_matrix::Color;
using std::vector;
using std::string;

typedef std::uniform_int_distribution<> distrib;

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
  bool checkBounds(Bounds *b) {
    return (checkXBounds(b) && checkYBounds(b));
  }
  bool checkYBounds(Bounds *b) {
    return (y >= b->yTop && y <= b->yBot);
  }
  bool checkXBounds(Bounds *b) {
    return (x >= b->xTop && x <= b->xBot);
  }
  void render(uint8_t *img, uint8_t width) {
    uint16_t index = imgIndex(x, y, width);
    img[index]   = c.r;
    img[index+1] = c.g;
    img[index+2] = c.b;
  }
};

struct AnimatedConfig {
  uint8_t *image;
  const uint8_t *origImage;
  uint8_t width;
  Bounds *b;
  weatherType weather;
};

class GenericDrop
{
private:
  uint8_t x, y, id;
  vector<Pixel> pixels;
  vector<string> colors;
  AnimatedConfig *conf;

public:
  GenericDrop(uint8_t x, uint8_t y, vector<string> colors) :
      x(x), y(y), colors(colors) {
    init(3);
  }
  GenericDrop(uint8_t x, uint8_t y, uint8_t size,
      uint8_t id, AnimatedConfig *newConf) :
      x(x), y(y), id(id), conf(newConf) {
    colors = weatherColors(conf->weather);
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
      pxl != pixels.end(); pxl++) {
        if (pxl->checkBounds(conf->b))
          pxl->render(conf->image, conf->width);
    }
  }

  // Shift all pixels down a row, removing ones
  // outside of our bounds
  void move()
  {
    uint16_t index;
    vector<Pixel>::iterator pxl;
    // _debug(__METHOD__);

    // Clear pixels (copy data from original image)
    for (pxl = pixels.begin(); pxl != pixels.end(); pxl++)
    {
      index = imgIndex(pxl->x, pxl->y, conf->width);
      // _debug("drop #%d: pixel #%d (%6d @ %d,%d) r=%3d, "
      //   "g=%3d, b=%3d", id, pxl->num, index, pxl->x, pxl->y,
      //   conf->image[index], conf->image[index+1], 
      //   conf->image[index+2]);
      memcpy(conf->image+index, conf->origImage+index, sizeof(uint8_t)*3);
    }

    // Check bounds, move and render at new location
    for (pxl = pixels.begin(); pxl != pixels.end();)
    {
      // Remove the pixel if it lies outside our bounds
      if (pxl->y >= conf->b->yBot) {
        pxl = pixels.erase(pxl);
        // _debug("  pixel outside bounds "
        //     "(%d,%d)-(%d,%d), removing", conf->b->xTop,
        //     conf->b->yTop, conf->b->xBot,
        //     conf->b->yBot);
      }
      // Update the position, then set the pixel color
      // in the new location
      else {
        pxl->y += 1;
        if (pxl->checkBounds(conf->b)) {
          pxl->render(conf->image, conf->width);
        }
        // _debug("  pixel moved (%6d @ %d,%d) r=%3d, g=%3d, "
        //     "b=%3d", index, pxl->x, pxl->y,
        //     conf->image[index], conf->image[index+1], 
        //     conf->image[index+2]);
        pxl++;
      }
    }

  }
};


// [===--- DropWidget ---===]
class DropWidget
{
private:
  AnimatedConfig *conf;
  uint8_t dropId = 0;
  vector<GenericDrop> drops;
  std::random_device rd;
  std::mt19937 gen;
  distrib *xCloud, *yUnderCloud, *yInCloud, *dropSize;

public:
  DropWidget() { gen.seed(rd()); }

  void configDrop(AnimatedConfig *animConf)
  {
    drops.clear();
    conf = animConf;
    xCloud = new distrib(conf->b->xTop, conf->b->xBot);
    yUnderCloud = new distrib(conf->b->yTop, conf->b->yBot);
    yInCloud = new distrib(conf->b->yTop-5, conf->b->yTop);
    dropSize = new distrib(2, 3);
  }

  // Create a drop at a random location within our bounds
  // TODO: Add some logic to prevent "clustering" (eg: ensure
  // semi-uniform distribution, maybe with a mandatory spacing)
  void addDrop(bool underCloud = false)
  {
    if (!conf) {
      _error("%s called without config set, aborting!",
          __METHOD__);
      return;
    }

    auto x = (uint8_t)(*xCloud)(gen);
    auto y = (underCloud) ? (uint8_t)(*yUnderCloud)(gen) : 
        (uint8_t)(*yInCloud)(gen);
    auto size = (uint8_t)(*dropSize)(gen);

    // _debug("DropWidget::addDrop(%d,%d).start; drops.size=%d",
    //   x, y, drops.size());
    GenericDrop *drop = new GenericDrop(
        x, y, size, dropId++, conf
    );
    drops.push_back(*drop);
    // _debug("DropWidget::addDrop(%d,%d).end; drops.size=%d",
    //   x, y, drops.size());
  }

  // Called via checkUpdate in AnimatedWidget parent class
  void updateAnimation()
  {
    if (!conf) {
      _error("%s called without config set,"\
          "aborting!", __METHOD__);
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
      }
      else drop++;
    }
    // _debug("%d drops removed (now at %d), adding new",
    //     removedDrops, drops.size());

    for (auto i=0; i<removedDrops; i++) addDrop();
  }
};

class AConfig
{
public:
  // uint8_t *image;
  // const uint8_t *origImage;
  // uint8_t width;
  // Bounds *b;
  // weatherType weather;
  AnimatedConfig *conf;

  void config(AnimatedConfig *animConf) {
    conf = animConf;
  }
  // float getUpdatePeriod() { return 1.0f; }
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
  Bounds bounds = Bounds(8, 15, 27, 23);
  uint16_t imageUpdatePeriodMs = 800;

  // Prepare an animation
  void config(AnimatedConfig *animConf)
  {
    _debug(__METHOD__);
    animConf->b = &bounds;
    configDrop(animConf);

    // Create some drops
    for (auto i=0; i<numDrops; i++) {
      addDrop(true);
    }
  }

  float getUpdatePeriod() {
    return imageUpdatePeriodMs / 1000.0;
  }
};

// [===--- WeatherWidget ---===]
class WeatherWidget : public AnimatedWidget
{
public:
  enum weatherMotionType{MOTION_NONE, MOTION_RAIN, MOTION_SNOW};

private:
  const uint8_t *iImageOrig;
  weatherType weather;
  AnimatedConfig animConf;

  time_t lastImageTime = 0;
  uint16_t imageUpdatePeriod = FRAME_UPDATE_PERIOD_MS / 1000;

  std::map<weatherType, std::any> widgetMap;
  RainWidget rainWidget;
  SunWidget sunWidget;

public:
  WeatherWidget(const char *name) : AnimatedWidget(name) {
    // widgetMap[WEATHER_RAINY] = rainWidget;
  }

  // Find our weather enum type from a NWS condition
  // string and update our widget based on that 
  void updateWeather(const char *nws, bool daytime)
  {
    // _debug(__METHOD__);
    // _debug("nws=%s", nws);
    updateWeather(
      nwsWeatherTypeLookup(nws, (daytime) ?
        DAY_TIME : NIGHT_TIME)
    );
  }

  // Lookup our icon filename from our
  // weather type and load the icon
  void updateWeather(weatherType weatherUpdate)
  {
    weather = weatherUpdate;
    string iconName = weatherIconLookup(weather);

    _debug(__METHOD_ARG__(weatherStr(weather)));
    // _debug("new icon=%s", iconName.c_str());
    updateIcon(iconName);
    initAnimation();

    // iImage & iWidth set by updateIcon() above
    // iImageOrig created by initAnimation() above
    animConf = {(uint8_t *)iImage, iImageOrig, iWidth,
        NULL, weather};

    switch (weather) {
    case WEATHER_RAINY:
      rainWidget.config(&animConf);
      setImageUpdatePeriod(rainWidget.getUpdatePeriod());
      break;
    default:
      break;
    }
  }

  // Prepare an animation
  // Must be called after setting icon
  void initAnimation()
  {
    if (!iInit) return;
    // _debug(__METHOD__);

    // Make a copy of our icon image
    // This is referenced as the background
    // layer when we do our animation effects
    auto size = iWidth * iHeight * 3;
    iImageOrig = new uint8_t[size];
    if (!iImageOrig) return;
    memcpy((void *)iImageOrig, iImage, size);

    aInit = true;
  }

  void doImageUpdate()
  {
    if (!aInit) return;

    switch (weather) {
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