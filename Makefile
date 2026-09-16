CC ?= cc
PKG_CONFIG ?= pkg-config
CPPFLAGS += -Isrc $(shell $(PKG_CONFIG) --cflags raylib 2>/dev/null)
CFLAGS ?= -O2 -g
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic
LDLIBS += $(shell $(PKG_CONFIG) --libs raylib 2>/dev/null) -lm
.PHONY: all run test clean check-deps
all: build/pacman
check-deps:
	@$(PKG_CONFIG) --exists raylib || { echo 'raylib development files and pkg-config are required. See README.md.'; exit 1; }
build:
	mkdir -p $@
build/pacman: src/main.c src/game.c src/game.h | build check-deps
	$(CC) $(CPPFLAGS) $(CFLAGS) src/main.c src/game.c -o $@ $(LDFLAGS) $(LDLIBS)
run: build/pacman
	./build/pacman
build/test_game: tests/test_game.c src/game.c src/game.h | build
	$(CC) -Isrc $(CFLAGS) tests/test_game.c src/game.c -o $@ $(LDFLAGS) -lm
test: build/test_game
	./build/test_game
clean:
	rm -rf build
