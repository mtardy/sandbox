#include <stdbool.h>
#include <stdint.h>
#include <sysexits.h>
#include <assert.h>
#include <gc.h> // NEVER, EVER HAVE TO CALL FREE (EVER) AGAIN (YES, REALLY)

#define malloc(n)       GC_MALLOC(n)
#define realloc(o, n)   GC_REALLOC(o, n)

typedef enum {
    Undefined,
    Integer,
    String,
    Symbol,
    Function,
    Map
} type_t;

union object;
typedef union object *oop;

struct Undefined {
    type_t type;
};

struct Integer {
    type_t type;
    int value;
};

struct String {
    type_t type;
    char *value;
    size_t size;
};

struct Symbol {
    type_t type;
    char *name;
};

typedef oop (*primitive_t)(oop params);

oop globals;

struct Function {
    type_t type;
    primitive_t primitive;
};

// usefull for map's elements
struct Pair {
    oop key;
    oop value;
};

struct Map {
    type_t type;
    struct Pair *elements; // even are keys, odd are values   [ key val key val key val ]
    size_t size;
    size_t capacity;
};

union object {
    type_t type;
    struct Undefined Undefined;
    struct Integer Integer;
    struct String String;
    struct Symbol Symbol;
    struct Function Function;
    struct Map Map;
};

union object _null = {.Undefined = {Undefined}};
oop null = &_null;

type_t getType(oop ptr)
{
    assert(ptr);
    return ptr->type;
}

int is(type_t type, oop obj)
{
    return type == getType(obj);
}

oop _checkType(oop ptr, type_t type, char *file, int line)
{
    assert(ptr);
    if (ptr->type != type) {
        fprintf(stderr, "\n%s:%i: expected %i got %i\n", file, line, type, ptr->type);
    }
    assert(ptr->type == type);
    return ptr;
}

// added parens around expansion to protect assignment
#define get(PTR, TYPE, FIELD)           (_checkType(PTR, TYPE, __FILE__, __LINE__)->TYPE.FIELD)
#define set(PTR, TYPE, FIELD, VALUE)    (_checkType(PTR, TYPE, __FILE__, __LINE__)->TYPE.FIELD = VALUE)

void *memcheck(void *ptr)
{
    if (NULL == ptr) {
        fprintf(stderr, "Error: out of memory\n");
        exit(EX_OSERR); // this is as close as we have for 'resource unavailable'
    }
    return ptr;
}

oop makeInteger(int value)
{
    oop newInt = memcheck(malloc(sizeof(union object)));
    newInt->type = Integer;
    newInt->Integer.value = value;
    return newInt;
}

oop makeString(char *value)
{
    oop newString = memcheck(malloc(sizeof(union object)));
    newString->type = String;
    newString->String.value = memcheck(strdup(value));
    newString->String.size = strlen(value);
    return newString;
}

oop makeSymbol(char *name)
{
    oop newSymb = memcheck(malloc(sizeof(union object)));
    newSymb->type = Symbol;
    newSymb->Symbol.name = name;
    return newSymb;
}

oop makeFunction(primitive_t primitive)
{
    oop newFunc = memcheck(malloc(sizeof(union object)));
    newFunc->type = Function;
    newFunc->Function.primitive = primitive;
    return newFunc;
}

oop makeMap()
{
    oop newMap = memcheck(malloc(sizeof(union object)));
    newMap->type = Map;
    return newMap;
}

int oopcmp(oop a, oop b)
{
    type_t ta = getType(a), tb = getType(b);
    if (ta == tb) {
        switch (getType(a)) {
        case Integer:
            return get(a, Integer, value) - get(b, Integer, value);
        case String:
            return strcmp(get(a, String, value), get(b, String, value));
        case Symbol:
            return strcmp(get(a, Symbol, name), get(b, Symbol, name));
        default:
            return (intptr_t)a - (intptr_t)b;
        }
    }
    return ta - tb;
}

ssize_t map_search(oop map, oop key)
{
    assert(is(Map, map));
    assert(key);
    ssize_t l = 0, r = get(map, Map, size) - 1;
    while (l <= r) {
        ssize_t mid = (l + r) / 2;
        int cmpres = oopcmp(get(map, Map, elements)[mid].key, key);
        if (cmpres > 0)         r = mid - 1;
        else if (cmpres < 0)    l = mid + 1;
        else                    return mid; // non-negative result => element found at this index
    }
    return -1 - l; // negative result => 'not found', reflected around -1 instead of 0 to allow 'not found' at index 0
}

