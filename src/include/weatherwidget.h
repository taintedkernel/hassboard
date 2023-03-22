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
#include <tuple>


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

class AnimatedConfig
{
public:
  uint8_t *image;
  const uint8_t *origImage;
  uint8_t imgWidth;
  Bounds bounds;
  weatherType weather;

  AnimatedConfig() {}
  AnimatedConfig(uint8_t *i, const uint8_t *o,
    uint8_t w, weatherType wthr) : image(i), origImage(o),
    imgWidth(w), weather(wthr) {};  

  void setBounds(const Bounds &b) { bounds = b; }

  // Return bounds as a tuple for readability
  std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> getBounds()
  {
    return std::make_tuple(
      bounds.xTop, bounds.yTop,
      bounds.xBot, bounds.yBot
    );
  }
};

struct Pixel {
  uint8_t x, y;
  uint8_t num;
  Color c;
  Pixel() : x(0), y(0), num(0), c(Color(0,0,0)) {}
  Pixel(uint8_t x, uint8_t y, uint8_t n, Color c) :
    x(x), y(y), num(n), c(c) {}
  bool checkBounds(const Bounds &b) {
    return (checkXBounds(b) && checkYBounds(b));
  }
  bool checkYBounds(const Bounds &b) {
    return (y >= b.yTop && y <= b.yBot);
  }
  bool checkXBounds(const Bounds &b) {
    return (x >= b.xTop && x <= b.xBot);
  }
  inline uint16_t index(const uint8_t& width) {
    return 3 * (y * width + x);
  }
  void render(uint8_t *image, const uint8_t& width) {
    auto idx = index(width);
    image[idx]   = c.r;
    image[idx+1] = c.g;
    image[idx+2] = c.b;
  }
};

// struct AnimatedConfig {
//   uint8_t *image;
//   const uint8_t *origImage;
//   uint8_t width;
//   Bounds *b;
//   weatherType weather;
// };

class GenericDrop : public AnimatedConfig
{
private:
  uint8_t x, y, id;
  vector<Pixel> pixels;

public:
  GenericDrop(uint8_t x, uint8_t y, uint8_t size,
      uint8_t id, const AnimatedConfig& conf) :
      AnimatedConfig(conf), x(x), y(y), id(id) {
    init(size);
  }
  auto size() {
    return pixels.size();
  }

  // Build a stacked set of several pixels,
  // starting at our drop origin and growing up
  void init(uint8_t dropSize)
  {
    // Use a fixed size of 3 pixels for now
    // Pull a random color from our color list
    for (auto i = 0; i < dropSize; i++)
    {
      pixels.push_back(
        Pixel(
          x, (y - i), i, getRandomColor(
            weatherColors(weather)
          )
        )
      );
    }

    render();
  }

  void render() {
    for (vector<Pixel>::iterator pxl = pixels.begin();
      pxl != pixels.end(); pxl++) {
        if (pxl->checkBounds(bounds))
          pxl->render(image, imgWidth);
    }
  }

  // Shift all pixels down a row, removing ones
  // outside of our bounds
  void move()
  {
    vector<Pixel>::iterator pxl;

    // Clear pixels (copy data from original image)
    for (pxl = pixels.begin(); pxl != pixels.end(); pxl++)
    {
      auto index = pxl->index(imgWidth);
      memcpy(image+index, origImage+index, sizeof(uint8_t)*3);
    }

    // Check bounds, move and render at new location
    for (pxl = pixels.begin(); pxl != pixels.end();)
    {
      // Remove the pixel if it lies outside our bounds
      if (pxl->y >= bounds.yBot) {
        pxl = pixels.erase(pxl);
        continue;
      }

      // Update the position, then set the pixel color
      // in the new location
      pxl->y += 1;

      if (pxl->checkBounds(bounds)) {
        pxl->render(image, imgWidth);
      }

      pxl++;
    }

  }
};


// [===--- DropAnimation ---===]
class DropAnimation
{
private:
  bool init = false;
  AnimatedConfig conf;

  uint8_t dropId = 0;
  vector<GenericDrop> drops;

  std::random_device rd;
  std::mt19937 gen;
  distrib *xCloud, *yUnderCloud, *yInCloud, *dropSize;

public:
  DropAnimation() { gen.seed(rd()); }

