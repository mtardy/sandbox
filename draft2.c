#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sysexits.h>

#define SYMBOL_TABLE_CHUNK 4

typedef struct symbol_t {
	char	*ident;
	bool	 defined;
	int	 value;
} symbol_t;

typedef struct table_t {
	symbol_t	**array;
	size_t		  size;       // size_t allows your table to grow past the 1G element limit of a 32-bit number on 64-bit machines
	size_t		  capacity;
} table_t;

// Make a static initialiser for tables so that they never have to be initialised explicitly.
// The first call to table_insert() will notice size == capacity (0 == 0) and grow the table.
// The realloc() function is officially documented to work with a NULL pointer, in which case it bahves like malloc().
// This makes tables 'self-initialising' and therefore much nicer to work with inside libraries and other 'opaque' places.
// I like to make all my data structures be self-initialising.

#define TABLE_INITIALISER { NULL, 0, 0 } // first call to table_insert() will initialise storage

table_t table = TABLE_INITIALISER;       // safe but not strictly needed on Unix because BSS segment is initialised to all zeroes

// this test happens often so let's abstract it
// (in a large program it would be better to write xcalloc, xrealloc, xstrdup, etc., that perform the test implicitly)

void *memcheck(void *ptr)
{
    if (NULL == ptr) {
	fprintf(stderr, "Error: out of memory\n");
	exit(EX_OSERR);  // this is as close as we have for 'resource unavailable'
    }
    return ptr;
}

// Pass 'table *' as first parameter to make it consistent with other table functions.
// (Being able to think about the 'table_*' functions as 'methods' that operate on tables is a nice API design.
// Of course, the disadvantage is that this function is now less general: it can no longer be used to search any array of 'symbol *'s.
// On the other hand, 'table_t' and 'symbol_t' seem to be part of the same facility, so making this search a 'table *' is maybe OK?)

// Renamed from binary_search to indicate that the first argument is a 'table *' to make it consistent with all the other table 'methods'.

// Return value changed to ssize_t because (1) 64-bit memory size and (2) let's go back to using -ve integers to represent 'not found'!

ssize_t table_search(table_t *table, char *ident)
{
	ssize_t l= 0, r= table->size - 1;      // no longer needed as parameters if we pass a table as the first parameter
	while (l <= r) {                       // swapped the order of l and r because I always visualise data as laid out from left-to-right ;-)
		ssize_t mid= (l + r) / 2;
		int cmpres= strcmp(table->array[mid]->ident, ident);
		if      (cmpres > 0) 	r= mid - 1;
		else if (cmpres < 0) 	l= mid + 1;
		else 			return mid;  // non-negative result => element found at this index
	}
	// As you pointed out: 0 == -0 so just negating the desired index is not possible.
	// Think of negation as 'reflecting the number around the point X=0', in other words negation of x is '0 - x'.
	// To allow 0 to make a negative result, just reflect around -1 instead of 0, so negation of x is '-1 - x'.
	// This is actually the one's complement of l, but using '~l' here might be obscure (and the compiler will figure it out anyway).
	return -1 - l;  // negative result => 'not found', reflected around -1 instead of 0 to allow 'not found' at index 0
}

// ssize_t result because -1 means 'error'

ssize_t table_insert(table_t *table, symbol_t *element, size_t pos)
{
	if (pos > table->size) {  // don't need to check for pos < 0 because size_t is unsigned
		return -1;
	}

	if (table->size  >= table->capacity) {
		// on the first call table->array will be NULL and realloc() will behave like malloc()
		table->array = memcheck(realloc(table->array, sizeof(symbol_t *) * (table->capacity + SYMBOL_TABLE_CHUNK)));
		table->capacity += SYMBOL_TABLE_CHUNK;
	}

	// use memmove() instead of a loop because memmove() is aggressively optimised on most platforms
	// remove a non-local dependency: use sizeof(*pointer-to-element) in case the type of elements changes in the declaration of table_t
        memmove(table->array + pos + 1, table->array + pos, sizeof(*table->array) * (table->size - pos));
	// for (int i = table->size; i > pos; i--) table->array[i] = table->array[i-1];
	table->array[pos] = element;
	return ++(table->size);
}

symbol_t *intern(char *ident, bool create)
{
	ssize_t res= table_search(&table, ident);  // < 0 => not found
	printf("pos:%zi\n", res);
	if (res >= 0) return table.array[res];
	if (!create)  return NULL;
	res= -1 - res; // 'un-negate' the resulr by reflecting it around X=-1
	symbol_t *new_symbol = memcheck(calloc(1, sizeof(symbol_t))); // calloc() will init all content to 0 (including .value member)
	new_symbol->ident = memcheck(strdup(ident));  // check for out-of-memory
	new_symbol->defined = false;                  // implicit in calloc(), but safer to do it explicitly anyway
	printf("insert:%zi\n", table_insert(&table, new_symbol, res));
	return new_symbol;
}

// no longer needed because tables are now self-initialising

// void init_table()
// {
//     table.array = malloc(sizeof(symbol_t *) * SYMBOL_TABLE_CHUNK);
//     if (table.array == NULL) {
//         printf("Error: running out of memory\n");
//         exit(1);
//     }
//     table.size = 0;
//     table.capacity = SYMBOL_TABLE_CHUNK;
// }

symbol_t *update_value(symbol_t * s, int value)
{
	s->value = value;
	s->defined = true;
	return s;
}

int main()
{
	// init_table();    	    // not needed: tables are self-initialising
	char *line= 0;      	    // this and
	size_t linecap= 0;  	    // this are needed for getline()
	intern("chaussure", true);  // identifiers will have no trailing newline so let's test with no trailing newline
	for (;;) {                  // using an infinite loop simplifies the break/continue logic in the body
		ssize_t len= getline(&line, &linecap, stdin);              // use getline() to auto-grow the buffer when necessary
		if (len < 0) break;                                        // stop at EOF
		while ((len > 0) && ('\n' == line[len-1])) line[--len]= 0; // trim newlines from the end
		if (len < 1) continue;                                     // ignore empty lines
		printf("intern : %p\n", intern(line, true));
		printf("after size : %zi\n", table.size);
		printf("after capacity : %zi\n", table.capacity);
		printf("\n");
		for (int i = 0; i < table.size; i++) {
			printf("%i %s\n", i, table.array[i]->ident);
		}
		printf("\n");
	}
}
