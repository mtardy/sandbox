#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sysexits.h>
#include <gc.h>    // NEVER, EVER HAVE TO CALL FREE (EVER) AGAIN (YES, REALLY)
#define malloc(n) GC_MALLOC(n)

typedef enum { Undefined, Integer, Symbol } type_t;

union object;
typedef union object *oop;

struct Undefined {
    type_t type;
};

struct Integer {
    type_t type;
    int    value;
};

struct Symbol {
    type_t  type;
    char   *name;
    int     defined;
    oop     value; // NULL
};

union object {
    type_t           type;
    struct Undefined Undefined;
    struct Integer   Integer;
    struct Symbol    Symbol;
};

union object _null = { .Undefined = { Undefined } };
oop null = &_null;

oop _checkType(oop ptr, type_t type) { 
	assert(ptr->type == type); 
	return ptr; 
}

#define get(PTR, TYPE, FIELD) _checkType(PTR, TYPE)->TYPE.FIELD
#define set(PTR, TYPE, FIELD, VALUE) _checkType(PTR, TYPE)->TYPE.FIELD = VALUE

void *memcheck(void *ptr)
{
    if (NULL == ptr) {
		fprintf(stderr, "Error: out of memory\n");
		exit(EX_OSERR);
    }
    return ptr;
}

oop makeInteger(int value) {
	oop newInt = memcheck(malloc(sizeof(union object)));
	newInt->type = Integer;
	newInt->Integer.value = value;
	return newInt;
}

oop makeSymbol(char *name) {
	oop newSymb = memcheck(calloc(1, sizeof(union object)));
	newSymb->type = Symbol;
	newSymb->Symbol.name = name;
	newSymb->Symbol.defined = 0;
	newSymb->Symbol.value = null;
	return newSymb; 
}

void print(oop ast) {
    switch (ast->type) {
	case Undefined:
	    printf("null");
	    return;
	case Integer:
	    printf("%i", get(ast, Integer, value));
	    return;
	case Symbol:
	    printf("%s=", get(ast, Symbol, name));
	    print(get(ast, Symbol, value));
	    return;
    }
    assert(0);
}

void println(oop ast)
{
    print(ast);
    printf("\n");
}

int main(int argc, char **argv)
{
    oop i = makeInteger(42);
	oop s = makeSymbol("symbolic");

    println(i);
    println(s);

    return 0;
}
