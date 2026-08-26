CXX := c++

TARGET := FastGenFitTest
SOURCE := FastGenFitTest.cc
OBJECT := FastGenFitTest.o

ROOTCFLAGS := $(shell root-config --cflags)
ROOTLIBS   := $(shell root-config --libs)

CXXFLAGS := -O2 -g -Wall -Wextra -std=c++20 \
            $(ROOTCFLAGS) \
            -I$(OFFLINE_MAIN)/include

LDFLAGS := $(ROOTLIBS) \
           -L$(OFFLINE_MAIN)/lib \
           -L$(OFFLINE_MAIN)/lib64 \
           -lgenfit2 \
           -lGeom

all: $(TARGET)

$(TARGET): $(OBJECT)
	$(CXX) $(OBJECT) -o $(TARGET) $(LDFLAGS)

$(OBJECT): $(SOURCE)
	$(CXX) $(CXXFLAGS) -c $(SOURCE) -o $(OBJECT)

clean:
	rm -f $(OBJECT) $(TARGET)

.PHONY: all clean
