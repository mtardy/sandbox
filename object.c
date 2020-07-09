#include <stdbool.h>
#include <sysexits.h>
#include <assert.h>
#include <gc.h>    // NEVER, EVER HAVE TO CALL FREE (EVER) AGAIN (YES, REALLY)

#define malloc(n)	GC_MALLOC(n)
#define realloc(o, n)	GC_REALLOC(o, n)

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
#   if defined(SYMBOL_PAYLOAD)
	SYMBOL_PAYLOAD;
#   endif // defined(SYMBOL_PAYLOAD)
};

union object {
	type_t           type;
	struct Undefined Undefined;
	struct Integer   Integer;
	struct Symbol    Symbol;
};

union object _null = { .Undefined = { Undefined } };
oop null = &_null;

type_t getType(oop ptr) {
	assert(ptr);
	return ptr->type;
}

int is(type_t type, oop obj) {
	return type == getType(obj);
}

oop _checkType(oop ptr, type_t type) {
	assert(ptr);
	assert(ptr->type == type);
	return ptr;
}

// added parens around expansion to protect assignment

#define get(PTR, TYPE, FIELD)		(_checkType(PTR, TYPE)->TYPE.FIELD)
#define set(PTR, TYPE, FIELD, VALUE)	(_checkType(PTR, TYPE)->TYPE.FIELD = VALUE)

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
#   if defined(SYMBOL_INITIALISE)
	SYMBOL_INITIALISE(newSymb->Symbol);
#   endif // defined(SYMBOL_INITIALISE)
	return newSymb;
}

void print(oop ast) {
	assert(ast);
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

#define SYMBOL_TABLE_CHUNK 1024

typedef struct table_t {
	oop    *array;
	size_t	size;
	size_t 	capacity;
} table_t;

#define TABLE_INITIALISER { NULL, 0, 0 } // first call to table_insert() will initialise storage

table_t table = TABLE_INITIALISER;       // safe but not strictly needed on Unix because BSS segment is initialised to all zeroes

ssize_t table_search(table_t *table, char *ident)
{
	assert(table);  assert(ident);
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
	/// You should never really have to use that function except implicitly via get/set.
	/// If you need to insist on a particular type, check it explicitly and produce a real error messge or assertion failure.
	assert(is(Symbol, object));
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
