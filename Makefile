CC := clang
CFLAGS := -g -O0 -static -std=c11
SRCS := $(filter-out tmp.c, $(wildcard *.c))
OBJS := $(SRCS:.c=.o)
TMP1 := build1
TMP2 := build2
TMP3 := build3

kcc: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): kcc.h

test: kcc
	./kcc -I tests tests/tests.c > tmp.s
	cc -static -g -o tmp tmp.s
	./tmp

test1: self1
	./$(TMP1)/kcc -Itests tests/tests.c > tmp-self1.s
	cc -static -g -o tmp-self1 tmp-self1.s
	./tmp-self1

test2: self2
	./$(TMP2)/kcc -Itests tests/tests.c > tmp-self2.s
	cc -static -g -o tmp-self2 tmp-self2.s
	./tmp-self2

test3: self3
	./$(TMP3)/kcc -Itests tests/tests.c > tmp-self3.s
	cc -static -g -o tmp-self3 tmp-self3.s
	./tmp-self3


self1: kcc
	./self-compile.sh kcc $(TMP1)

self2: self1
	./self-compile.sh $(TMP1)/kcc $(TMP2)

self3: self2
	./self-compile.sh $(TMP2)/kcc $(TMP3)


diff12: self1 self2
	diff build1/ build2/

diff23: self2 self3
	diff build2/ build3/


clean:
	rm -rf kcc *.o tmp* *.s build/ build1/ build2/ build3/

.PHONY: test clean
