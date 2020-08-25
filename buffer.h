#ifndef __buffer_h
#define __buffer_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define BUFFER_INITIALISER { 0, 0, 0 }

#define DECLARE_BUFFER(TYPE, NAME)								\
												\
typedef struct											\
{												\
    TYPE	*contents;									\
    size_t	 position;									\
    size_t	 capacity;									\
} NAME;												\
												\
extern inline NAME *new##NAME(NAME *b)								\
{												\
    return calloc(1, sizeof(NAME));								\
}												\
												\
extern inline NAME *NAME##_release(NAME *b)							\
{												\
    if (b->contents) free(b->contents);								\
    memset(b, 0, sizeof(NAME));									\
    return b;											\
}												\
												\
extern inline void NAME##_delete(NAME *b)							\
{												\
    NAME##_release(b);										\
    free(b);											\
}												\
												\
extern inline void NAME##_clear(NAME *b)							\
{												\
    b->position= 0;										\
}												\
												\
extern inline size_t NAME##_position(NAME *b)							\
{												\
    return b->position;										\
}												\
												\
extern inline void NAME##_errorBounds(NAME *b, ssize_t index)					\
{												\
    fprintf(stderr, "index %zi out of bounds for "#NAME" of size %zi\n", index, b->capacity);	\
    abort();											\
}												\
												\
extern inline void NAME##_errorMemory(NAME *b)							\
{												\
    fprintf(stderr, "out of memory typing to grow "#NAME" of size %zi\n", b->capacity);		\
    abort();											\
}												\
												\
extern inline TYPE NAME##_get(NAME *b, ssize_t index)						\
{												\
    if (index >= 0) {										\
	if (index < b->position) return b->contents[index];					\
    }												\
    else {											\
	if (b->position + index >= 0) return b->contents[b->position + index];			\
    }												\
    NAME##_errorBounds(b, index);								\
    abort();											\
    /* NOTREACHED */										\
}												\
												\
extern inline NAME *NAME##_grow(NAME *b, size_t size)						\
{												\
    if (0 == size) size= 2;									\
    if (b->capacity < size) {									\
	b->contents= b->contents								\
	    ? realloc(b->contents, sizeof(TYPE) * size)						\
	    : malloc (             sizeof(TYPE) * size);					\
	if (!b->contents) NAME##_errorMemory(b);						\
	memset(b->contents + b->capacity, 0, sizeof(TYPE) * (size - b->capacity));		\
	b->capacity= size;									\
    }												\
    return b;											\
}												\
												\
extern inline TYPE NAME##_append(NAME *b, TYPE value)						\
{												\
    if (b->position == b->capacity) NAME##_grow(b, b->capacity * 2);				\
    return b->contents[b->position++]= value;							\
}												\
												\
extern inline void NAME##_appendAll(NAME *b, const TYPE *s, size_t len)				\
{												\
    while (len--) NAME##_append(b, *s++);							\
}												\
												\
extern inline TYPE *NAME##_buffer(NAME *b)							\
{												\
    return b->contents;										\
}

#define DECLARE_STRING_BUFFER(TYPE, NAME)				\
									\
DECLARE_BUFFER(TYPE, NAME);						\
									\
extern inline TYPE *NAME##_appendString(NAME *b, TYPE *string)		\
{									\
    for (TYPE *ptr= string;  *ptr;  ++ptr) NAME##_append(b, *string++);	\
    return string;							\
}									\
									\
extern inline TYPE *NAME##_contents(NAME *b)				\
{									\
    NAME##_append(b, 0);						\
    b->position--;							\
    return b->contents;							\
}

#define buffer_do(T, V, B)								\
    for ( size_t index_of_##V= 0;							\
	  index_of_##V < (B)->position;							\
	  index_of_##V = (B)->position )						\
	for ( T V;									\
	      index_of_##V < (B)->position && (V= (B)->contents[index_of_##V], 1);	\
	      ++index_of_##V )

#endif // __buffer_h
