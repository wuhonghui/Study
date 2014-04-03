# This Makefile is created by wuhonghui.
# How to use:
# 	1. add cflags and ldflags if needed.
# 	2. add sub-objects
# 	3. un-comment target command

SILENT ?= @
CROSS ?=
CC = $(CROSS)gcc
AR = $(CROSS)ar
LD = $(CROSS)ld
RANLIB = $(CROSS)ranlib

# Common cflags
COMMON_CFLAGS ?= -Wall -O2 -fno-common -fomit-frame-pointer -pipe -D_REENTRANT -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -fno-common 

# DEBUG=1
ifdef DEBUG
    COMMON_CFLAGS += -DDEBUG -g
endif

export COMMON_CFLAGS

# cflags and ldflags
CFLAGS = $(COMMON_CFLAGS)
CFLAGS += -I./ 
LDFLAGS +=

# list source files
target = $(shell basename `pwd`)
clist = $(shell ls *.c 2>/dev/null)
depends = $(patsubst %.c, %.o, $(clist))

# add objs in sub-directories
SUBOBJS = modules/build-modules.o

# modify Makefile targets
build-$(target).o: $(depends) subobjs
	@echo -e "  LD\t$@"
	$(LD) -r -o $@ $(depends) $(SUBOBJS)
#	@echo -e "  LD\t$(target).so"
#	$(LD) -shared -fPIC -soname=$(target).so $(LDFLAGS) -o $(target).so $(depends) $(SUBOBJS) 
	@echo -e "  Build\t$(target)"
	$(CC) -o $(target) $@ $(LDFLAGS)

subobjs: $(patsubst %, __%__, $(dir $(SUBOBJS)))

$(patsubst %, __%__, $(dir $(SUBOBJS))):
	make -C $(patsubst __%__, %, $@) $(TGT)

ifneq ($(clist), )
-include $(clist:.c=.d)
endif

%.d:%.c
	@set -e;rm -f $@;\
	$(CC) -MM $(CFLAGS) $< > $@.$$$$;\
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; 
	@rm -rf $@.[0-9]*

%.o:%.c
	@echo -e "  CC\t$<"
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@echo -e "  Clean\t$(target)"
	make subobjs TGT=clean
	-rm -f *.o *.a $(target).so $(target) *.d *.d.[0-9]*

ifeq ($(SILENT),@)
ifeq ($(MAKELEVEL),0)
.SILENT:
endif
endif
