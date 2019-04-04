CC=clang
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')


ifeq ($(uname_S),SunOS)
	CC=/opt/gcc-8/bin/gcc
endif

ifeq ($(uname_S),SunOS)
	CFLAGS=-m64 -std=gnu99 -Wall -Wextra -I/usr/local/include -I/usr/local/include/sodium
	# -fsanitize=undefined -ggdb3 -O0 -std=c99 -Wall \
	# -Werror -Wextra -Wno-sign-compare \
	# -Wno-unused-parameter -Wno-unused-variable -Wshadow
endif
ifeq ($(uname_S),Linux)
	LDFLAGS=$(shell pkg-config --libs libsodium) -lpthread -luuid -lrt -lpthread -lnsl -ldl
	PROGNAME=ranbuffer.linux
endif
ifeq ($(uname_S),Darwin)
	LDFLAGS=$(shell pkg-config --libs libsodium) -lpthread -luuid -lpthread -ldl
	LDFLAGS_NO_SODIUM=-lpthread -luuid -lpthread -ldl
	PROGNAME=ranbuffer.macosx
endif
ifeq ($(uname_S),SunOS)
	LDFLAGS=/usr/local/lib/libsodium.a -lssp -luuid -lpthread -ldl
	PROGNAME=ranbuffer.solaris
endif
objects = name.o thread.o genfile.o units.o ranbuffer.o

ranbuffer: $(objects)
		$(CC) $(CFLAGS) $(objects) $(LDFLAGS) -o $(PROGNAME)

ranbuffer-static-libsodium: $(objects)
		$(CC) $(CFLAGS) libsodium.a $(objects) \
		$(LDFLAGS_NO_SODIUM) -o ranbuffer

ranbuffer-debug: $(objects)
		$(CC) $(CFLAGS) -DDEBUG \
		libsodium.a $(objects) \
		$(LDFLAGS) -o ranbuffer

ranbuffer-debug-static-libsodium: $(objects)
		$(CC) $(CFLAGS) -DDEBUG \
		libsodium.a $(objects) \
		$(LDFLAGS_NO_SODIUM) -o ranbuffer

clean:
	rm -rf *.o *.dSYM