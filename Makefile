CC := clang
CFLAGS := -g -O0 -static -Wno-switch -std=c11
SRCS := $(filter-out tmp.c, $(wildcard *.c))
OBJS := $(SRCS:.c=.o)

kcc: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): kcc.h

test: kcc
	./kcc -I tests/ tests/tests.c > tmp.s
	cc -static -g -o tmp tmp.s
	./tmp

clean:
	rm -f kcc *.o tmp*

.PHONY: test clean
