CC := gcc
CFLAGS := -Wall -Wno-vla -O3 -I./include
SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c,%.o,$(SRCS))
OUT := tiny

all: $(OUT)

$(OUT): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

depend: .depend

.depend: $(SRCS)
	rm -f $@
	$(CC) $(CFLAGS) -MM $^ -MF $@

include .depend

.PHONY: clean

clean:
	rm -fR $(OUT) $(OBJS) .depend
