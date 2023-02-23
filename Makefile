VERSION = 1.3.1

CC     ?= gcc
CFLAGS ?= -g
CFLAGS += -W -Wall -pedantic -ansi -std=c99 -DVERSION=\"$(VERSION)\"

# The Ncurses library with wide character support is available as "lncurses"
# under macOS.
ifeq ($(shell uname -s),Darwin)
	LDFLAGS += -L/usr/local/opt/ncurses/lib -lncurses
	CFLAGS += -I/usr/local/opt/ncurses/include
else
	LDFLAGS += -lncursesw
endif

PREFIX ?= /usr/local

EXECUTABLE  = ttysolitaire
SRC_DIR     = src
SRC         = $(wildcard $(SRC_DIR)/*.c)
SRC_OBJECTS = $(SRC:%.c=%.o)

TESTS_EXECUTABLE = ttysolitaire_test
TESTS_DIR        = tests
TEST_SRC         = $(wildcard $(TESTS_DIR)/*.c)
TESTS_OBJECTS    = $(TEST_SRC:%.c=%.o)

.PHONY: test clean install uninstall

all: $(EXECUTABLE)

$(EXECUTABLE): $(SRC_OBJECTS)
	$(CC) $(CFLAGS) -o $(EXECUTABLE) $^ $(LDFLAGS)

$(TESTS_EXECUTABLE): $(TESTS_OBJECTS) $(filter-out src/ttysolitaire.o,$(SRC_OBJECTS))
	$(CC) $(CFLAGS) -o $(TESTS_EXECUTABLE) $^ $(LDFLAGS)

test: $(TESTS_EXECUTABLE)
	./$<

$(SRC_OBJECTS) $(TESTS_OBJECTS): %.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(SRC_DIR)/*.o $(TESTS_DIR)/*.o $(EXECUTABLE) $(TESTS_EXECUTABLE)

install:
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m755 $(EXECUTABLE) $(DESTDIR)$(PREFIX)/bin/$(EXECUTABLE)

uninstall:
	rm -f $(PREFIX)/bin/$(EXECUTABLE)
