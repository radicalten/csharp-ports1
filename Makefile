CXX			:= clang++
CXXSTD   	:= c++20
QT_PREFIX := $(shell brew --prefix qt@6 2>/dev/null || echo /opt/homebrew/opt/qt@6)
CXXFLAGS 	:= -std=$(CXXSTD) -O2 \
            -Iinclude \
			-F$(QT_PREFIX)/lib \
            -I$(QT_PREFIX)/include \
            -I$(QT_PREFIX)/include/QtWidgets \
            -I$(QT_PREFIX)/include/QtCore \
            -I$(QT_PREFIX)/include/QtGui
LDFLAGS := $(CXXFLAGS) \
            -framework QtWidgets -framework QtCore -framework QtGui \
            -Wl,-rpath,$(QT_PREFIX)/lib \
			-lm
TARGET  := myapp
SRCS    := $(wildcard src2/*.cpp)
OBJS    := $(patsubst %.c,%.o,$(SRCS))
all: $(OBJS)
	$(CXX) $(OBJS) $(CXXFLAGS) $(LDFLAGS) -o $(TARGET) 
