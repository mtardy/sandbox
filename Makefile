CC=cc
PG=leg
LDFLAGS=-L/usr/local/lib -lgc

cparser: calc.c
	$(CC) $(LDFLAGS) -o calc calc.c

calc.c: calc.leg
	$(PG) -o calc.c calc.leg

obj: drafts/draft_object.c
	$(CC) $(LDFLAGS) -o draft drafts/draft_object.c

clean:
	rm calc.c calc draft
