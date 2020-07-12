CC :=clang
CFLAGS := -g -O0 -static -std=c11
SRCS := $(filter-out tmp.c, $(wildcard *.c))
OBJS := $(SRCS:.c=.o)
TMP := build

kcc: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): kcc.h

test: kcc
	./kcc -I tests/ -I/usr/lib/gcc/x86_64-linux-gnu/9/include/ tests/tests.c > tmp.s
	cc -static -g -o tmp tmp.s
	./tmp

self: kcc
	./self-compile.sh $(TMP)
	./$(TMP)/kcc -Itests tests/tests.c > tmp.s
	cc -static -g -o tmp tmp.s
	./tmp


clean:
	rm -f kcc *.o tmp*

.PHONY: test clean
