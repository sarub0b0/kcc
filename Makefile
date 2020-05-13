CC := clang
CFLAGS := -g -O0 -static -Wno-switch
SRCS := $(filter-out foo.c, $(wildcard *.c))
OBJS := $(SRCS:.c=.o)

kcc: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): kcc.h

foo: foo.c
	cc -O0 -g -c $<

test: kcc foo
	./test.sh

clean:
	rm -f kcc *.o tmp*

.PHONY: test clean
