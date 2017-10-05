#ifndef _UTF8_TEST_H_
#define _UTF8_TEST_H_

#include "../src/tests.h"
#include "../src/utf8.h"

static int get_c(void *context) {
	return *(*(unsigned char**)context)++;
}
static int put_c(void *context, char v) {
	*(*(char**)context)++ = v;
	return 1;
}

static void test_both_ways(int a, char *b) {
	char buffer[20];
	char *p = buffer;
	char *pp = buffer;
	ASSERT(put_utf8(a, put_c, &p) > 0);
	*p++ = 0;
	ASSERT(memcmp(buffer, b, p - buffer) == 0);
	ASSERT(get_utf8(get_c, &pp) == a);
	ASSERT(pp + 1  == p);
}

void utf8_test()
{
	test_both_ways(0x24, "\x24");
	test_both_ways(0xa2, "\xc2\xa2");
	test_both_ways(0x20AC, "\xe2\x82\xac");
	test_both_ways(0x10348, "\xf0\x90\x8d\x88");
}

#endif //TESTS
