#ifndef _CML_DOM_WRITER_H_
#define _CML_DOM_WRITER_H_

typedef struct d_dom_tag d_dom;

int cml_write(
	d_dom *dom,
	int (*putc)(char c, void *context),
	void *putc_context);

#endif
