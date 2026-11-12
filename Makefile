CC      ?= gcc
CFLAGS  ?= -std=c99 -O2 -Wall -Wextra -Wpedantic -Werror -Iinclude
LDLIBS  := -lm

SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)

.PHONY: all test sanitize demo clean

all: build/test_runner build/pid_demo

build/test_runner: $(SRC) tests/test_main.c | build
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

build/pid_demo: $(SRC) examples/pid_demo.c | build
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

build:
	mkdir -p build

test: build/test_runner
	./build/test_runner

# Same tests with AddressSanitizer + UndefinedBehaviorSanitizer enabled.
sanitize: | build
	$(CC) $(CFLAGS) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
		$(SRC) tests/test_main.c -o build/test_runner_asan $(LDLIBS)
	./build/test_runner_asan

demo: build/pid_demo
	./build/pid_demo > build/pid_step.csv
	@echo "CSV written to build/pid_step.csv"

clean:
	rm -rf build
