#ifndef _CML_DOM_READER_H_
#define _CML_DOM_READER_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct d_dom_tag d_dom;

d_dom *cml_read(
	int (*getc)(void *context),
	void *getc_context,
	void (*on_error)(void *context, const char *error, int line_num, int char_pos),
	void *on_error_context);

#ifdef __cplusplus
}
#endif

#endif
