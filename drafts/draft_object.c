#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sysexits.h>
#include <assert.h>
#include <gc.h>    // NEVER, EVER HAVE TO CALL FREE (EVER) AGAIN (YES, REALLY)
#define malloc(n) GC_MALLOC(n)
#define realloc(o, n) GC_REALLOC(o, n)

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
		exit(EX_OSERR);  // this is as close as we have for 'resource unavailable'
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
	oop newSymb = memcheck(malloc(sizeof(union object)));
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

#define SYMBOL_TABLE_CHUNK 4

typedef struct table_t {
	oop    *array;
	size_t	size;
	size_t 	capacity;
} table_t;

#define TABLE_INITIALISER { NULL, 0, 0 } // first call to table_insert() will initialise storage

table_t table = TABLE_INITIALISER;       // safe but not strictly needed on Unix because BSS segment is initialised to all zeroes

ssize_t table_search(table_t *table, char *ident)
{
	ssize_t l = 0, r = table->size - 1; 
	while (l <= r) {
		ssize_t mid = (l + r) / 2;
		int cmpres = strcmp(get(table->array[mid], Symbol, name), ident);
		if      (cmpres > 0) 	r = mid - 1;
		else if (cmpres < 0) 	l = mid + 1;
		else 					return mid;  // non-negative result => element found at this index
	}
	return -1 - l;  // negative result => 'not found', reflected around -1 instead of 0 to allow 'not found' at index 0
}

// ssize_t result because -1 means 'error'
ssize_t table_insert(table_t *table, oop object, size_t pos)
{
	// Should I use in my code a function starting with _ or is it a convention to prevent its usage ?
	_checkType(object, Symbol);
	if (pos > table->size) {  // don't need to check for pos < 0 because size_t is unsigned
		return -1;
	}

	if (table->size  >= table->capacity) {
		// on the first call table->array will be NULL and realloc() will behave like malloc()
		table->array = memcheck(realloc(table->array, sizeof(oop) * (table->capacity + SYMBOL_TABLE_CHUNK)));
		table->capacity += SYMBOL_TABLE_CHUNK;
	}

	memmove(table->array + pos + 1, table->array + pos, sizeof(*table->array) * (table->size - pos));
	table->array[pos] = object;
	return ++(table->size);
}

oop intern(char *ident)
{
	ssize_t res= table_search(&table, ident);  // < 0 => not found
	if (res >= 0) return table.array[res];
	res= -1 - res; // 'un-negate' the result by reflecting it around X=-1
    oop new_symbol = makeSymbol(memcheck(strdup(ident)));
	table_insert(&table, new_symbol, res);
	return new_symbol;
}

oop update_symbol_value(oop symbol, oop integer)
{
	_checkType(symbol, Symbol);
	_checkType(integer, Integer);
	symbol->Symbol.value = integer;
	return symbol;
}

int main()
{
	char *line= 0;      	    // this and
	size_t linecap= 0;  	    // this are needed for getline()
	intern("chaussure");  // identifiers will have no trailing newline so let's test with no trailing newline
	printf("Enter identifier names!\n");
	for (;;) {                  // using an infinite loop simplifies the break/continue logic in the body
		ssize_t len= getline(&line, &linecap, stdin);              // use getline() to auto-grow the buffer when necessary
		if (len < 0) break;                                        // stop at EOF
		while ((len > 0) && ('\n' == line[len-1])) line[--len]= 0; // trim newlines from the end
		if (len < 1) continue;                                     // ignore empty lines
		printf("intern : %p\n", intern(line));
		printf("after size : %zi\n", table.size);
		printf("after capacity : %zi\n", table.capacity);
		printf("\n");
		for (int i = 0; i < table.size; i++) {
			printf("%i %s\n", i, get(table.array[i], Symbol, name));
		}
		printf("\n");
	}
}
