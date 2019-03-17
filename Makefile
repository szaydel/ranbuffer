CC=clang
CFLAGS=-m64 -fsanitize=signed-integer-overflow \
	-fsanitize=undefined -ggdb3 -O0 -std=c11 -Wall \
	-Werror -Wextra -Wno-sign-compare \
	-Wno-unused-parameter -Wno-unused-variable -Wshadow

uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
ifeq ($(uname_S),Linux)
	LDFLAGS=$(shell pkg-config --libs libsodium) -lpthread -luuid -lrt -lpthread -lnsl -ldl
endif
ifeq ($(uname_S),Darwin)
	LDFLAGS=$(shell pkg-config --libs libsodium) -lpthread -luuid -lpthread -ldl
	LDFLAGS_NO_SODIUM=-lpthread -luuid -lpthread -ldl
endif

units.o: units.c
		$(CC) -c units.c

ranbuffer: ranbuffer.c units.o
		$(CC) $(CFLAGS) units.o ranbuffer.c $(LDFLAGS) -o ranbuffer

ranbuffer-static-libsodium: ranbuffer.c units.o
		$(CC) $(CFLAGS) libsodium.a units.o ranbuffer.c \
		$(LDFLAGS_NO_SODIUM) -o ranbuffer

ranbuffer-debug: ranbuffer.c units.o
		$(CC) $(CFLAGS) -DDEBUG \
		libsodium.a units.o ranbuffer.c \
		$(LDFLAGS) -o ranbuffer

ranbuffer-debug-static-libsodium: ranbuffer.c units.o
		$(CC) $(CFLAGS) -DDEBUG \
		libsodium.a units.o ranbuffer.c \
		$(LDFLAGS_NO_SODIUM) -o ranbuffer

clean:
	rm -rf *.o *.dSYM