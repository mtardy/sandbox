LEG	= leg
CC	= cc
CFLAGS	= -Wall -g
LDLIBS	= -L/usr/local/lib -lgc

# moved LDLIBS to end because ld scans files from left to right and collects only required symbols

calc: parse.c object.c
	$(CC) $(CFLAGS) -o parse parse.c $(LDLIBS)

parse.c: parse.leg
	$(LEG) -o parse.c parse.leg

calc.c: calc.leg
	$(LEG) -o calc.c calc.leg

obj: drafts/draft_object.c
	$(CC) -o draft drafts/draft_object.c $(LDLIBS)

clean:
	rm calc.c calc draft
