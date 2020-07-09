#include <stdbool.h>
#include <sysexits.h>
#include <assert.h>
#include <gc.h>    // NEVER, EVER HAVE TO CALL FREE (EVER) AGAIN (YES, REALLY)

#define malloc(n)	GC_MALLOC(n)
#define realloc(o, n)	GC_REALLOC(o, n)

typedef enum { Undefined, Integer, String, Symbol, Map } type_t;

union object;
typedef union object *oop;

struct Undefined {
	type_t type;
};

struct Integer {
	type_t type;
	int    value;
};

struct String {
	type_t	type;
	char   *value;
	size_t	size;
};

struct Symbol {
	type_t  type;
	char   *name;
#   if defined(SYMBOL_PAYLOAD)
	SYMBOL_PAYLOAD;
#   endif // defined(SYMBOL_PAYLOAD)
};

struct Pair {
	oop key;
	oop value;
};

struct Map {
	type_t 			type;
    struct Pair	   *elements;  // even are keys, odd are values   [ key val key val key val ]
    size_t			size;
    size_t  		capacity;
};

union object {
	type_t           type;
	struct Undefined Undefined;
	struct Integer   Integer;
	struct String	 String;	
	struct Symbol    Symbol;
	struct Map		 Map;
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

#define get(PTR, TYPE, FIELD)			(_checkType(PTR, TYPE)->TYPE.FIELD)
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

oop makeString(char *value) {
	oop newString = memcheck(malloc(sizeof(union object)));
	newString->type = String;
	newString->String.value = memcheck(strdup(value));
	newString->String.size = strlen(value);
	return newString;
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

oop makeMap() {
	oop newMap = memcheck(malloc(sizeof(union object)));
	newMap->type = Map;
	return newMap;
}

ssize_t map_search(oop map, oop key)
{
	assert(map);  assert(key);
	ssize_t l = 0, r = get(map, Map, size) - 1;
	while (l <= r) {
		ssize_t mid = (l + r) / 2;
		int cmpres = strcmp(get(get(map, Map, elements)[mid].key, String, value), get(key, String, value));
		if      (cmpres > 0) 	r = mid - 1;
		else if (cmpres < 0) 	l = mid + 1;
		else 					return mid;  // non-negative result => element found at this index
	}
	return -1 - l;  // negative result => 'not found', reflected around -1 instead of 0 to allow 'not found' at index 0
}

oop map_get(oop map, oop key) {
	assert(is(Map, map));
	assert(is(String, key));
	ssize_t pos = map_search(map, key);
	if (pos < 0) return null;
    return get(map, Map, elements)[pos].value;
}

#define MAP_CHUNK_SIZE 8

oop map_set(oop map, oop key, oop value) {
	assert(is(Map, map));
	assert(is(String, key));
	assert(value);
	ssize_t pos = map_search(map, key);
	if (pos >= 0) {
		get(map, Map, elements)[pos].value = value;
		// In your opinion, which is better in C
		// - Writing "return map" here and then write the rest of the function's code flat
		// - Or use this if / else statement (like here) because of the symmetry of the pb 
		//   and the fact that we return the same stuff anyway
	} else {
		pos = -1 - pos;
		// check capacity and expand if needed
		if (get(map, Map, size) >= get(map, Map, capacity)) {
			size_t newCapacity = get(map, Map, capacity) + MAP_CHUNK_SIZE;
			set(map, Map, elements, memcheck(realloc(
				get(map, Map, elements), 
				sizeof(struct Pair) * newCapacity))
			);
			set(map, Map, capacity, newCapacity);
		}
		// insert
		memmove(get(map, Map, elements) + pos + 1, get(map, Map, elements) + pos, sizeof(struct Pair) * get(map, Map, size) - pos);
		// Maybe this syntax is not very nice and I should access the Pair stuff differently?
		// I mean modifying something on a line that begin with "get"... :/
		get(map, Map, elements)[pos].value = value;
		get(map, Map, elements)[pos].key = key;
		set(map, Map, size, ++get(map, Map, size));
	}
    return map;
}

oop map_del(oop map, oop key) {
	assert(is(Map, map));
	assert(is(String, key));
	ssize_t pos = map_search(map, key);
	if (pos < 0) return map;
	if (pos < get(map, Map, size) - 1) {
		memmove(get(map, Map, elements) + pos, get(map, Map, elements) + pos + 1, sizeof(struct Pair) * get(map, Map, size) - pos);
	}
	set(map, Map, size, --get(map, Map, size));
    return map;
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
	case String:
		printf("'%s'", get(ast, String, value));
		return;
	case Symbol:
		printf("%s=", get(ast, Symbol, name));
		print(get(ast, Symbol, value));
		return;
	case Map:
		printf("{");
		for (size_t i = 0; i < get(ast, Map, size); i++) {
			printf(" ");
			// I could write this instead but I want a special print for my string key name
			// print(get(ast, map, elements)[i].key);
			printf("%s", get(get(ast, Map, elements)[i].key, String, value));
			printf(": ");
			print(get(ast, Map, elements)[i].value);
			if (i < get(ast, Map, size) - 1) printf(",");
			else printf(" ");
		}
		printf("}");
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
