# 
# Description: Makefile for onvif-profile-s
# Author: Wuhonghui
# Note: to silent make info, assign SILENT="@", to print make info, assign SILENT="".
# CROSS: using this to enable cross-compile.
# 
# Each subobjs can be compile independently, it generate a .o obj file for link into main programe, 
# and it also can be used to generate a .so dynamic lib for main programe use(no implement now).
#

# to silent makefile output, SILENT = @, else SIELNT = .
export SILENT = @
export CROSS = mipsel-linux-
export CC = $(CROSS)gcc
export AR = $(CROSS)ar
export LD = $(CROSS)ld
export RANLIB = $(CROSS)ranlib

# common compile flags
COMMON_CFLAGS = -Wall -O2 -fno-common -fomit-frame-pointer -pipe -D_REENTRANT -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -fno-common -Wno-unused-variable

DEBUG=1
# debug flag
ifdef DEBUG
	COMMON_CFLAGS += -DDEBUG -g
endif

export COMMON_CFLAGS

# compile flags and link flags
CFLAGS = $(COMMON_CFLAGS)

LDFLAGS += -lpthread -levent -lm

CFLAGS += -I./include -I./input/ -I./mediaframe/

SUBOBJS += input/input.o
SUBOBJS += common/common.o
SUBOBJS += mediaframe/mediaframe.o
SUBOBJS += output/output.o
#SUBOBJS += lib/lib.o
SUBOBJS += utils/utils.o

this_name = $(shell basename `pwd`)

all: subobjs main.o
	@echo "Linking excutable..."
	$(CC) $(CFLAGS) -o $(this_name)  main.o $(SUBOBJS) $(LDFLAGS)
	@echo "Link Done."

subobjs: $(patsubst %, __%__, $(dir $(SUBOBJS)))

$(patsubst %, __%__, $(dir $(SUBOBJS))):
	make -C $(patsubst __%__, %, $@) $(TGT)

clist = $(shell ls *.c)
-include $(clist:.c=.d)

%.d:%.c
	@echo "Resolving dependency for $<..."
	@set -e;rm -f $@;\
	$(CC) -MM $(CFLAGS) $< > $@.$$$$;\
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; 
	@rm -rf $@.[0-9]*

%.o:%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@echo "Clean $(this_name)..."
	make subobjs TGT=clean
	-rm -f *.o *.a *.so $(this_name) *.d *.d.[0-9]*

install:
	@echo "Install $(this_name) to rootfs"
	cp -f $(this_name) $(INSTALL_ROOT)/sbin/

ifeq ($(SILENT),@)
.SILENT:
endif
