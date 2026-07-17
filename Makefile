CC		:= g++
CFLAGS 	:= -O2
LDFLAGS := $(CFLAGS) \
-lm
TARGET  := myapp
SRCS    := $(wildcard src2/*.cpp)
OBJS    := $(patsubst %.c,%.o,$(SRCS))
all: $(OBJS)
	$(CC) $(OBJS) $(CFLAGS) $(LDFLAGS) -o $(TARGET) 
