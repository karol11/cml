#ifndef _STRING_BUILDER_H_
#define _STRING_BUILDER_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct string_builder_tag {
	char *start, *end, *pos;
} string_builder;

void sb_init(string_builder *b);
void sb_dispose(string_builder *b);
const char *sb_get_str(string_builder *b);
void sb_clear(string_builder *b);
void sb_append(string_builder *b, char c);
void sb_puts(string_builder *b, const char *s);
void sb_grow(string_builder *b, size_t delta);

#ifdef __cplusplus
}
#endif

#endif
