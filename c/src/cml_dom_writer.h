#ifndef _CML_DOM_WRITER_H_
#define _CML_DOM_WRITER_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct d_dom_tag d_dom;

int cml_write(
	d_dom *dom,
	int (*putc)(void *context, char c),
	void *putc_context);

#ifdef __cplusplus
}
#endif

#endif
