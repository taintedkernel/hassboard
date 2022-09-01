CXXFLAGS=-O3 -W -Wall -Wextra -Wno-unused-parameter -g -D_FILE_OFFSET_BITS=64
MAGICK_CFLAGS?=$(shell GraphicsMagick++-config --cppflags --cxxflags)
LIBPNG_CFLAGS?=$(shell libpng-config --cflags)

MAGICK_LDFLAGS?=$(shell GraphicsMagick++-config --ldflags --libs)
LIBPNG_LDFLAGS?=$(shell libpng-config --ldflags --libs)
LDFLAGS=-L$(HOME)/rpi-rgb-led-matrix/lib -lpthread -lrt -lm -lmosquitto -lrgbmatrix -lpthread $(LIBPNG_LDFLAGS) $(MAGICK_LDFLAGS)

INCDIR=-I$(HOME)/rpi-rgb-led-matrix/include -I./include

OBJECTS=smartgirder.o widget.o display.o dashboard.o mqtt.o logger.o secrets.o datetime.o
HEADERS=widget.h display.h dashboard.h mqtt.h logger.h secrets.h datetime.h icons.h
BINARIES=smartgirder

all : $(BINARIES)

clean:
	rm *.o $(BINARIES)

smartgirder: smartgirder.o $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o smartgirder $(LDFLAGS)
	objdump -Sdr $(BINARIES) > $(BINARIES).txt
	nm -lnC $(BINARIES) > $(BINARIES).sym

# %.o : %.cpp include/*.h
# 	$(CXX) $(INCDIR) $(CXXFLAGS) -c -o $@ $<

smartgirder.o : smartgirder.cpp include/dashboard.h include/logger.h include/display.h include/mqtt.h include/widget.h
	$(CXX) $(INCDIR) $(CXXFLAGS) -c -o $@ $<

dashboard.o : dashboard.cpp include/dashboard.h include/logger.h include/widget.h include/icons.h include/mqtt.h
	$(CXX) $(INCDIR) $(CXXFLAGS) -c -o $@ $<

widget.o : widget.cpp include/display.h include/logger.h include/widget.h include/icons.h
	$(CXX) $(INCDIR) $(LIBPNG_CFLAGS) $(MAGICK_CFLAGS) $(CXXFLAGS) -c -o $@ $<

display.o : display.cpp include/display.h include/logger.h include/widget.h include/datetime.h
	$(CXX) $(INCDIR) $(CXXFLAGS) -c -o $@ $<

mqtt.o : mqtt.cpp include/mqtt.h include/logger.h
	$(CXX) $(INCDIR) $(CXXFLAGS) -c -o $@ $<

datetime.o : datetime.cpp include/datetime.h
	$(CXX) $(INCDIR) $(CXXFLAGS) -c -o $@ $<

logger.o : logger.cpp include/logger.h
	$(CXX) $(INCDIR) $(CXXFLAGS) -c -o $@ $<

secrets.o : secrets.cpp
	$(CXX) $(INCDIR) $(CXXFLAGS) -c -o $@ $<