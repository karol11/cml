#ifndef _CML_CONFIG_H_
#define _CML_CONFIG_H_

#define CONFIG_CML_FLOATINGPOINT

#ifdef _MSC_VER
#define cml_inline __forceinline
#elif defined(__GNUC__)
#define cml_inline __attribute__((always_inline)) inline
#else
#define cml_inline inline
#endif

#ifdef TESTS

#ifdef __cplusplus
extern "C" {
#endif


#define _STR(A) #A
#define STR(A) _STR(A)
#define ASSERT(A) if(!(A)) fail(__FILE__  " " STR(__LINE__));

#define malloc test_malloc
#define free test_free

void *test_malloc(int size);
void test_free(void *);
void fail(const char *s);

#ifdef __cplusplus
}
#endif

#endif

#endif
