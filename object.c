#include <stdbool.h>
#include <stdint.h>
#include <sysexits.h>
#include <assert.h>

#define USE_TAG  1

#define USE_GC   1

#if (USE_GC)
# include <gc.h>
#endif

void *xmalloc(size_t n)
{
#if (USE_GC)
    void *mem= GC_malloc(n);
#else
    void *mem= calloc(1, n);
#endif
    assert(mem);
    return mem;
}

void *xrealloc(void *p, size_t n)
{
#if (USE_GC)
    void *mem= GC_realloc(p, n);
#else
    void *mem= realloc(p, n);
#endif
    assert(mem);
    return mem;
}

char *xstrdup(char *s)
{
#if (USE_GC)
    size_t len= strlen(s);
    char  *mem= GC_malloc_atomic(len + 1);
    memcpy(mem, s, len + 1);
#else
    char *mem= strdup(s);
#endif
    assert(mem);
    return mem;
}

#define malloc(n)       xmalloc(n)
#define realloc(o, n)   xrealloc(o, n)
#define strdup(s)       xstrdup(s)

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
    int _value;
};

struct String {
    type_t type;
    char *value;
    size_t size;
};

struct Symbol {
    type_t type;
    char *name;
    #ifdef SYMBOL_PAYLOAD
    SYMBOL_PAYLOAD
    #endif //SYMBOL_PAYLOAD
};

typedef oop (*primitive_t)(oop params);


