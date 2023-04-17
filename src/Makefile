# compiler flags
CXXFLAGS=-O3 -std=c++20 -W -Wall -Wextra -Wno-unused-parameter -g -D_FILE_OFFSET_BITS=64
MAGICK_CFLAGS?=$(shell GraphicsMagick++-config --cppflags --cxxflags)
LIBPNG_CFLAGS?=$(shell libpng-config --cflags)

# linker flags
MAGICK_LDFLAGS?=$(shell GraphicsMagick++-config --ldflags --libs)
LIBPNG_LDFLAGS?=$(shell libpng-config --ldflags --libs)
LDFLAGS=-L$(HOME)/rpi-rgb-led-matrix/lib -lpthread -lrt -lm -lmosquitto -lrgbmatrix -lpthread $(LIBPNG_LDFLAGS) $(MAGICK_LDFLAGS)

# includes flags
INCDIR=-I$(HOME)/rpi-rgb-led-matrix/include -I./include

# sources
OBJECTS=smartgirder.o widget.o display.o dashboard.o mqtt.o logger.o secrets.o datetime.o dynamicwidget.o widgetmanager.o font.o weatherwidget.o weather.o
HEADERS=widget.h display.h dashboard.h mqtt.h logger.h secrets.h datetime.h dynamicwidget.h widgetmanager.h font.h weatherwidget.h weather.h icons.h

# output
BINARIES=smartgirder

# targets
all : smartgirder ../smartgirder

clean:
	rm *.o smartgirder

../smartgirder: smartgirder
	-pkill $(BINARIES)
	-rm ../$(BINARIES) 2>/dev/null
	cp $(BINARIES) ../$(BINARIES)

nokill: smartgirder

smartgirder: smartgirder.o $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(BINARIES) $(LDFLAGS)
	objdump -Sdr $(BINARIES) > $(BINARIES).txt
	nm -lnC $(BINARIES) > $(BINARIES).sym

smartgirder.o : smartgirder.cpp include/dashboard.h include/logger.h include/display.h include/mqtt.h include/widget.h
	$(CXX) $(INCDIR) $(CXXFLAGS) -c -o $@ $<

dashboard.o : dashboard.cpp weatherwidget.cpp dynamicwidget.cpp weather.cpp include/dashboard.h include/logger.h include/widget.h include/icons.h include/mqtt.h include/weatherwidget.h include/weather.h include/dynamicwidget.h
	$(CXX) $(INCDIR) $(CXXFLAGS) -c -o $@ $<

widgetmanager.o : widgetmanager.cpp include/widgetmanager.h include/widget.h include/dashboard.h
	$(CXX) $(INCDIR) $(CXXFLAGS) -c -o $@ $<

widget.o : widget.cpp include/display.h include/logger.h include/widget.h include/icons.h
	$(CXX) $(INCDIR) $(CXXFLAGS) -c -o $@ $<

dynamicwidget.o: dynamicwidget.cpp include/dynamicwidget.h include/datetime.h include/logger.h
	$(CXX) $(INCDIR) $(CXXFLAGS) -c -o $@ $<

weather.o: weather.cpp include/weather.h include/icons.h
	$(CXX) $(INCDIR) $(CXXFLAGS) -c -o $@ $<

weatherwidget.o: weatherwidget.cpp include/weatherwidget.h include/dynamicwidget.h include/weather.h include/logger.h include/datetime.h
	$(CXX) $(INCDIR) $(CXXFLAGS) -c -o $@ $<

display.o : display.cpp include/display.h include/logger.h include/widget.h include/datetime.h include/font.h
	$(CXX) $(INCDIR) $(CXXFLAGS) -c -o $@ $<

mqtt.o : mqtt.cpp include/mqtt.h include/logger.h
	$(CXX) $(INCDIR) $(CXXFLAGS) -c -o $@ $<

datetime.o : datetime.cpp include/datetime.h
	$(CXX) $(INCDIR) $(CXXFLAGS) -c -o $@ $<

logger.o : logger.cpp include/logger.h
	$(CXX) $(INCDIR) $(CXXFLAGS) -c -o $@ $<

secrets.o : secrets.cpp
	$(CXX) $(INCDIR) $(CXXFLAGS) -c -o $@ $<

font.o : font.cpp include/font.h include/logger.h
	$(CXX) $(INCDIR) $(CXXFLAGS) -c -o $@ $<
