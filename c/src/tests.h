#ifndef _TESTS_H_
#define _TESTS_H_

#ifdef TESTS

#define STR(A) #A
#define ASSERT(A) if(!(A)) fail(STR(A));

#define malloc test_malloc
#define free test_free

void *test_malloc(size_t size);
void test_free(void *);
void fail(const char *s);

#endif

#endif
