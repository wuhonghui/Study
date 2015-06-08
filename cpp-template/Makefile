# Author: wuhonghui
# How to use:
# 	1. add cflags, ldflags if needed
# 	2. add sub-objects
# 	3. change target if needed

SILENT ?= @
CROSS ?=
ECHO = /bin/echo
CC = $(CROSS)g++
AR = $(CROSS)ar
LD = $(CROSS)ld
RANLIB = $(CROSS)ranlib

# Common cflags
COMMON_CFLAGS ?= -Wall -O2 -fno-common -fomit-frame-pointer -pipe -D_REENTRANT -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -fno-common 

# make DEBUG=1
ifdef DEBUG
    COMMON_CFLAGS += -DDEBUG -g
endif

ifeq ($(MAKELEVEL),0)
export COMMON_CFLAGS
export CROSS
endif

# cflags and ldflags
CFLAGS = $(COMMON_CFLAGS)
CFLAGS += -I./ 
LDFLAGS +=

# list source files
target = $(shell basename `pwd`)
clist = $(shell ls *.cpp 2>/dev/null)
depends = $(patsubst %.cpp, %.o, $(clist))

# add objs in sub-directories
SUBOBJS = modules/build-modules.o

# modify Makefile targets
build-$(target).o: subobjs $(depends)
	@$(ECHO) -e "  LD\t$@"
	$(LD) -r -o $@ $(depends) $(SUBOBJS)
#	@$(ECHO) -e "  LD\t$(target).so"
#	$(LD) -shared -fPIC -soname=$(target).so $(LDFLAGS) -o $(target).so $(depends) $(SUBOBJS) 
	@$(ECHO) -e "  Build\t$(target)"
	$(CC) -o $(target) $@ $(LDFLAGS)

subobjs: $(patsubst %, __%__, $(dir $(SUBOBJS)))

$(patsubst %, __%__, $(dir $(SUBOBJS))):
	make -C $(patsubst __%__, %, $@) $(TGT)

ifneq ($(clist), )
-include $(clist:.cpp=.d)
endif

%.d:%.cpp
	@set -e;rm -f $@;\
	$(CC) -MM $(CFLAGS) $< > $@.$$$$;\
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; 
	@rm -rf $@.[0-9]*

%.o:%.cpp
	@$(ECHO) -e "  CC\t$<"
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	make subobjs TGT=clean
	@$(ECHO) -e "  Clean\t$(target)"
	-rm -f *.o *.a $(target).so $(target) *.d *.d.[0-9]*

ifeq ($(SILENT),@)
ifeq ($(MAKELEVEL),0)
.SILENT:
endif
endif
