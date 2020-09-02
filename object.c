#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <sysexits.h>
#include <assert.h>

#define USE_TAG  1
#define USE_GC   1

#if (USE_GC)
# include <gc.h>
#endif

typedef long long int_t;

#define FMT_I "%lli"

void *memcheck(void *ptr)
{
    if (NULL == ptr) {
        fprintf(stderr, "Error: out of memory\n");
        exit(EX_OSERR); // this is as close as we have for 'resource unavailable'
    }
    return ptr;
}

unsigned long long nalloc= 0;

void *xmalloc(size_t n)
{
    nalloc += n;
#if (USE_GC)
    void *mem= GC_malloc(n);
    assert(mem);
#else
    void *mem= memcheck(calloc(1, n));
#endif
    return mem;
}

void *xrealloc(void *p, size_t n)
{
    nalloc += n;
#if (USE_GC)
    void *mem= GC_realloc(p, n);
    assert(mem);
#else
    void *mem= memcheck(realloc(p, n));
#endif
    return mem;
}

char *xstrdup(char *s)
{
#if (USE_GC)
    size_t len= strlen(s);
    char  *mem= GC_malloc_atomic(len + 1);
    assert(mem);
    memcpy(mem, s, len + 1);
    nalloc += len;
#else
    char *mem= memcheck(strdup(s));
#endif
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

#define NTYPES (Map + 1)

union object;
typedef union object *oop;

struct Undefined {
    type_t type;
};

struct Integer {
    type_t type;
    int_t _value;
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

typedef oop (*primitive_t)(oop scope, oop params);

struct Function {
    type_t type;
    primitive_t primitive;
    oop name;
    oop body;
    oop param;
    oop parentScope;
    oop fixed;
};

// usefull for map's elements
struct Pair {
    oop key;
    oop value;
};

enum {
    MAP_ENCLOSED = 1 << 0,    // set when map is used as a scope and closed over by a function
};

struct Map {
    type_t type;
    int    flags;
    struct Pair *elements; // even are keys, odd are values   [ key val key val key val ]
    size_t capacity;
    union {
	size_t size;       // free Maps will be reset to 0 size on allocation
	oop    pool;       // free list of Map objects
    };
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
const oop null = &_null;

int is(type_t type, oop obj);

#if (USE_TAG)
int isTag(oop obj)
{
    return ((intptr_t)obj & 1);
}
#endif

int isInteger(oop obj)
{
#if (USE_TAG)
    return ((intptr_t)obj & 1) || is(Integer, obj);
#else
    return is(Integer, obj);
#endif
}

#if (USE_TAG)
# define getType(PTR) (((intptr_t)(PTR) & 1) ? Integer : (PTR)->type)
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

#include "buffer.h"
DECLARE_STRING_BUFFER(char, StringBuffer);

void print(oop ast);
void println(oop ast);
void printOn(StringBuffer *buf, oop obj, int indent);

int_t getInteger(oop obj)
{
#if (USE_TAG)
    if (isTag(obj)) return (intptr_t)obj >> 1;
#endif
    if (!isInteger(obj)) {
       fprintf(stderr, "\nNon-integer in arithmetic expression\n");
        exit(1);
    }
    return get(obj, Integer, _value);
}

#if (USE_TAG)
int isIntegerValue(int_t value)
{
    return (((intptr_t)value << 1) >> 1) == value;
//  return -32 <= value && value < 32;
}
#endif

oop makeInteger(int_t value)
{
#if (USE_TAG)
    if (isIntegerValue(value)) return (oop)(((intptr_t)value << 1) | 1);
#endif
    oop newInt = malloc(sizeof(struct Integer));
    newInt->type = Integer;
    newInt->Integer._value = value;
    return newInt;
}

oop makeString(char *value)
{
    oop newString = malloc(sizeof(struct String));
    newString->type = String;
    newString->String.value = strdup(value);
    newString->String.size = strlen(value);
    return newString;
}

size_t string_size(oop s)
{
    return get(s, String, size);
}

oop string_concat(oop str1, oop str2)
{
    size_t len = string_size(str1) + string_size(str2);
    char *concat = malloc(sizeof(char) * len + 1);
    memcpy(concat, get(str1, String, value), string_size(str1));
    memcpy(concat + string_size(str1), get(str2, String, value), string_size(str2));
    concat[len]= '\0';
    oop newString = malloc(sizeof(struct String));
    newString->type = String;
    newString->String.value = concat;
    newString->String.size = len;
    return newString;
}

oop string_mul(oop str, oop factor)
{
    ssize_t len = string_size(str) * getInteger(factor);
    if (len < 0) len = 0;
    char *concat = malloc(sizeof(char) * len + 1);
    for (int i=0; i < getInteger(factor); ++i) {
        memcpy(concat + (i * string_size(str)), get(str, String, value), string_size(str));
    }
    concat[len]= '\0';
    oop newString = malloc(sizeof(struct String));
    newString->type = String;
    newString->String.value = concat;
    newString->String.size = len;
    return newString;
}

oop makeSymbol(char *name)
{
    oop newSymb = malloc(sizeof(struct Symbol));
    newSymb->type = Symbol;
    newSymb->Symbol.name = strdup(name);
    newSymb->Symbol.prototype = 0;
    return newSymb;
}

oop makeFunction(primitive_t primitive, oop name, oop param, oop body, oop parentScope, oop fixed)
{
    oop newFunc = malloc(sizeof(struct Function));
    newFunc->type = Function;
    newFunc->Function.primitive = primitive;
    newFunc->Function.name = name;
    newFunc->Function.param = param;
    newFunc->Function.body = body;
    newFunc->Function.parentScope = parentScope;
    newFunc->Function.fixed = fixed;
    return newFunc;
}

oop makeMap()
{
    oop newMap = malloc(sizeof(struct Map));			assert(0 == newMap->Map.flags);
    newMap->type = Map;
    return newMap;
}

size_t map_size(oop map)
{
    assert(is(Map, map));
    return get(map, Map, size);
}

bool map_hasIntegerKey(oop map, size_t index)
{
    if (index >= map_size(map)) return 0;
    oop key= get(map, Map, elements)[index].key;
    if (!isInteger(key)) return 0;
    return index == getInteger(key);
}

int oopcmp(oop a, oop b)
{
    type_t ta = getType(a), tb = getType(b);
    if (ta == tb) {
        switch (getType(a)) {
	    case Integer: {
		int l= getInteger(a), r= getInteger(b);
		if (l < r) return -1;
		if (l > r) return  1;
		return 0;
	    }
	    case String:
		return strcmp(get(a, String, value), get(b, String, value));
	    default: {
		intptr_t l= (intptr_t)a, r= (intptr_t)b;
		if (l < r) return -1;
		if (l > r) return  1;
		return 0;
	    }
        }
    }
    return ta - tb;
}

ssize_t map_search(oop map, oop key)
{
    assert(is(Map, map));
    assert(key);

    ssize_t r = map_size(map) - 1;

    if (isInteger(key)) {
        ssize_t index = getInteger(key);
        if (index <= r) {
            oop probe = get(map, Map, elements)[index].key;
            if (key == probe) return index;
        }
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
    assert(is(Map, map));
    assert(key);
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

#define MAP_MIN_SIZE  4
#define MAP_GROW_SIZE 2

oop map_insert(oop map, oop key, oop value, size_t pos)
{
    assert(is(Map, map));
    assert(key);
    assert(value);
    if (pos > map_size(map)) { // don't need to check for pos < 0 because size_t is unsigned
        fprintf(stderr, "\nTrying to insert in a map out of bound\n");
        assert(-1);
    }

    // check capacity and expand if needed
    if (map_size(map) >= get(map, Map, capacity)) {
        size_t newCapacity = get(map, Map, capacity) * MAP_GROW_SIZE;
	if (newCapacity < MAP_MIN_SIZE) newCapacity= MAP_MIN_SIZE;
        set(map, Map, elements, realloc(get(map, Map, elements), sizeof(struct Pair) * newCapacity));
        set(map, Map, capacity, newCapacity);
    }

    // insert
    memmove(get(map, Map, elements) + pos + 1, get(map, Map, elements) + pos, sizeof(struct Pair) * (map_size(map) - pos));
    // Maybe this syntax is not very nice and I should access the Pair stuff differently?
    // I mean modifying something on a line that begin with "get"... :/
    get(map, Map, elements)[pos].value = value;
    get(map, Map, elements)[pos].key = key;
    set(map, Map, size, map_size(map) + 1);

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
    if (pos < map_size(map) - 1) {
        memmove(get(map, Map, elements) + pos, get(map, Map, elements) + pos + 1, sizeof(struct Pair) * (map_size(map) - pos));
    }
    set(map, Map, size, map_size(map) - 1);
    return map;
}

oop map_append(oop map, oop value)
{
    return map_set(map, makeInteger(map_size(map)), value);
}

bool isHidden(oop obj) {
    if (is(Symbol, obj)) {
        char *s = get(obj, Symbol, name);
        size_t l = strlen(s);
        // maybe 'l > 5' because of ____?
        return (l > 4 && s[0] == '_' && s[1] == '_' && s[l-2] == '_' && s[l-1] == '_');
    }
    return false;
}

oop map_keys(oop map)
{
    assert(is(Map, map));
    oop keys = makeMap();
    for (size_t i = 0; i < get(map, Map, size); i++) {
        if (!isHidden(get(map, Map, elements)[i].key)) {
            map_append(keys, get(map, Map, elements)[i].key);
        }
    }
    return keys;
}

oop map_allKeys(oop map)
{
    assert(is(Map, map));
    oop keys = makeMap();
    for (size_t i = 0; i < get(map, Map, size); i++) {
        map_append(keys, get(map, Map, elements)[i].key);
    }
    return keys;
}

oop map_values(oop map)
{
    assert(is(Map, map));
    oop values = makeMap();
    for (size_t i = 0; i < get(map, Map, size); i++) {
        if (!isHidden(get(map, Map, elements)[i].key)) {
            map_append(values, get(map, Map, elements)[i].value);
        }
    }
    return values;
}

DECLARE_BUFFER(oop, OopStack);
OopStack printing = BUFFER_INITIALISER;

#define OopStack_push(s, o) OopStack_append(s, o)
oop OopStack_pop(OopStack *s)
{
    if (s->position < 1) {
        return null;
    }
    return s->contents[--(s->position)];
}

int OopStack_includes(OopStack *s, oop map)
{
    for (size_t i=0;  i < s->position;  ++i) {
        if (s->contents[i] == map) {
            return 1;
        }
    }
    return 0;
}

void indentOn(StringBuffer *buf, int indent)
{
    for (size_t i = 0; i < indent; i++) {
        if (isatty(fileno(stdout))) {
            StringBuffer_appendString(buf, "\033[90m|\033[0m");
        } else {
            StringBuffer_appendString(buf, "|");
        }
        StringBuffer_appendString(buf, "   ");
    }
}

void map_printOn(StringBuffer *buf, oop map, int ident)
{
    assert(is(Map, map));
    if (ident == 0) {
        StringBuffer_append(buf, '{');
        map_printOn(buf, map, ident + 1);
        StringBuffer_append(buf, '}');
        return;
    }
    if (OopStack_includes(&printing, map)) {
        StringBuffer_appendString(buf, "<cycle>");
        return;
    }
    OopStack_push(&printing, map);
    for (size_t i = 0; i < map_size(map); i++) {
        StringBuffer_append(buf, '\n');
        indentOn(buf, ident);
        // todo: a key could be a map itself
        printOn(buf, get(map, Map, elements)[i].key, ident);
        StringBuffer_appendString(buf, ": ");
        oop rhs = get(map, Map, elements)[i].value;
        if (getType(rhs) == Map) {
            map_printOn(buf, rhs, ident + 1);
        } else {
            printOn(buf, rhs, ident);
        }
        if (i < map_size(map) - 1) StringBuffer_append(buf, ',');
        if (ident == 1 && i == map_size(map) - 1) StringBuffer_append(buf, '\n');
    }
    OopStack_pop(&printing);
}

void printOn(StringBuffer *buf, oop obj, int indent)
{
    assert(obj);
    switch (getType(obj)) {
        case Undefined: {
            StringBuffer_appendString(buf, "null");
            return;
        }
        case Integer: {
            char tmp[40];
            int length = snprintf(tmp, sizeof(tmp), FMT_I, getInteger(obj));
            StringBuffer_appendAll(buf, tmp, length);
            return;
        }
        case String: {
            StringBuffer_appendAll(buf, get(obj, String, value), string_size(obj));
            return;
        }
        case Symbol: {
            char *name= get(obj, Symbol, name);
            StringBuffer_appendString(buf, name);
            return;
        }
        case Function: {
            if (get(obj, Function, primitive) == NULL) {
                StringBuffer_appendString(buf, "Function:");
            } else {
                StringBuffer_appendString(buf, "Primitive:");
            }
            printOn(buf, get(obj, Function, name), indent);
            StringBuffer_append(buf, '(');
            printOn(buf, get(obj, Function, param), indent + 1);
            if (is(Map, get(obj, Function, param)) && map_size(get(obj, Function, param)) > 0) {
                StringBuffer_append(buf, '\n');
                indentOn(buf, indent);
            }
            StringBuffer_append(buf, ')');
            return;
        }
        case Map: {
            map_printOn(buf, obj, indent);
            return;
        }
    }
    assert(0);
}

char *printString(oop obj)
{
    static StringBuffer buf= BUFFER_INITIALISER;
    StringBuffer_clear(&buf);
    printOn(&buf, obj, 0);
    return StringBuffer_contents(&buf);
}

void print(oop obj)
{
    fputs(printString(obj), stdout);
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
    ssize_t l = 0, r = map_size(map) - 1;
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
