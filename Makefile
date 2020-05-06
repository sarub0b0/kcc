CXX := clang++
CXXFLAGS := -std=c++14 -g -O0 -static
SRCS := $(wildcard *.cc)
OBJS := $(SRCS:.cc=.o)

kcc: $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): kcc.h

test: kcc
	./test.sh

clean:
	rm -f kcc *.o tmp*

.PHONY: test clean
