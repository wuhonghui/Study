CROSS=#mipsel-linux-
CC=$(CROSS)gcc
LD=$(CROSS)ld
AS=$(CROSS)as

CPPFLAGS=
CFLAGS=-Wall -D_GNU_SOURCE -D_REENTRANT
LDFLAGS=-rdynamic -lavdevice -lavformat -lavfilter -lavcodec -lavutil -lswscale -lswresample -lm -lz -lpthread -lbz2 

CFLAGS += -I/usr/local/include/SDL2/
LDFLAGS += -lSDL2

lists=$(shell ls *.c 2>/dev/null)
TARGET=$(basename $(lists))

default:
	@echo -e "make \E[32m$(patsubst %, %\n    ,$(basename $(lists)))\b\b\b\b\b\E[0m";

all: $(TARGET) 

$(TARGET): $(addsuffix .c, $(TARGET))
	$(CC) $(CFLAGS) -o $@ $(addsuffix .c, $@) $(LDFLAGS)

clean:
	-rm -rf $(TARGET) *.o
