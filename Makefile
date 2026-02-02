CXXFLAGS ?= -Wall -g
CXXFLAGS += -std=c++1y
CXXFLAGS += `pkg-config --cflags x11 spdlog`
LDFLAGS += `pkg-config --libs x11 spdlog`

all: basic_wm

HEADERS = \
    util.hpp \
    window_manager.hpp
SOURCES = \
    util.cpp \
    window_manager.cpp \
    main.cpp
OBJECTS = $(SOURCES:.cpp=.o)

basic_wm: $(HEADERS) $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f basic_wm $(OBJECTS)

