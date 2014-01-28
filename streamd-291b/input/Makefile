SILENT ?= @
CROSS ?= 
CC = $(CROSS)gcc
AR = $(CROSS)ar
LD = $(CROSS)ld
RANLIB = $(CROSS)ranlib

CFLAGS = $(COMMON_CFLAGS)
CFLAGS += -I./ -DDEBUG -I../include -I../mediaframe/

this_name = $(shell basename `pwd`)

depends = $(patsubst %.c, %.o, $(shell ls *.c))

$(this_name).o: $(depends)
	@echo -e "Make archieve $@ with $^\n"
	$(LD) -r -o $@ $^

.PHONY:clean

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
	-rm -f *.o *.a *.so *.d *.d.[0-9]*
