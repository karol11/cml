#include <stdlib.h>
#include <stdio.h>
#include "cml_stax_writer.h"

struct cml_stax_writer_tag {
	int (*putc)(char c, void *context);
	void *putc_context;
	int depth;
	int in_array;
};

static int put_s(cml_stax_writer *w, const char *s) {
	while (*s) {
		if (!w->putc(*s++, w->putc_context))
			return 0;
	}
	return 1;
}

static int write_head(cml_stax_writer *w, const char *field) {
	int i = w->depth + 1;
	while (--i) {
		if (!w->putc('\t', w->putc_context))
			return CMLW_PUTC_ERROR;
	}
	if (w->in_array != !field)
		return CMLW_UNEXPECTED_FIELD;
	if (field) {
		if (!put_s(w, field))
			return CMLW_PUTC_ERROR;
		if (!w->putc(' ', w->putc_context))
			return CMLW_PUTC_ERROR;
	}
	return 0;
}

cml_stax_writer *cml_create_writer(int (*putc)(char c, void *context), void *putc_context) {
	cml_stax_writer *r = (cml_stax_writer *) malloc(sizeof(cml_stax_writer));
	r->depth = 0;
	r->in_array = 1;
	r->putc = putc;
	r->putc_context = putc_context;
	return r;
}

void cml_dispose_writer(cml_stax_writer *w) {
	free(w);
}

int cml_write_int(cml_stax_writer *w, const char *field, long long value) {
	char buffer[21];
	int r = write_head(w, field);
	if (r < 0)
		return r;
	sprintf(buffer,"%ld", value);
	if (!put_s(w, buffer))
		return CMLW_PUTC_ERROR;
	return w->putc('\n', w->putc_context) ? 0 : CMLW_PUTC_ERROR;
}

int cml_write_str(cml_stax_writer *w, const char *field, const char *s) {
	int r = write_head(w, field);
	if (r < 0)
		return r;
	if (!w->putc('\"', w->putc_context))
		return CMLW_PUTC_ERROR;
	while (*s) {
		if (*s && !w->putc('\"', w->putc_context))
			return CMLW_PUTC_ERROR;
		if (!w->putc(*s++, w->putc_context))
			return CMLW_PUTC_ERROR;
	}
	return put_s(w, "\"\n") ? 0 : CMLW_PUTC_ERROR;
}

int cml_write_array(cml_stax_writer *w, const char *field) {
	int r = write_head(w, field);
	if (r < 0)
		return r;
	r = w->in_array;
	w->depth++;
	w->in_array = 1;
	return put_s(w, ":\n") ? r : CMLW_PUTC_ERROR;
}

int cml_write_end_array(cml_stax_writer *w, int prev_state) {
	w->in_array = prev_state;
	w->depth--;
	return 0;
}

int cml_write_struct(cml_stax_writer *w, const char *field, const char *type, const char *id) {
	int r = write_head(w, field);
	if (r < 0)
		return r;
	if (!put_s(w, type))
		return CMLW_PUTC_ERROR;
	if (id && (!w->putc('.', w->putc_context) || !put_s(w, id)))
		return CMLW_PUTC_ERROR;
	r = w->in_array;
	if (!w->in_array)
		w->depth++;
	w->in_array = 0;
	return w->putc('\n', w->putc_context) ? r : CMLW_PUTC_ERROR;
}

int cml_write_end_struct(cml_stax_writer *w, int prev_state) {
	w->in_array = prev_state;
	w->depth--;
	return !w->in_array || w->putc('\n', w->putc_context) ? 0 : CMLW_PUTC_ERROR;
}

int cml_write_ref(cml_stax_writer *w, const char *field, const char *id) {
	int r = write_head(w, field);
	if (r < 0)
		return r;
	return
		w->putc('=', w->putc_context) &&
		put_s(w, id) &&
		w->putc('\n', w->putc_context) ? 0 : CMLW_PUTC_ERROR;
}
