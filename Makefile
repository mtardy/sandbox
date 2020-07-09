LEG	= leg
CC	= cc
CFLAGS	= -Wall -g
LDLIBS	= -L/usr/local/lib -lgc

# moved LDLIBS to end because ld scans files from left to right and collects only required symbols

calc: calc.c object.c
	$(CC) $(CFLAGS) -o calc calc.c $(LDLIBS)

calc.c: calc.leg
	$(LEG) -o calc.c calc.leg

obj: drafts/draft_object.c
	$(CC) -o draft drafts/draft_object.c $(LDLIBS)

clean:
	rm calc.c calc draft
