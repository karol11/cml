CXX=gcc
CXXFLAGS=-Wall -O3 -DTESTS

TARGET=test
SRCS=tests-main.c ../src/dom.c ../src/string_builder.c

OBJS=$(SRCS:.c=.o)
DEPS=$(SRCS:.c=.depends)

.PHONY: clean all

all: $(TARGET) $(DEPS)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(OBJS) -o $(TARGET)

.c.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.depends: %.c
	$(CXX) -M $(CXXFLAGS) $< > $@

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)
