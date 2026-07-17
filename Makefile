CXX			:= clang++
CXXSTD   	:= c++20
QT_PREFIX := $(shell brew --prefix qt@6 2>/dev/null || echo /opt/homebrew/opt/qt@6)
MOC      := $(QT_PREFIX)/bin/moc
RCC      := $(QT_PREFIX)/bin/rcc
AUTOMOC  := $(MOC)
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
# moc-generated sources
MOC_HDRS := src2/include/main_window.hpp
MOC_SRCS := $(MOC_HDRS:src2/include/%.hpp=/moc_%.cpp)
TARGET  := myapp
BUILD 	:= build
SRCS    := $(wildcard src2/*.cpp)
OBJS    := $(patsubst %.c,%.o,$(SRCS)) $(MOC_SRCS:%.cpp=$(BUILD)%.o)
all: $(OBJS)

# Run moc on headers
$(BUILD)/moc_%.cpp: src2/include/%.hpp | $(BUILD)
	@mkdir -p $(dir $@)
	$(MOC) $< -o $@

# Compile regular sources
$(BUILD)/%.o: %.cpp | $(BUILD)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile moc sources
$(BUILD)/moc_%.o: $(BUILD)/moc_%.cpp | $(BUILD)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

	$(CXX) $(OBJS) $(CXXFLAGS) $(LDFLAGS) -o $(TARGET) 

$(BUILD):
	@mkdir -p $(BUILD)/src
