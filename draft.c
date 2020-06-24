#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#define SYMBOL_TABLE_CHUNK 4

typedef struct symbol_t {
	char	*ident;
	bool	defined;
	int		value;
} symbol_t;

typedef struct table_t {
	symbol_t	**array;
	int			size;
	int			capacity;
} table_t;

table_t table;

typedef struct bsearch_t {
	int		pos;
	bool	found;
} bsearch_t;

bsearch_t binary_search(symbol_t *arr[], int l, int r, char *ident)
{
	while (r >= l) {
		int mid = l + (r - l) / 2;
		int cmpres = strcmp(arr[mid]->ident, ident);
		if (cmpres > 0) {
			r = mid - 1;
		} else if (cmpres < 0) {
			l = mid + 1;
		} else {
			bsearch_t res = { mid, true };
			return res;
		}
	} 
	bsearch_t res = { l, false };
	return res;
}

int insert(table_t *table, symbol_t *element, int pos)
{
	if (pos < 0 || pos > table->size) {
		return -1;
	}

	if (table->size  >= table->capacity) {
		table->array = realloc(table->array, sizeof(symbol_t *) * (table->capacity + SYMBOL_TABLE_CHUNK));
		if (table->array == NULL) {
			printf("Error: running out of memory\n");
			exit(1);
		}
		table->capacity += SYMBOL_TABLE_CHUNK;
	}

	for (int i = table->size; i > pos; i--) {
		table->array[i] = table->array[i-1];
	}
	table->array[pos] = element;
	return ++(table->size);
}

symbol_t *intern(char *ident, bool create)
{
	bsearch_t res = binary_search(table.array, 0, table.size - 1, ident);
	printf("pos:%d\n", res.pos);
	if (res.found) {
		return table.array[res.pos];
	}

	if (create) {
		symbol_t *new_symbol = malloc(sizeof(symbol_t));
		new_symbol->ident = strdup(ident);
		new_symbol->defined = false;
		printf("insert:%d\n", insert(&table, new_symbol, res.pos));
		return new_symbol;
	} else {
		return NULL;
	}
}

void init_table()
{
	table.array = malloc(sizeof(symbol_t *) * SYMBOL_TABLE_CHUNK);
	if (table.array == NULL) {
			printf("Error: running out of memory\n");
			exit(1);
	}
	table.size = 0;
	table.capacity = SYMBOL_TABLE_CHUNK;
}

symbol_t *update_value(symbol_t * s, int value)
{
	s->value = value;
	s->defined = true;
	return s;
}

int main() 
{
	init_table();
	char line[256];
	intern("chaussure\n", true);
	while (fgets(line, sizeof(line), stdin)) {
		printf("intern:%p\n", intern(line, true));
		printf("after size:%d\n", table.size);
		printf("after capacity:%d\n", table.capacity);
		printf("\n");
		for (int i = 0; i < table.size; i++) {
			printf("%d.%s", i, table.array[i]->ident);
		}
		printf("\n");
	}	
}