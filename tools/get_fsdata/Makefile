CROSS_COMPILE_PATH ?= ../../../toolchains/aarch64/bin/aarch64-linux-gnu

COMPILER ?= $(CROSS_COMPILE_PATH)-gcc

OBJS = main.o

CFLAGS += -Wall -O1 

BINARY = get_data

.PHONY: all
all: clean $(BINARY)

$(BINARY): $(OBJS)
	$(COMPILER) -o $@ $^ 

.PHONY: clean
clean:
	rm -f $(OBJS) $(BINARY)

%.o: %.c
	$(COMPILER) $(CFLAGS) -c $< -o $@
