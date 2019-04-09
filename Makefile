CC=clang
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')

ifeq ($(uname_S),SunOS)
	CC=/opt/gcc-8/bin/gcc
	ifdef DEBUG
		CFLAGS+=-DDEBUG
	endif
	CFLAGS=-m64 -std=gnu99 -Wall -Wextra -I/usr/local/include -I/usr/local/include/sodium
	# -fsanitize=undefined -ggdb3 -O0 -std=c99 -Wall \
	# -Werror -Wextra -Wno-sign-compare \
	# -Wno-unused-parameter -Wno-unused-variable -Wshadow
endif
ifeq ($(uname_S),Linux)
	ifdef DEBUG
		CFLAGS+=-DDEBUG
	endif
	LDFLAGS=$(shell pkg-config --libs libsodium) -lpthread -luuid -lrt -lpthread -lnsl -ldl -lm
	PROGNAME=ranbuffer.linux
endif
ifeq ($(uname_S),Darwin)
	ifdef DEBUG
		CFLAGS+=-DDEBUG
	endif
	LDFLAGS=$(shell pkg-config --libs libsodium) -lpthread -luuid -lpthread -ldl
	LDFLAGS_NO_SODIUM=-lpthread -luuid -lpthread -ldl -lm
	PROGNAME=ranbuffer.macosx
endif
ifeq ($(uname_S),SunOS)
	LDFLAGS=/usr/local/lib/libsodium.a -lssp -luuid -lpthread -ldl -lm
	PROGNAME=ranbuffer.solaris
endif
objects = genbuf.o genfile.o name.o ranbuffer.o thread.o units.o validate.o

ranbuffer: $(objects)
		$(CC) $(CFLAGS) $(objects) $(LDFLAGS) -o $(PROGNAME)

ranbuffer-static-libsodium: $(objects)
		$(CC) $(CFLAGS) libsodium.a $(objects) \
		$(LDFLAGS_NO_SODIUM) -o $(PROGNAME)

ranbuffer-debug: $(objects)
		$(CC) -DDEBUG $(CFLAGS) \
		libsodium.a $(objects) \
		$(LDFLAGS) -o $(PROGNAME)

ranbuffer-debug-static-libsodium: $(objects)
		$(CC) $(CFLAGS) -DDEBUG \
		libsodium.a $(objects) \
		$(LDFLAGS_NO_SODIUM) -o ranbuffer

clean:
	rm -rf *.o *.dSYM