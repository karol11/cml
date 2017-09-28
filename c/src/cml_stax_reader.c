#include <stdlib.h>
#include <string.h>
#include "tests.h"
#include "cml_stax_reader.h"
#include "string_builder.h"

typedef struct cml_stax_state_tag {
	struct cml_stax_state_tag *prev;
	int indent;
	int in_array;
} cml_stax_state;

struct cml_stax_reader_tag {
	int (*getc)(void *context);
	void *getc_context;

	string_builder str;
	string_builder type;
	string_builder field;
	long long int_val;
	int line_number;
	int char_pos;
	const char *error;

	int was_indented;
	int indent_with_tabs;
	int indent_pos;
	int cur;
	int cur_state_indent;
	int in_array;
	cml_stax_state *prev;
};

static int next_char(cml_stax_reader *r) {
	if (!r->cur || r->error)
		return 0;
	r->char_pos++;
	return r->cur = r->getc(r->getc_context);
}

static int error(cml_stax_reader *r, const char *e) {
	if (!r->error)
		r->error = e;
	return 0;
}

static int skip_ws(cml_stax_reader *r) {
	int c = r->cur;
	while (c == ' ' || c == '\t') 
		c = next_char(r);
	if (c == '#') {
		while (c != '\n' && c != '\r' && c)
			c = next_char(r);
	}
	return c;
}

static int expected_new_line(cml_stax_reader *r) {
	skip_ws(r);
	if (r->cur == '\n') {
		if (next_char(r) == '\r')
			next_char(r);
	} else if (r->cur == '\r') {
		if (next_char(r) == '\n')
			next_char(r);
	} else if (r->cur > 0)
		return error(r, "expected new line");
	r->line_number++;
	r->char_pos = 1;
	if (r->cur == ' ' || r->cur == '\t') {
		if (!r->was_indented) {
			r->was_indented = 1;
			r->indent_with_tabs = r->cur == '\t';
		}
		for (; r->cur == ' ' || r->cur == '\t'; next_char(r)) {
			if (r->indent_with_tabs != (r->cur == '\t'))
				return error(r, "mixed tabs and spaces");
		}
	}
	r->indent_pos = r->char_pos;
	return 1;
}

