CROSS=mipsel-linux-
CC=${CROSS}gcc
LD=ld
AS=as

CPPFLAGS=
CFLAGS=-Wall -D_GNU_SOURCE -D_REENTRANT
LDFLAGS=-lpthread

lists=$(shell ls *.c 2>/dev/null)
TARGET=$(basename $(lists))

all:
	@echo -e "make \E[32m$(patsubst %, %\n    ,$(basename $(lists)))\b\b\b\b\b\E[0m";

TARGET: $(patsubst %, %.c, $TARGET)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	-rm -rf $(TARGET)