struct Function {
    type_t type;
    primitive_t primitive;
    oop body;
    oop param;
    oop parentScope;
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

int isInteger(oop obj)
{
#if (USE_TAG)
    return (intptr_t)obj & 1;
#else
    return is(Integer, obj);
#endif
}

#if (USE_TAG)
# define getType(PTR)	(((intptr_t)(PTR) & 1) ? Integer : (PTR)->type)
#else
type_t getType(oop ptr)
{
    assert(ptr);
    return ptr->type;
}
#endif

int is(type_t type, oop obj)
{
    return type == getType(obj);
}

oop _checkType(oop ptr, type_t type, char *file, int line)
{
    assert(ptr);
    if (getType(ptr) != type) {
        fprintf(stderr, "\n%s:%i: expected %i got %i\n", file, line, type, ptr->type);
    }
    assert(getType(ptr) == type);
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

void print(oop ast);
void println(oop ast);


int getInteger(oop obj)
{
#if (USE_TAG)
    return (intptr_t)obj >> 1;
#else
    return get(obj, Integer, _value);
#endif
}

oop makeInteger(int value)
{
#if (USE_TAG)
    return (oop) (((intptr_t)value << 1) | 1);
#else
    oop newInt = memcheck(malloc(sizeof(union object)));
    newInt->type = Integer;
    newInt->Integer._value = value;
    return newInt;
#endif
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
    newSymb->Symbol.name = memcheck(strdup(name));
    newSymb->Symbol.prototype = 0;
    return newSymb;
}

oop makeFunction(primitive_t primitive, oop param, oop body, oop parentScope)
{
    oop newFunc = memcheck(malloc(sizeof(union object)));
    newFunc->type = Function;
    newFunc->Function.primitive = primitive;
    newFunc->Function.param = param;
    newFunc->Function.body = body;
    newFunc->Function.parentScope = parentScope;
    return newFunc;
}

oop makeMap()
{
    oop newMap = memcheck(malloc(sizeof(union object)));
    newMap->type = Map;
    return newMap;
}

bool map_hasIntegerKey(oop map, size_t index)
{
    if (index >= get(map, Map, size)) return 0;
    oop key= get(map, Map, elements)[index].key;
    if (!isInteger(key)) return 0;
    return index == getInteger(key);
}

int oopcmp(oop a, oop b)
{
    type_t ta = getType(a), tb = getType(b);
    if (ta == tb) {
        switch (getType(a)) {
        case Integer:
            return getInteger(a) - getInteger(b);
        case String:
            return strcmp(get(a, String, value), get(b, String, value));
        default:
            if (a < b) return -1;
            if (a > b) return  1;
            return 0;
        }
    }
    return ta - tb;
}

ssize_t map_search(oop map, oop key)
{
    assert(is(Map, map));
    assert(key);

    ssize_t r = get(map, Map, size) - 1;

    if (isInteger(key)) {
        ssize_t index = getInteger(key);
        if (index > r) {
            return -1 - (r + 1);
        }
        oop probe = get(map, Map, elements)[index].key;
    if (key == probe) return index;
    }

    ssize_t l = 0;
    while (l <= r) {
        ssize_t mid = (l + r) / 2;
        int cmpres = oopcmp(get(map, Map, elements)[mid].key, key);
        if      (cmpres > 0)    r = mid - 1;
        else if (cmpres < 0)    l = mid + 1;
        else                    return mid; // non-negative result => element found at this index
    }
    return -1 - l; // negative result => 'not found', reflected around -1 instead of 0 to allow 'not found' at index 0
}

bool map_hasKey(oop map, oop key)
{
    // checks already done by map_search
    //assert(is(Map, map));
    //assert(key);
    return map_search(map, key) >= 0;
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
    set(map, Map, size, get(map, Map, size) + 1);

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
    if (pos < 0) return map;
    if (pos < get(map, Map, size) - 1) {
        memmove(get(map, Map, elements) + pos, get(map, Map, elements) + pos + 1, sizeof(struct Pair) * (get(map, Map, size) - pos));
    }
    set(map, Map, size, get(map, Map, size) - 1);
    return map;
}

oop map_append(oop map, oop value)
{
    return map_set(map, makeInteger(get(map, Map, size)), value);
}

void map_print(oop map, int ident)
{
    assert(is(Map, map));
    if (ident > 1) {
        printf("\n");
    }
    for (size_t i = 0; i < get(map, Map, size); i++) {
        for (size_t i = 0; i < ident; i++) {
            printf("|");
            printf("   ");
        }
        // todo: a key could be a map itself
        print(get(map, Map, elements)[i].key);
        printf(": ");
        oop rhs = get(map, Map, elements)[i].value;
        if (getType(rhs) == Map) {
            map_print(rhs, ident + 1);
        } else {
            print(rhs);
        }
        if (i < get(map, Map, size) - 1) printf(",\n");
    }
    return;
}

void print(oop ast)
{
    assert(ast);
    switch (getType(ast)) {
    case Undefined:
        printf("null");
        return;
    case Integer:
        printf("%i", getInteger(ast));
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
        /*
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
        */
        printf("{\n");
        map_print(ast, 1);
        printf("\n}");
        return;
    }
    assert(0);
}

void println(oop ast)
{
    print(ast);
    printf("\n");
}

oop symbol_table;

ssize_t map_intern_search(oop map, char* ident)
{
    assert(is(Map, map));
    assert(ident);
    ssize_t l = 0, r = get(map, Map, size) - 1;
    while (l <= r) {
        ssize_t mid = (l + r) / 2;
        int cmpres = strcmp(get(get(map, Map, elements)[mid].key, Symbol, name), ident);
        if (cmpres > 0)         r = mid - 1;
        else if (cmpres < 0)    l = mid + 1;
        else                    return mid; // non-negative result => element found at this index
    }
    return -1 - l; // negative result => 'not found', reflected around -1 instead of 0 to allow 'not found' at index 0
}

oop intern(char *ident)
{
    assert(ident);
    ssize_t pos = map_intern_search(symbol_table, ident);
    if (pos >= 0) return get(symbol_table, Map, elements)[pos].key;
    pos = -1 - pos; // 'un-negate' the result by reflecting it around X=-1
    oop symbol = makeSymbol(ident);
    map_insert(symbol_table, symbol, null, pos);
    return symbol;
}
