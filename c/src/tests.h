#ifndef _TESTS_H_
#define _TESTS_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef TESTS

#define STR(A) #A
#define ASSERT(A) if(!(A)) fail(STR(A));

#define malloc test_malloc
#define free test_free

void *test_malloc(int size);
void test_free(void *);
void fail(const char *s);

#endif

#ifdef __cplusplus
}
#endif

#endif