  void configDrop(const AnimatedConfig& animConf)
  {
    uint8_t xTop, yTop, xBot, yBot;

    conf = animConf;
    std::tie(xTop, yTop, xBot, yBot) = conf.getBounds();

    xCloud = new distrib(xTop, xBot);
    yUnderCloud = new distrib(yTop, yBot);
    yInCloud = new distrib(yTop - 5, yTop);
    dropSize = new distrib(2, 3);

    drops.clear();
    init = true;
  }

  // Create a drop at a random location within our bounds
  // TODO: Add some logic to prevent "clustering" (eg: ensure
  // semi-uniform distribution, maybe with a mandatory spacing)
  void addDrop(bool underCloud = false)
  {
    if (!init) {
      _error("%s called without initialization, aborting!",
          __METHOD__);
      return;
    }

    auto x = (uint8_t)(*xCloud)(gen);
    auto y = (underCloud) ? (uint8_t)(*yUnderCloud)(gen) : 
        (uint8_t)(*yInCloud)(gen);
    auto size = (uint8_t)(*dropSize)(gen);

    // _debug("DropAnimation::addDrop(%d,%d).start; drops.size=%d",
    //   x, y, drops.size());
    GenericDrop *drop = new GenericDrop(
        x, y, size, dropId++, conf
    );
    drops.push_back(*drop);
    // _debug("DropAnimation::addDrop(%d,%d).end; drops.size=%d",
    //   x, y, drops.size());
  }

  // Called via checkUpdate in AnimatedWidget parent class
  void updateAnimation()
  {
    if (!init) {
      _error("%s called without initialization, aborting!",
          __METHOD__);
      return;
    }
    // _debug("DropAnimation::updateAnimation() enter drops
    //  size=%d", drops.size());

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

    for (auto i=0; i<removedDrops; i++, addDrop());
  }
};

class SunAnimation
{
private:
  uint8_t width;
  vector<int> rays;
};

// [===--- RainAnimation ---===]
class RainAnimation : public DropAnimation
{
public:
  static const uint8_t numDrops = 16;
  const Bounds bounds = Bounds(8, 15, 27, 23);
  uint16_t imageUpdatePeriodMs = 800;

  // Prepare an animation
  void config(AnimatedConfig& animConf)
  {
    _debug(__METHOD__);

    // Set bounds for our rain animation, pass
    // config to parent DropAnimation class
    animConf.setBounds(bounds);
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
  RainAnimation rainWidget;
  SunAnimation sunWidget;

public:
  WeatherWidget(const char *name) : AnimatedWidget(name) {
    // widgetMap[WEATHER_RAINY] = rainWidget;
  }

  // Find our weather enum type from a NWS condition
  // string and update our widget based on that 
  void updateWeather(const char *nws, bool daytime)
  {
    updateWeather(
      nwsWeatherTypeLookup(nws, (daytime) ?
        DAY_TIME : NIGHT_TIME)
    );
  }

  // Lookup our icon filename from our
  // weather type and load the icon
  void updateWeather(weatherType newWeather)
  {
    // Update our widget icon
    weather = newWeather;
    updateIcon(
      weatherIconLookup(weather)
    );

    // Make a copy of our icon image
    // Used in animation as a "background layer" reference
    if (iImageOrig)
      delete iImageOrig;
    
    auto size = iWidth * iHeight * 3;
    iImageOrig = new uint8_t[size];
  
    if (!iImageOrig) {
      _error("%s : unable to allocate buffer for icon "\
        "image, aborting!",
        __METHOD_ARG__(weatherStr(weather))
      );
      return;
    }
    
    memcpy((void *)iImageOrig, iImage, size);

    // iImage/iWidth set in DashboardWidget::updateIcon()
    // iImageOrig created by initAnimation() above
    animConf = {(uint8_t *)iImage, iImageOrig, iWidth,
        weather};
    aInit = true;

    switch (weather) {
    case WEATHER_RAINY:
      rainWidget.config(animConf);
      setImageUpdatePeriod(rainWidget.getUpdatePeriod());
      break;
    default:
      break;
    }
  }

  void doImageUpdate()
  {
    if (!aInit) {
      return;
    }

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