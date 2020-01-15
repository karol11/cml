#include <stdlib.h>
#include <string.h>
#include "cml_config.h"
#include "string_builder.h"

void sb_init(string_builder *b) {
	b->pos = b->start = (char*) malloc(16);
	b->end = b->start + 16;
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

void sb_grow(string_builder *b, size_t delta) {
	if (b->pos + delta >= b->end) {
		size_t old_size = b->pos - b->start;
		size_t new_size = old_size + delta;
		char *new_data = malloc(new_size);
		memcpy(new_data, b->start, old_size);
		free(b->start);
		b->pos = new_data + old_size;
		b->start = new_data;
		b->end = new_data + new_size;
	}
}

void sb_append(string_builder *b, char c) {
	*b->pos = c;
	if (++b->pos == b->end)
		sb_grow(b, 255);
}

void sb_puts(string_builder *b, const char *s) {
	size_t size = strlen(s);
	sb_grow(b, size + 1);
	memcpy(b->pos, s, size);
	b->pos += size;
}

size_t sb_size(string_builder *b) {
	return b->pos - b->start;
}
void sb_trunc(string_builder *b, size_t size) {
	if (size < (size_t)(b->pos - b->start))
		b->pos = b->start + size;
}