CC := clang
CFLAGS := -g -O0 -static -std=c11
SRCS := $(filter-out tmp.c, $(wildcard *.c))
OBJS := $(SRCS:.c=.o)
TMP := build
TMP2 := build2

kcc: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): kcc.h

test: kcc
	./kcc -I tests/ -I/usr/lib/gcc/x86_64-linux-gnu/9/include/ tests/tests.c > tmp.s
	cc -static -g -o tmp tmp.s
	./tmp

self2: self
	./self-compile.sh $(TMP)/kcc $(TMP2)
	./$(TMP2)/kcc -Itests tests/tests.c > tmp.s
	cc -static -g -o tmp tmp.s
	./tmp

self: kcc
	./self-compile.sh kcc $(TMP)
	./$(TMP)/kcc -Itests tests/tests.c > tmp.s
	cc -static -g -o tmp tmp.s
	./tmp



clean:
	rm -rf kcc *.o tmp* *.s build/*

.PHONY: test clean
