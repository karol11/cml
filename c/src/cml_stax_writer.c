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
	int blank_line_needed;
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

static int put_c(cml_stax_writer *w, char c) {
	return
		w->in_error ? w->in_error = CMLW_WRITE_AFTER_ERROR :
		w->putc(w->putc_context, c) ? 0 : (w->in_error = CMLW_PUTC_ERROR);
}

static int write_indent(cml_stax_writer *w, int i) {
	if (w->in_error)
		return w->in_error = CMLW_WRITE_AFTER_ERROR;
	while (--i) {
		if (!w->putc(w->putc_context, '\t'))
			return w->in_error = CMLW_PUTC_ERROR;
	}
	return 0;
}

static int write_head(cml_stax_writer *w, const char *field) {
	if (w->blank_line_needed && put_c(w, '\n'))
		return w->in_error;
	w->blank_line_needed = 0;
	if (write_indent(w, w->depth + 1))
		return w->in_error;
	if (w->in_array != !field)
		return w->in_error = CMLW_UNEXPECTED_FIELD;
	if (field) {
		if (put_s(w, field) || put_c(w, ' '))
			return w->in_error;
	}
	return 0;
}

static int get_c(void *context) {
	return *(*(unsigned char**)context)++;
}

cml_stax_writer *cmlw_create(int (*putc)(void *context, char c), void *putc_context) {
	cml_stax_writer *w = (cml_stax_writer *) malloc(sizeof(cml_stax_writer));
	w->depth = 0;
	w->in_array = 1;
	w->in_error = 0;
	w->blank_line_needed = 0;
	w->putc = putc;
	w->putc_context = putc_context;
	return w;
}

void cmlw_dispose(cml_stax_writer *w) {
	free(w);
}

int cmlw_int(cml_stax_writer *w, const char *field, long long value) {
	char buffer[21];
	sprintf(buffer,"%lld\n", value);
	if (!write_head(w, field))
		put_s(w, buffer);
	return w->in_error;
}

int cmlw_bool(cml_stax_writer *w, const char *field, int value) {
	if (!write_head(w, field))
		put_s(w, value ? "+\n" : "-\n");
	return w->in_error;
}

int cmlw_str(cml_stax_writer *w, const char *field, const char *s) {
	int r = write_head(w, field);
	if (r)
		return r;
	if (put_c(w, '\"'))
		return w->in_error;
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
			char buffer[7];
			sprintf(buffer,"\\u%04x", c);
			if (put_s(w, buffer))
				return w->in_error;
		} else if (!put_utf8(c, w->putc, w->putc_context))
			return w->in_error = CMLW_PUTC_ERROR;
	}
	return put_s(w, "\"\n");
}

int cmlw_array(cml_stax_writer *w, const char *field, int size) {
	char buffer[21];
	int r = w->in_array;
	if (write_head(w, field))
		return w->in_error;
	w->depth++;
	w->in_array = 1;
	put_s(w, size >= 0 ? sprintf(buffer,":%d\n", size), buffer : ":\n");
	return w->in_error ? w->in_error : r;
}

int cmlw_end_array(cml_stax_writer *w, int prev_state) {
	w->in_array = prev_state;
	w->blank_line_needed = 0;
	return w->depth == 0 ? w->in_error = CMLW_UNPAIRED_END : (w->depth--, 0);
}

int cmlw_struct(cml_stax_writer *w, const char *field, const char *type, const char *id) {
	int r = w->in_array;
	if (write_head(w, field) || put_s(w, type))
		return w->in_error;
	if (id && (put_c(w, '.') || put_s(w, id)))
		return w->in_error;
	if (!w->in_array)
		w->depth++;
	w->in_array = 0;
	put_c(w, '\n');
	return w->in_error ? w->in_error : r;
}

int cmlw_end_struct(cml_stax_writer *w, int prev_state) {
	w->in_array = prev_state;
	if (w->in_array)
		w->blank_line_needed = 1;
	else if (w->depth)
		w->depth--;
	else
		return w->in_error = CMLW_UNPAIRED_END;
	return 0;
}

int cmlw_ref(cml_stax_writer *w, const char *field, const char *id) {
	if (!(write_head(w, field) || put_c(w, '=') || put_s(w, id)))
		put_c(w, '\n');
	return w->in_error;
}

static int base64code2char(int c) {
	c &= 0x3f;
	if (c < 26) return 'A' + c;
	if (c < 52) return 'a' + c - 26;
	if (c < 62) return '0' + c - 52;
	return c == 62 ? '+' : '/';
}

int cmlw_bin(cml_stax_writer *w, const char *field, const char *data, int data_size) {
	char temp[21];
	int per_row = 1;
	sprintf(temp,"#%d", data_size);
	if (write_head(w, field) || put_s(w, temp))
		return w->in_error;
	for (; data_size >= 3; data_size -= 3, data += 3) {
		int a = ((const unsigned char*)data)[0];
		int b = ((const unsigned char*)data)[1];
		int c = ((const unsigned char*)data)[2];
		if (--per_row == 0) {
			per_row = 20;
			if (put_c(w, '\n') || write_indent(w, w->depth + 2))
				return w->in_error;
		}
		if (put_c(w, base64code2char(a >> 2)) ||
			put_c(w, base64code2char(a << 4 | b >> 4)) ||
			put_c(w, base64code2char(b << 2 | c >> 6)) ||
			put_c(w, base64code2char(c)))
			return w->in_error;
	}
	if (data_size != 0) {
		int a = *(const unsigned char*)data;
		if (put_c(w, base64code2char(a >> 2)))
			return w->in_error;
		if (data_size == 1) {
			if (put_c(w, base64code2char(a << 4)) || put_c(w, '='))
				return w->in_error;
		} else {
			int b = ((const unsigned char*)data)[1];
			if (put_c(w, base64code2char(a << 4 | b >> 4)) || put_c(w, base64code2char(b << 2)))
				return w->in_error;
		}
		put_c(w, '=');
	}
	put_c(w, '\n');
	return w->in_error;
}

#ifdef CONFIG_LIBC_FLOATINGPOINT

int cmlw_double(cml_stax_writer *w, const char *field, double value) {
	char buffer[32];
	if (write_head(w, field))
		return w->in_error;
	if (value == (long long)value)
		sprintf(buffer,"%lld.0\n", (long long)value);
	else
		sprintf(buffer,"%lg\n", value);
	return put_s(w, buffer) ? w->in_error : 0;
}

#endif
