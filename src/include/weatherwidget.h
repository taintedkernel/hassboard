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
*/

// Data structure to store coordinates
// for a rendering bounding rectangle
struct Bounds {
  uint8_t xTop, yTop;
  uint8_t xBot, yBot;
  Bounds() : xTop(0), yTop(0), xBot(0), yBot(0) {}
  Bounds(uint8_t xT, uint8_t yT, uint8_t xB, uint8_t yB) :
    xTop(xT), yTop(yT), xBot(xB), yBot(yB) {}
};

// Class to store configuration for an animation:
//   Image parameters/buffers, animation bounds
//   and weather type.
class AnimatedConfig
{
public:
  uint8_t *image;
  const uint8_t *origImage;
  uint8_t imgWidth;
  Bounds bounds;
  weatherType weather;

  AnimatedConfig() {};
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

// Simple data structure to store information
// about a pixel used in animations.  Built-in
// bounds checking and rendering to image buffer.
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

//
// [===--- GenericDrop ---===]
//
// A class to handle "drop" (eg: precip) logic
//
// Assumptions: Drops move (straight) down, pixels
// that fall outside our bounds are removed.
//
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
    // Sample random colors from our color list to use 
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

  // Render a drop's pixels, if inside our bounds
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


//
// [===--- DropAnimation ---===]
//
// Class used to handle animation of drops (eg: precipitation)
//
// Some assumptions here: A cloud graphic exists directly
// above the configured bounds. Drops are of size [2,3] pixels.
// New drops are created to replace those that scroll/move
// outside the defined bounds.
//
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
    yInCloud = new distrib(yTop - 7, yTop);
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
      _error(__METHOD__);
      _error("  called without initialization, aborting!");
      return;
    }

    // Choose random X and size parameters
    // Y random bounds depend if drop is "in" or "under"
    // the cloud
    auto size = (uint8_t)(*dropSize)(gen);
    auto y = (underCloud) ? (uint8_t)(*yUnderCloud)(gen) : 
        (uint8_t)(*yInCloud)(gen);
    auto x = (uint8_t)(*xCloud)(gen);

    // uint8_t i, xC, yC;
    // bool valid = false;
    // for (i = 0; i < 25, valid == false; i++) {
    //   for (xC = x-1; xC <= x+1, valid == false; x++) {
    //     for (yC = y-size; yC <= y+1, valid == false; y++) {
    //       auto idx = imgIndex(xC, yC, conf.imgWidth);
    //       if (conf.image[idx] == 0 && conf.image[idx+1] == 0
    //           && conf.image[idx+2] == 0) {
    //         valid = true;
    //       } else {
    //         x = (uint8_t)(*xCloud)(gen);
    //       }
    //     }
    //   }
    // }

    // Create the drop and add to our list (vector)
    GenericDrop *drop = new GenericDrop(
        x, y, size, dropId++, conf
    );
    drops.push_back(*drop);
  }

  void updateDropAnimation()
  {
    if (!init) {
      _error(__METHOD__);
      _error("  called without initialization, aborting!");
      return;
    }

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

    for (auto i=0; i<removedDrops; i++, addDrop());
  }
};

class AnimationBase {
private:
  bool aInit = false;

public:
  const uint16_t DEFAULT_INTERVAL_MS = 1000;
  bool isInit() { return aInit; }
  void setInit(bool init) { aInit = init; }

  //
  // Functions overridden in our derived classes
  // Defines the interface between WeatherWidget
  // and the animations
  //

  // Set any animation configuration parameters
  virtual void config(AnimatedConfig& animConf) {}

  // Provide update interval to weather widget
  virtual float getUpdatePeriod() {
    return DEFAULT_INTERVAL_MS / 1000.0f;
  }

  // Render next animation frame
  virtual void updateAnimation() {}
};

class SunAnimation : public AnimationBase
{
private:
  uint8_t width;
  vector<int> rays;
};

//
// [===--- RainAnimation ---===]
//
// Class to define parameters for a rain
// animation graphic. Built mostly upon a
// "DropAnimation" base class, which does
// most of the heavy lifting.
//
// We set some bounds for the rain drop
// animation rendering, number of drops
// to render and an animation speed. Some
// of these may be dynamic in the future
// based upon real-time sensor data.
//
class RainAnimation : public DropAnimation, AnimationBase
{
public:
  static const uint8_t numDrops = 16;
  const Bounds bounds = Bounds(8, 15, 27, 23);
  const uint16_t imageUpdatePeriodMs = 800;

  // Prepare an animation
  void config(AnimatedConfig& animConf)
  {
    // Set bounds for our rain animation, pass
    // config to parent DropAnimation class
    animConf.setBounds(bounds);
    configDrop(animConf);

    // Create some drops
    for (auto i=0; i<numDrops; i++) {
      addDrop(true);
    }
  }

  // How frequently we update our animation
  float getUpdatePeriod() {
    return imageUpdatePeriodMs / 1000.0;
  }

  void updateAnimation() {
    updateDropAnimation();
  }
};

//
// [===--- WeatherWidget ---===]
//
// A class to wrap up weather-specific logic (lookups,
// translations) and animations for different weather
// conditions/types. We use a single instance of this
// for weather rendering, and update the icon/animation
// based upon the weather condition.
//
class WeatherWidget : public AnimatedWidget
{
public:
  enum weatherMotionType{MOTION_NONE, MOTION_RAIN, MOTION_SNOW};

private:
  // General configuration
  weatherType weather;
  AnimatedConfig aConf;
  const uint8_t *iImageOrig;

  // Animation timing
  time_t lastImageTime = 0;
  uint16_t imageUpdatePeriod = FRAME_UPDATE_PERIOD_MS / 1000;

  // Animations & management
  RainAnimation aRain;
  SunAnimation aSun;
  std::map<weatherType, AnimationBase*> animationMap;

public:
  WeatherWidget(const char *name) : AnimatedWidget(name) {
    animationMap[WEATHER_RAINY] = (AnimationBase*)&aRain;
    animationMap[WEATHER_SUNNY] = (AnimationBase*)&aSun;
  }

  AnimationBase* getAnimation(weatherType weather)
  {
    if ( auto anim = animationMap.find(weather);
        anim != animationMap.end() )
      return anim->second;
    return NULL;
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
  // weather type and load the icon. This is
  // called upon receipt of a weather state
  // MQTT message.
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
      _error(__METHOD_ARG__(weatherStr(weather)));
      _error("  unable to allocate buffer for icon "\
          "image, aborting!");
      return;
    }
    
    // Copy the data
    memcpy((void *)iImageOrig, iImage, size);

    // iImage/iWidth set in DashboardWidget::updateIcon() above
    aConf = {
        (uint8_t *)iImage, iImageOrig, iWidth, weather
    };

    // Find our animation and if present, configure
    if (auto anim = getAnimation(weather))
    {
      anim->config(aConf);
      setImageUpdatePeriod(
          anim->getUpdatePeriod()
      );
      anim->setInit(true);
    }
    else {
      _warn(__METHOD_ARG__(weatherStr(weather)));
      _warn("  animation not set for weather condition, "
          "skipping configuration");
    }
  }

  // Generate a new animation frame
  // and render to the display
  void doImageUpdate()
  {
    // This will likely be called without init
    // for a brief time upon startup. Animation config
    // update is only triggered after a weather state
    // MQTT message
    auto anim = getAnimation(weather);
    if (!anim || !anim->isInit())
        return;

    anim->updateAnimation();
    render();
  }
};

#endif