#include <stdlib.h>
#include <string.h>
#include "string_builder.h"

void sb_init(string_builder *b) {
	b->pos = b->start = (char*) malloc(5);
	b->end = b->start + 5;
}
void sb_dispose(string_builder *b) {
	free(b->start);
}
const char *sb_get_str(string_builder *b) {
	*b->pos = 0;
	return (const char*) b->start;
}
void sb_clear(string_builder *b) {
	b->pos = b->start;
}
void sb_append(char c, string_builder *b) {
	*b->pos = c;
	if (++b->pos == b->end) {
		int old_size = b->end - b->start;
		int new_size = old_size + 256;
		char *new_data = malloc(new_size);
		memcpy(new_data, b->start, old_size);
		free(b->start);
		b->pos = new_data + old_size;
		b->start = new_data;
		b->end = new_data + new_size;
	}
}