bool map_hasKey(oop map, oop key)
{
    assert(is(Map, map));
    assert(key);
    return map_search >= 0;
}

oop map_get(oop map, oop key)
{
    assert(is(Map, map));
    assert(key);
    ssize_t pos = map_search(map, key);
    if (pos < 0)    return null;
    return get(map, Map, elements)[pos].value;
}

#define MAP_CHUNK_SIZE 8

oop map_insert(oop map, oop key, oop value, size_t pos)
{
    assert(is(Map, map));
    assert(key);
    assert(value);
    if (pos > get(map, Map, size)) { // don't need to check for pos < 0 because size_t is unsigned
        fprintf(stderr, "\nTrying to insert in a map out of bound\n");
        assert(-1);
    }

    // check capacity and expand if needed
    if (get(map, Map, size) >= get(map, Map, capacity)) {
        size_t newCapacity = get(map, Map, capacity) + MAP_CHUNK_SIZE;
        set(map, Map, elements, memcheck(realloc(get(map, Map, elements), sizeof(struct Pair) * newCapacity)));
        set(map, Map, capacity, newCapacity);
    }

    // insert
    memmove(get(map, Map, elements) + pos + 1, get(map, Map, elements) + pos, sizeof(struct Pair) * (get(map, Map, size) - pos));
    // Maybe this syntax is not very nice and I should access the Pair stuff differently?
    // I mean modifying something on a line that begin with "get"... :/
    get(map, Map, elements)[pos].value = value;
    get(map, Map, elements)[pos].key = key;
    set(map, Map, size, ++get(map, Map, size));

    return value;
}

oop map_set(oop map, oop key, oop value)
{
    assert(is(Map, map));
    assert(key);
    assert(value);
    ssize_t pos = map_search(map, key);
    if (pos >= 0) {
        get(map, Map, elements)[pos].value = value;
    } else {
        pos = -1 - pos;
        map_insert(map, key, value, pos);
    }
    return value;
}

oop map_del(oop map, oop key)
{
    assert(is(Map, map));
    assert(is(String, key));
    ssize_t pos = map_search(map, key);
    if (pos < 0)    return map;
    if (pos < get(map, Map, size) - 1) {
        memmove(get(map, Map, elements) + pos, get(map, Map, elements) + pos + 1, sizeof(struct Pair) * (get(map, Map, size) - pos));
    }
    set(map, Map, size, --get(map, Map, size));
    return map;
}

oop map_append(oop map, oop value)
{
    return map_set(map, makeInteger(get(map, Map, size)), value);
}

bool variableIsDefined(oop aSymbol)
{
    assert(is(Symbol, aSymbol));
    return map_hasKey(globals, aSymbol);
}

oop variableSet(oop aSymbol, oop value)
{
    assert(is(Symbol, aSymbol));
    assert(value);
    map_set(globals, aSymbol, value);
    return value;
}

oop variableGet(oop aSymbol)
{
    assert(is(Symbol, aSymbol));
    return map_get(globals, aSymbol);
}

void print(oop ast)
{
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
        printf("%s", get(ast, Symbol, name));
        return;
    case Function:
        printf("Function@%p", get(ast, Function, primitive));
        return;
    case Map:
        printf("{");
        for (size_t i = 0; i < get(ast, Map, size); i++) {
            printf(" ");
            // I could write this instead but I want a special print for my string key name
            print(get(ast, Map, elements)[i].key);
            //printf("%s", get(get(ast, Map, elements)[i].key, String, value));
            printf(": ");
            print(get(ast, Map, elements)[i].value);
            if (i < get(ast, Map, size) - 1)    printf(",");
            else                                printf(" ");
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

oop intern(oop scope, char *ident)
{
    assert(is(Map, scope));
    oop symbol = makeSymbol(memcheck(strdup(ident)));
    ssize_t pos = map_search(scope, symbol);
    if (pos >= 0) return get(scope, Map, elements)[pos].key; // So it this case, symbol will be garbage collected right?
    pos = -1 - pos; // 'un-negate' the result by reflecting it around X=-1
    map_insert(scope, symbol, null, pos);
    return symbol;
}
 