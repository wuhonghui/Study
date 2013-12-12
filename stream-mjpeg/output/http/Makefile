SILENT ?= @
CROSS ?=
CC ?= $(CROSS)gcc
AR ?= $(CROSS)ar
LD ?= $(CROSS)ld
RANLIB ?= $(CROSS)ranlib

CFLAGS = $(COMMON_CFLAGS)
CFLAGS += -I./ -I../../include -I ../../../libevent/libevent-2.0.21-stable/include/

this_name = $(shell basename `pwd`)

depends = $(patsubst %.c, %.o, $(shell ls *.c))

SUBOBJS = http_handler/http_handler.o

$(this_name).o: $(depends) subobjs
	@echo -e "Make archieve $@ with $(depends) $(SUBOBJS)\n"
	$(LD) -r -o $@ $(depends) $(SUBOBJS)
#	$(CC) $(CFLAGS) -fPIC -shared -o $@ $(depends) $(SUBOBJS)

subobjs: $(patsubst %, __%__, $(dir $(SUBOBJS)))

$(patsubst %, __%__, $(dir $(SUBOBJS))):
	make -C $(patsubst __%__, %, $@) $(TGT)

clist = $(shell ls *.c)
-include $(clist:.c=.d)

%.d:%.c
	@echo "Resolving dependencies for $<..."
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
