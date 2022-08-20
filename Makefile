CXXFLAGS=-O3 -W -Wall -Wextra -Wno-unused-parameter -g -D_FILE_OFFSET_BITS=64
LDFLAGS=-L$(HOME)/rpi-rgb-led-matrix/lib -lpthread -lrt -lm -lmosquitto -lrgbmatrix -lpthread
INCDIR=-I$(HOME)/rpi-rgb-led-matrix/include -I./include
OBJECTS=smartgirder.o widget.o display.o dashboard.o mqtt.o logger.o secrets.o datetime.o
HEADERS=widget.h display.h dashboard.h mqtt.h logger.h secrets.h datetime.h icons.h
BINARIES=smartgirder

all : $(BINARIES)

clean:
	rm *.o $(BINARIES)

smartgirder: smartgirder.o $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o smartgirder $(LDFLAGS)

%.o : %.cpp include/*.h
	$(CXX) $(INCDIR) $(CXXFLAGS) -c -o $@ $<
	objdump -Sdr $(BINARIES).o > $(BINARIES).txt
	objdump -t $(BINARIES).o > $(BINARIES).sym