#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string_builder_test.h"
#include "dom_test.h"
#include "cml_stax_reader_test.h"
#include "cml_dom_reader_test.h"
#include "utf8_test.h"

#undef malloc
#undef free

#define MALLOCS_MAX 1024
int *malloc_ids[MALLOCS_MAX+1];
int malloc_numerator = 0;
int allocs_cnt = 0;

void *test_malloc(int size) {
	int *r = (int*) malloc(size + sizeof(int));
	*r = malloc_numerator;
	malloc_ids[malloc_numerator] = r;
	if (malloc_numerator++ == MALLOCS_MAX)
		fail("alloc overflow");
	allocs_cnt++;
	return r + 1;
}

void test_free(void *p) {
	int *r = (int*) p;
	if (r[-1] >= malloc_numerator || malloc_ids[r[-1]] == 0)
		fail("double disposal");
	malloc_ids[r[-1]] = 0;
	allocs_cnt--;
	free(r - 1);
}

void fail(const char *s)
{
	printf("error %s", s);
	exit(-1);
}

void perform_test(void (*test)(), const char *name) {
	printf("test: %s ", name);
	test();
	printf(" ok\n");
	if (allocs_cnt) {
		int i = malloc_numerator;
		while (--i >= 0) {
			if (malloc_ids[i])
				printf("leaked allocation#%d\n", *malloc_ids[i]);
		}
		printf("%d leaks\n", allocs_cnt);
		fail("leaks!");
	}
}

int main() {
	perform_test(string_builder_test, "string builder");
	perform_test(utf8_test, "utf8");
	perform_test(dom_test, "dom");
	perform_test(cml_stax_reader_test, "cml_stax_reader");
	perform_test(cml_dom_reader_test, "cml_dom_reader");
	printf("done\n");
	return 0;
}
