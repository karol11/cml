#ifndef _STRING_BUILDER_TEST_H_
#define _STRING_BUILDER_TEST_H_

#include "../src/string_builder.h"
#include "../src/tests.h"

const char test_text[] = "hello world!\nThis string constant is so long that it probably won't fit in initial 16-bytes buffer!";
const char test_text1[] = "smaller";

void string_builder_test() {
	const char *s;
	string_builder a;
	sb_init(&a);
	for (s = test_text; *s; s++)
		sb_append(&a, *s);
	ASSERT(strcmp(sb_get_str(&a), test_text) == 0);

	sb_clear(&a);
	for (s = test_text1; *s; s++)
		sb_append(&a, *s);
	ASSERT(strcmp(sb_get_str(&a), test_text1) == 0);

	sb_dispose(&a);
}

#endif
