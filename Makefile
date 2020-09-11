LEG	= leg
CC	= cc
CFLAGS	= -I/usr/local/include -I/opt/local/include -Wall -Wno-unused-label -g
LDLIBS	= -L/usr/local/lib -L/opt/local/lib -lgc -lm

all : parse

# moved LDLIBS to end because ld scans files from left to right and collects only required symbols

%: %.c object.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

%.c: %.leg
	$(LEG) -o $@ $<

clean:
	rm -f parse parse.c

opt:
	$(MAKE) CFLAGS="-DNDEBUG -O3 -fomit-frame-pointer"
