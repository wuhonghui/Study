SILENT ?= @
CROSS ?=
CC ?= $(CROSS)gcc
AR ?= $(CROSS)ar
LD ?= $(CROSS)ld
RANLIB ?= $(CROSS)ranlib

CFLAGS = $(COMMON_CFLAGS)
CFLAGS += -I./ -I../include

OBJ = librtsp.o

depends = $(patsubst %.c, %.o, $(shell ls *.c))

$(OBJ): $(depends)
	@echo "  LD $@"
	$(LD) -r -o $@ $^

.PHONY:clean

clist = $(shell ls *.c)
-include $(clist:.c=.d)

%.d:%.c
	@set -e;rm -f $@;\
	$(CC) -MM $(CFLAGS) $< > $@.$$$$;\
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; 
	@rm -rf $@.[0-9]*

%.o:%.c
	@echo "  CC $<"
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-rm -f *.o *.a *.so *.d *.d.[0-9]*
	@echo "  Clean $(OBJ)"
