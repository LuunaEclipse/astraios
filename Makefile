CXXFLAGS ?= -Wall -g
CXXFLAGS += -std=c++1y
CXXFLAGS += `pkg-config --cflags x11 spdlog`
LDFLAGS += `pkg-config --libs x11 spdlog`

all: basic_wm

HEADERS = \
    window_manager.hpp
SOURCES = \
    window_manager.cpp \
    main.cpp
OBJECTS = $(SOURCES:.cpp=.o)

basic_wm: $(HEADERS) $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f basic_wm $(OBJECTS)
