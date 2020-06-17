CC=cc
PG=leg

cparser: calc.c
	$(CC) -o calc calc.c

calc.c: calc.leg
	$(PG) -o calc.c calc.leg

clean:
	rm calc.c calc
