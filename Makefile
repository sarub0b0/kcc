CXX := clang++
CXXFLAGS := -std=c++14 -g -O0 -static
SRCS := $(filter-out foo.cc, $(wildcard *.cc))
OBJS := $(SRCS:.cc=.o)

kcc: $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): kcc.h

foo: foo.c
	cc -O0 -g -c $<

test: kcc foo
	./test.sh

clean:
	rm -f kcc *.o tmp*

.PHONY: test clean
