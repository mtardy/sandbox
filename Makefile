CC=cc
PG=leg

cparser: calc.c
	$(CC) -o calc calc.c

calc.c: calc.leg
	$(PG) calc.leg > calc.c

clean:
	rm calc.c calc
