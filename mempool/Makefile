CFLAGS= -Wall -O3  -D_REENTRANT

LDFLAGS= -Wall -Wl,-allow-shlib-undefined -lpthread -lrt -lc

ifeq ($(strip $(DEBUG)), 1)
CFLAGS += -g -static
else
CFLAGS += -s
endif

OBJS = mempool.o

mempool.so : $(OBJS) test
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(EXTRA_LDFLAGS) -shared -o $@ $(OBJS)

mempool.o: mempool.c
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(CFLAGS_$@) -c -o $@ $<

test:test.o mempool.o
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(EXTRA_LDFLAGS) -o $@ test.o mempool.o $(LDFLAGS)

test.o:test.c
	$(CC) $(CFLAGS)  $(EXTRA_CFLAGS) $(CFLAGS_$@) -c -o $@ $<


clean:
	-rm -f $(OBJS) mempool.o mempool.so test.o test
