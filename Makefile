LEG	= leg
CC	= cc
CFLAGS	= -Wall -g
LDLIBS	= -L/usr/local/lib -lgc

# moved LDLIBS to end because ld scans files from left to right and collects only required symbols

parse: parse.c object.c
	$(CC) $(CFLAGS) -o parse parse.c $(LDLIBS)

parse.c: parse.leg
	$(LEG) -o parse.c parse.leg

clean:
	rm parse parse.c
