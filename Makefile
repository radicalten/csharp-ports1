CXX			:= clang++
CXXSTD   	:= c++20
CXXFLAGS 	:= -std=$(CXXSTD) -O2
LDFLAGS := $(CFLAGS) \
-lm
TARGET  := myapp
SRCS    := $(wildcard src2/*.cpp)
OBJS    := $(patsubst %.c,%.o,$(SRCS))
all: $(OBJS)
	$(CXX) $(OBJS) $(CXXFLAGS) $(LDFLAGS) -o $(TARGET) 
