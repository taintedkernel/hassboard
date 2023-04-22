#ifndef WEATHERWIDGET_H
#define WEATHERWIDGET_H

#include "dynamicwidget.h"
#include "datetime.h"
#include "weather.h"
#include "logger.h"

#include <png++/png.hpp>
#include <sys/stat.h>
#include <string.h>
#include <time.h>

#include <random>
#include <string>
#include <ranges>
#include <tuple>
#include <functional>

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

  int8_t (*pixelGenX)(int8_t);
  int8_t (*pixelGenY)(int8_t);

  AnimatedConfig() {};
  AnimatedConfig(uint8_t *i, const uint8_t *o,
    uint8_t w, weatherType wthr) : image(i), origImage(o),
    imgWidth(w), weather(wthr) {};

  void setBounds(const Bounds &b) { bounds = b; }
  void setPixelGen(int8_t (*x)(int8_t), int8_t (*y)(int8_t)) {
    pixelGenX = x;
    pixelGenY = y;
  }

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
  GenericDrop(uint8_t id, const AnimatedConfig& conf) :
      AnimatedConfig(conf), id(id) {}
  auto size() {
    return pixels.size();
  }

  // Build a stacked set of several pixels,
  // starting at our drop origin and growing up
  void init(uint8_t iX, uint8_t iY, uint8_t pixelCount)
  {
    // Set our base coordinates
    x = iX; y = iY;

    // Sample random colors from our color list to use 
    for (auto i = 0; i < pixelCount; i++)
    {
      // _debug("%s: pixel %d, y: %d", __METHOD__, i, pixelGenY(i));
      pixels.push_back(
        Pixel(
          x + pixelGenX(i),
          y + pixelGenY(i),
          i, getRandomColor(
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
  vector<GenericDrop> drops;

protected:
  uint8_t dropId = 0;
  AnimatedConfig conf;

public:
  virtual GenericDrop* makeDrop(bool) {}

  void configDrop(const AnimatedConfig& animConf)
  {
    conf = animConf;
    drops.clear();
    init = true;
  }

  // Create a drop at a random location within our bounds
  // TODO: Add some logic to prevent "clustering" (eg: ensure
  // semi-uniform distribution, maybe with a mandatory spacing)
  void addDrop(bool inCloud = true)
  {
    if (!init) {
      _error(__METHOD__);
      _error("  called without initialization, aborting!");
      return;
    }

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
    //         x = (uint8_t)(*xDrop)(gen);
    //       }
    //     }
    //   }
    // }
    drops.push_back(*makeDrop(inCloud));
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
  virtual milliseconds getUpdatePeriod() {
    return milliseconds(DEFAULT_INTERVAL_MS);
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
  uint8_t boundWidth;
  static const uint8_t numDrops = 16;
  const Bounds bounds = Bounds(8, 15, 27, 23);
  const milliseconds imageUpdatePeriodMs = 200ms;
  vector<uint8_t> columnDropHold;
  uint8_t maxDropAttempts = 50;

  std::random_device rd;
  std::mt19937 gen;
  distrib *dropDistX, *dropDistY, *dropDistYCloud, *dropSize;

  RainAnimation()
  {
    gen.seed(rd());
    dropDistX = new distrib(bounds.xTop, bounds.xBot);
    dropDistY = new distrib(bounds.yTop, bounds.yBot);
    dropDistYCloud = new distrib(bounds.yTop - 5, bounds.yTop);
    dropSize = new distrib(2, 3);
    boundWidth = bounds.xBot - bounds.xTop;
    columnDropHold = vector<uint8_t>(bounds.xBot - bounds.xTop, 0);
  }

  // Functions to generate pixel coordinates from an index
  static int8_t pixelGenX(int8_t idx) { return 0; }
  static int8_t pixelGenY(int8_t idx) { return -1 * idx; }

  GenericDrop* makeDrop(bool inCloud = true)
  {
    // _debug(__METHOD__);
    // for (auto i=0; i<columnDropHold.size(); i++) {
      // _debug("columnDropHold[%d]=%d", i, columnDropHold[i]);
    // }    

    // Try to space raindrops out to avoid clumping, which
    // tends to occur when using strictly random spacing.
    // This is a naive approach, if random dX isn't valid
    // then just try again, up until an attempt limit.
    //
    // With some additional logic we could optimize this,
    // tracking the previous guesses to cap the max number
    // of attempts to the total number of possible options.
    //
    // But it would likely be somewhat complicated, having
    // to change the random distribution bounds after each
    // attempt; may not be worth the effort.
    uint8_t count = 0;
    uint8_t dX = (uint8_t)(*dropDistX)(gen);
    while (columnDropHold[dX-bounds.xTop] > 0 && count++ < maxDropAttempts) {
      dX = (uint8_t)(*dropDistX)(gen);
    }
    uint8_t size = (uint8_t)(*dropSize)(gen);
    uint8_t dY = inCloud ? (uint8_t)(*dropDistYCloud)(gen) :
        (uint8_t)(*dropDistY)(gen);

    // _debug("RainAnimation::makeDrop(): choosing dX=%d, dY=%d, size=%d, iterations=%d", dX, dY, size, count);

    // Mark off some rough boundries to prevent
    // other raindrops from spawning nearby
    // Note: This still needs to take into account dY
    if (dX > bounds.xTop) 
      columnDropHold[dX-bounds.xTop-1] = size+2;
    columnDropHold[dX-bounds.xTop] = size+2;
    if (dX < bounds.xTop)
      columnDropHold[dX-bounds.xTop+1] = size+2;

    GenericDrop *drop = new GenericDrop(dropId++, conf);
    drop->init(dX, dY, size);
    return drop;
  }

  // Prepare an animation
  void config(AnimatedConfig& animConf)
  {
    // Set bounds for our rain animation, pass
    // config to parent DropAnimation class
    animConf.setBounds(bounds);
    animConf.setPixelGen(&pixelGenX, &pixelGenY);
    configDrop(animConf);

    // Create some drops
    for (auto i=0; i<numDrops; i++) {
      addDrop(false);
    }
  }

  // How frequently we update our animation
  milliseconds getUpdatePeriod() {
    return milliseconds(imageUpdatePeriodMs);
  }

  void updateAnimation()
  {
    updateDropAnimation();
    for (auto i=0; i<columnDropHold.size(); i++) {
      columnDropHold[i] = std::max(columnDropHold[i] - 1, 0);
    }
  }
};

class LightningRainAnimation : public DropAnimation, AnimationBase
{
public:
  static const uint8_t numDrops = 16;
  const Bounds bounds = Bounds(8, 15, 27, 23);
  const milliseconds imageUpdatePeriodMs = 50ms;
  // const milliseconds imageRainPeriodMs = 200ms;

  uint16_t frame = 0;
  std::random_device rd;
  std::mt19937 gen;
  distrib *dropDistX, *dropDistY, *dropDistYCloud, *dropSize;
  uint8_t *lImage;
  uint32_t lWidth, lHeight;

  LightningRainAnimation()
  {
    dropDistX = new distrib(bounds.xTop, bounds.xBot);
    dropDistY = new distrib(bounds.yTop, bounds.yBot);
    dropDistYCloud = new distrib(bounds.yTop - 5, bounds.yTop);
    dropSize = new distrib(2, 3);

    // Load lightning bolt image into buffer
    png::image<png::rgb_pixel> imagePNG;
    struct stat buffer;

    if (stat(ICON_LIGHTNING_BOLT, &buffer) == 0) {
      imagePNG.read(ICON_LIGHTNING_BOLT);
    } else {
      _error("unable to find storm image %s", ICON_LIGHTNING_BOLT);
      return;
    }

    // _log(ICON_LIGHTNING_BOLT);
    lWidth = imagePNG.get_width();
    lHeight = imagePNG.get_height();
    // printf("width: %d, height: %d\n", lWidth, lHeight);
    lImage = (uint8_t *)malloc(lWidth * lHeight * 3 * sizeof(uint8_t));
    if (lImage == NULL) {
      _error("unable to allocate buffer for icon, aborting");
      return;
    }
    // _log(__METHOD__);
    // _log("width: %d, height: %d", lWidth, lHeight);
    // _log("width: %d, height: %d", imagePNG.get_width(), imagePNG.get_height());

    uint16_t idx = 0;
    for (uint16_t y = 0; y < lHeight; y++) {
      for (uint16_t x = 0; x < lWidth; x++) {
        png::rgb_pixel pixel = imagePNG.get_pixel(x, y);
        lImage[idx++] = pixel.red;
        lImage[idx++] = pixel.green;
        lImage[idx++] = pixel.blue;
      }
    }
  }

  // Functions to generate pixel coordinates from an index
  static int8_t pixelGenX(int8_t idx) { return 0; }
  static int8_t pixelGenY(int8_t idx) { return -1 * idx; }

  GenericDrop* makeDrop(bool inCloud = true)
  {
    auto dX = (uint8_t)(*dropDistX)(gen);
    auto dY = inCloud ? (uint8_t)(*dropDistYCloud)(gen) :
        (uint8_t)(*dropDistY)(gen);
    auto size = (uint8_t)(*dropSize)(gen);

    GenericDrop *drop = new GenericDrop(dropId++, conf);
    drop->init(dX, dY, size);
    return drop;
  }

  // Prepare an animation
  void config(AnimatedConfig& animConf)
  {
    // Set bounds for our rain animation, pass
    // config to parent DropAnimation class
    animConf.setBounds(bounds);
    animConf.setPixelGen(&pixelGenX, &pixelGenY);
    configDrop(animConf);

    // Create some drops
    for (auto i=0; i<numDrops; i++) {
      addDrop(false);
    }
  }

  // How frequently we update our animation
  milliseconds getUpdatePeriod() {
    return milliseconds(imageUpdatePeriodMs);
  }

  void updateBackground(bool drawLightning)
  {
    // _log("updateBackground(%d)", drawLightning);

    uint8_t *origImage = (uint8_t *)conf.origImage;
    uint8_t *image = (uint8_t *)conf.image;
    uint16_t idx = 0;
    for (uint16_t y = 0; y < lHeight; y++) {
      for (uint16_t x = 0; x < lWidth; x++)
      {
        idx = 3 * (y * lWidth + x);
        if (lImage[idx] == 0 &&
            lImage[idx+1] == 0 &&
            lImage[idx+2] == 0)
          continue;

        // _log("x,y: %d,%d; image[idx:+2]: %d, %d, %d; lImage[idx:+2]: %d, %d, %d", x, y, image[idx], image[idx+1], image[idx+2], lImage[idx], lImage[idx+1], lImage[idx+2]);

        if (drawLightning) {
          memcpy(image+idx, lImage+idx, sizeof(uint8_t)*3);
          memcpy(origImage+idx, lImage+idx, sizeof(uint8_t)*3);
        } else {
          memset(image+idx, 0, sizeof(uint8_t)*3);
          memset(origImage+idx, 0, sizeof(uint8_t)*3);
        }
      }
    }
  }

  void updateAnimation()
  {
    frame++;
    updateDropAnimation();

    if (frame % 15 == 0)
      updateBackground(true);
    if (frame % 15 == 1)
      updateBackground(false);
    if (frame % 15 == 2)
      updateBackground(true);
    if (frame % 15 == 3)
      updateBackground(false);
  }
};

//
// [===--- SnowAnimation ---===]
//
// Class to define parameters for a snow
// animation graphic. Built mostly upon a
// "DropAnimation" base class, which does
// most of the heavy lifting.
//
class SnowAnimation : public DropAnimation, AnimationBase
{
public:
  static const uint8_t numDrops = 12;
  const Bounds bounds = Bounds(8, 15, 27, 23);
  const milliseconds imageUpdatePeriodMs = 1200ms;

  // Prepare an animation
  void config(AnimatedConfig& animConf)
  {
    // Set bounds for our rain animation, pass
    // config to parent DropAnimation class
    animConf.setBounds(bounds);
    configDrop(animConf);

    // Create some drops
    for (auto i=0; i<numDrops; i++) {
      addDrop();
    }
  }

  // How frequently we update our animation
  milliseconds getUpdatePeriod() {
    return milliseconds(imageUpdatePeriodMs);
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
  float lastImageTime = 0;
  milliseconds imageUpdatePeriod = FRAME_UPDATE_PERIOD_MS;

  // Animations & management
  LightningRainAnimation aStorm;
  RainAnimation aRain;
  SunAnimation aSun;
  std::map<weatherType, AnimationBase*> animationMap;

public:
  WeatherWidget(const char *name) : AnimatedWidget(name) {
    animationMap[WEATHER_RAINY] = (AnimationBase*)&aRain;
    animationMap[WEATHER_SUNNY] = (AnimationBase*)&aSun;
    animationMap[WEATHER_STORMY] = (AnimationBase*)&aStorm;
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
      _debug("animationPeriod: %ld", anim->getUpdatePeriod());
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

    // TODO: Check to see if animation is actually active/
    // visible (eg: background weather forecast widget)
    auto anim = getAnimation(weather);
    if (!anim || !anim->isInit())
        return;

    anim->updateAnimation();
    render();
  }
};

#endif