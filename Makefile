CC := clang
CFLAGS := -g -O0 -static -Wno-switch
SRCS := $(filter-out tmp.c, $(wildcard *.c))
OBJS := $(SRCS:.c=.o)

kcc: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): kcc.h

test: kcc
	./test.sh

clean:
	rm -f kcc *.o tmp*

.PHONY: test clean
