CC=gcc
CCFLAGS=-Wall -O3 -DTESTS -DCONFIG_LIBC_FLOATINGPOINT

TARGET=test
SRCS=\
	tests-main.c\
	../../src/cml_dom.c\
	../../src/string_builder.c\
	../../src/cml_stax_reader.c\
	../../src/cml_dom_reader.c\
	../../src/utf8.c

OBJS=$(SRCS:.c=.o)
DEPS=$(SRCS:.c=.depends)

.PHONY: clean all

all: $(TARGET) $(DEPS)

$(TARGET): $(OBJS)
	$(CC) $(CCFLAGS) $(OBJS) -o $(TARGET)

.c.o:
	$(CC) $(CCFLAGS) -c $< -o $@

%.depends: %.c
	@set -e; rm -f $@; \
	$(CC) -M $(CCFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)

-include $(DEPS)
