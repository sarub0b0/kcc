CFLAGS= -std=c++14 -g -static

kcc: kcc.cc

test: kcc
	./test.sh

clean:
	rm -f kcc *.o tmp

.PHONY: test clean
