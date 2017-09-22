#ifndef _STRING_BUILDER_H_
#define _STRING_BUILDER_H_

typedef struct string_builder_tag {
	char *start, *end, *pos;
} string_builder;

void sb_init(string_builder *b);
void sb_dispose(string_builder *b);
const char *sb_get_str(string_builder *b);
void sb_clear(string_builder *b);
void sb_append(char c, string_builder *b);

#endif
