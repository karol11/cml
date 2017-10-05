#include <stdlib.h>
#include <stdio.h>
#include "tests.h"
#include "cml_stax_writer.h"
#include "utf8.h"

struct cml_stax_writer_tag {
	int (*putc)(void *context, char c);
	void *putc_context;
	int depth;
	int in_array;
	int in_error;
};

static int put_s(cml_stax_writer *w, const char *s) {
	if (w->in_error)
		return w->in_error = CMLW_WRITE_AFTER_ERROR;
	while (*s) {
		if (!w->putc(w->putc_context, *s++))
			return w->in_error = CMLW_PUTC_ERROR;
	}
	return 0;
}

static int write_head(cml_stax_writer *w, const char *field) {
	int i = w->depth + 1;
	if (w->in_error)
		return CMLW_WRITE_AFTER_ERROR;
	while (--i) {
		if (!w->putc(w->putc_context, '\t'))
			return w->in_error = CMLW_PUTC_ERROR;
	}
	if (w->in_array != !field)
		return w->in_error = CMLW_UNEXPECTED_FIELD;
	if (field) {
		if (put_s(w, field))
			return w->in_error = CMLW_PUTC_ERROR;
		if (!w->putc(w->putc_context, ' '))
			return w->in_error = CMLW_PUTC_ERROR;
	}
	return 0;
}

static int get_c(void *context) {
	return *(*(unsigned char**)context)++;
}

cml_stax_writer *cmlw_create(int (*putc)(void *context, char c), void *putc_context) {
	cml_stax_writer *r = (cml_stax_writer *) malloc(sizeof(cml_stax_writer));
	r->depth = 0;
	r->in_array = 1;
	r->in_error = 0;
	r->putc = putc;
	r->putc_context = putc_context;
	return r;
}

void cmlw_dispose(cml_stax_writer *w) {
	free(w);
}

int cmlw_int(cml_stax_writer *w, const char *field, long long value) {
	char buffer[21];
	if (write_head(w, field))
		return w->in_error;
	sprintf(buffer,"%lld", value);
	if (put_s(w, buffer))
		return w->in_error;
	return w->putc(w->putc_context, '\n') ? 0 : (w->in_error = CMLW_PUTC_ERROR);
}

int cmlw_str(cml_stax_writer *w, const char *field, const char *s) {
	int r = write_head(w, field);
	if (r < 0)
		return r;
	if (!w->putc(w->putc_context, '\"'))
		return w->in_error = CMLW_PUTC_ERROR;
	for (;;) {
		unsigned int c = get_utf8(get_c, (void*)&s);
		if (c == 0)
			break;
		if (c == '\\') {
			if (put_s(w, "\\\\"))
				return w->in_error;
		} else if (c == '"') {
			if (put_s(w, "\\\""))
				return w->in_error;
		} else if (c <= 0x1f || (c >= 0x7f && c <= 0x9f) || c == 0x2028 || c == 0x2029) {
			if (put_s(w, "\\u"))
				return w->in_error;
			{
				char buffer[21];
				sprintf(buffer,"%04x", c);
				if (put_s(w, buffer))
					return w->in_error;
			}
		}
		if (put_utf8(c, w->putc, w->putc_context))
			return w->in_error = CMLW_PUTC_ERROR;
	}
	return put_s(w, "\"\n");
}

int cmlw_array(cml_stax_writer *w, const char *field) {
	int r = w->in_array;
	if (write_head(w, field))
		return w->in_error;
	w->depth++;
	w->in_array = 1;
	return put_s(w, ":\n") ? w->in_error : r;
}

int cmlw_end_array(cml_stax_writer *w, int prev_state) {
	w->in_array = prev_state;
	return w->depth == 0 ? w->in_error = CMLW_UNPAIRED_END : (w->depth--, 0);
}

int cmlw_struct(cml_stax_writer *w, const char *field, const char *type, const char *id) {
	int r = w->in_array;
	if (write_head(w, field) || put_s(w, type))
		return w->in_error;
	if (id && (!w->putc(w->putc_context, '.') || put_s(w, id)))
		return w->in_error = CMLW_PUTC_ERROR;
	if (!w->in_array)
		w->depth++;
	w->in_array = 0;
	return w->putc(w->putc_context, '\n') ? r : (w->in_error = CMLW_PUTC_ERROR);
}

int cmlw_end_struct(cml_stax_writer *w, int prev_state) {
	w->in_array = prev_state;
	if (w->depth)
		w->depth--;
	else
		return w->in_error = CMLW_UNPAIRED_END;
	return !w->in_array || w->putc(w->putc_context, '\n') ? 0 : CMLW_PUTC_ERROR;
}

int cmlw_ref(cml_stax_writer *w, const char *field, const char *id) {
	if (write_head(w, field))
		return w->in_error;
	return
		w->putc(w->putc_context, '=') &&
		!put_s(w, id) &&
		w->putc(w->putc_context, '\n') ? 0 : (w->in_error = CMLW_PUTC_ERROR);
}