static int is_first_id_letter(int c) {
	return
		(c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z') ||
		c == '$' || c == '_';
}

static int is_digit(int c) {
	return c >= '0' && c <= '9';
}

static int is_id_letter(int c) {
	return is_first_id_letter(c) || is_digit(c);
}

static void get_id(cml_stax_reader *r, string_builder *s, const char *err) {
	int c = skip_ws(r);
	sb_clear(s);
	if (!is_first_id_letter(c))
		error(r, err);
	else {
		do
			sb_append(s, c);
		while (is_id_letter(c = next_char(r)));
	}
}

static int match(cml_stax_reader *r, char c) {
	if (skip_ws(r) == c) {
		next_char(r);
		return 1;
	}
	return 0;
}

static void push_state(cml_stax_reader *r, int in_array, int indent) {
	cml_stax_state *s = (cml_stax_state*) malloc(sizeof(cml_stax_state));
	s->in_array = r->in_array;
	s->indent = r->cur_state_indent;
	s->prev = r->prev;
	r->prev = s;
	r->in_array = in_array;
	r->cur_state_indent = indent;
}

static int parse_int(cml_stax_reader *r, int sign) {
	long long i = 0;
	int c = r->cur;
	for (; is_digit(c); c = next_char(r)) {
		long long n = i * 10 + c - '0';
		if (n < i) {
			error(r, "long overflow");
			return CMLR_ERROR;
		}
		i = n;
	}
	expected_new_line(r);
	r->int_val = i * sign;
	return CMLR_INT;
}

cml_stax_reader *cmlr_create(int (*getc)(void *context), void *getc_context) {
	cml_stax_reader *r = (cml_stax_reader *) malloc(sizeof(cml_stax_reader));
	r->getc = getc;
	r->getc_context = getc_context;
	sb_init(&r->str);
	sb_init(&r->type);
	sb_init(&r->field);
	r->error = 0;
	r->was_indented = 0;
	r->indent_with_tabs = 0; 
	r->line_number = 0;
	r->indent_pos = 0;
	r->char_pos = 0;
	r->cur_state_indent = 0;
	r->in_array = 1;
	r->cur = '\n';
	r->prev = 0;
	expected_new_line(r);
	return r;
}

void cmlr_dispose(cml_stax_reader *r) {
	sb_dispose(&r->str);
	sb_dispose(&r->type);
	sb_dispose(&r->field);
	for (;;) {
		cml_stax_state *n = r->prev;
		if (!n)
			break;
		r->prev = n->prev;
		free(n);
	}
	free(r);
}

long long cmlr_int(cml_stax_reader *r) { return r->int_val; }
const char *cmlr_str(cml_stax_reader *r) { return sb_get_str(&r->str); }
const char *cmlr_type(cml_stax_reader *r) { return sb_get_str(&r->type); }
const char *cmlr_id(cml_stax_reader *r) { return sb_get_str(&r->str); }
const char *cmlr_field(cml_stax_reader *r) { return sb_get_str(&r->field); }
const char *cmlr_error(cml_stax_reader *r) { return r->error; }
int cmlr_line_num(cml_stax_reader *r) { return r->line_number; }
int cmlr_char_pos(cml_stax_reader *r) { return r->char_pos; }

int cmlr_next(cml_stax_reader *r) {
	int result;
	int empty_line = r->cur == '\n' || r->cur == '\r';
	if (r->error)
		return CMLR_ERROR;
	if (empty_line) {
		do
			expected_new_line(r);
		while (r->cur == '\n' || r->cur == '\r');
	}
	if (r->indent_pos < r->cur_state_indent || r->cur <= 0 || empty_line) {
		int was_in_array = r->in_array;
		sb_clear(&r->field);
		if (!r->prev)
			return CMLR_EOF;
		{
			cml_stax_state *s = r->prev;
			r->in_array = s->in_array;
			r->cur_state_indent = s->indent;
			r->prev = s->prev;
			free(s);
			for (s = r->prev; s && s->indent >= r->indent_pos; s = s->prev) {
				if (s->indent == r->indent_pos && !s->in_array) {
					s->indent++;
					break;
				}
			}
		}
		return was_in_array ? CMLR_END_ARRAY : CMLR_END_STRUCT;
	}
	if (r->in_array)
		sb_clear(&r->field);
	else
		get_id(r, &r->field, "expected field id");
	if (match(r, '=')) {
		get_id(r, &r->str, "expected object id");
		expected_new_line(r);
		result = CMLR_REF;
	} else if (match(r, ':')) {
		int array_indent = r->indent_pos;
		expected_new_line(r);
		push_state(r, 1, r->indent_pos <= array_indent ? r->indent_pos + 1 : r->indent_pos);
		result = CMLR_START_ARRAY;
	} else if (match(r, '"')) {
		int c = r->cur;
		sb_clear(&r->str);
		for (;; c = next_char(r)) {
			if (c == '"') {
				c = next_char(r);
				if (c == '"')
					sb_append(&r->str, '\"');
				else
					break;
			} else if (!c) {
				error(r, "string not terminated");
				return CMLR_ERROR;
			} else
				sb_append(&r->str, c);
		}
		expected_new_line(r);
		result = CMLR_STRING;
	} else if (is_digit(r->cur)) {
		result = parse_int(r, 1);
	} else if (r->cur == '-') {
		result = parse_int(r, -1);
	} else {
		get_id(r, &r->type, "expected type id");
		if (match(r, '.'))
			get_id(r, &r->str, "expected object id");
		else
			sb_clear(&r->str);
		{
			int object_indent = r->indent_pos;
			expected_new_line(r);
			{
				int has_fields = r->in_array ? r->indent_pos == object_indent : r->indent_pos > object_indent;
				push_state(r, 0, has_fields ? r->indent_pos : r->indent_pos + 1);
			}
		}
		result = CMLR_START_STRUCT;
	}
	return r->error ? CMLR_ERROR : result;
}
