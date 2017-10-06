#include <stdio.h>
#include "../../src/dom.h"
#include "../../src/string_builder.h"
#include "../../src/utf8.h"
#include "../../src/cml_dom_writer.h"

FILE *in, *out;

char name_buffer[8];
int struct_numerator = 0;
int cur = ' ';
string_builder s;
d_dom *raw;

int sb_putc(void *context, char byte) {
	sb_append((string_builder*)context, byte);
	return 1;
}

int out_putc(void *context, char byte) {
	putc(byte, out);
	return 1;
}

char next() {
	do {
		if (cur <= 0)
			return 0;
		cur = fgetc(in);
	} while (cur <= ' ');
	return cur;
}

int match(char c) {
	if (cur == c) {
		next();
		return 1;
	}
	return 0;
}

int expected(char c) {
	if (match(c))
		return 1;
	printf("expected %c", c);
	return 0;
}

const char *parse_str() {
	sb_clear(&s);
	if (cur != '"') {
		printf("expected string");
		return 0;
	}
	while ((cur = fgetc(in)) != '"') {
		if (cur == '\\') {
			switch (cur = fgetc(in)) {
			case '\\':
			case '/':
			case '"': sb_append(&s, cur); break;
			case 'r': sb_append(&s, '\r'); break;
			case 'n': sb_append(&s, '\r'); break;
			case 't': sb_append(&s, '\r'); break;
			case 'b': sb_append(&s, '\b'); break;
			case 'f': sb_append(&s, '\f'); break;
			case 'u':
				{
					int i = 0, c = 5;
					while (--c) {
						cur = fgetc(in);
						if (cur >= '0' && cur <= '9') i = i << 4 | (cur - '0');
						else if (cur >= 'a' && cur <= 'f') i = i << 4 | (cur - 'a' + 10);
						else if (cur >= 'A' && cur <= 'F') i = i << 4 | (cur - 'A' + 10);
						else {
							printf("bad hex digit %c", cur);
							return 0;
						}
					}
					put_utf8(i, sb_putc, &s);
				}
				break;
			default:
				printf("bad escape %c", cur);
				return 0;
			}
		} else
			sb_append(&s, cur);
	}
	next();
	return sb_get_str(&s);
}

int expected_str(const char *str) {
	const char *s = str;
	while (*s) {
		if ((cur = getc(in)) != *s++) {
			printf("expected %s", str);
			return 0;
		}
	}
	return 1;
}

int parse_val(d_var *dst) {
	if (match('[')) {
		d_set_array(dst, raw, 0);
		if (!match(']')) {
			for (;;) {
				if (!parse_val(d_at(dst, d_insert(dst, raw, d_get_count(dst), 1))))
					return 0;
				if (!match(','))
					return expected(']');
			}
		}
	} else if (match('{')) {
		d_type *type;
		sprintf(name_buffer, "$%d", struct_numerator++);
		type = d_add_type(raw, name_buffer);
		d_set_struct(dst, type);
		if (!match('}')) {
			for (;;) {
				d_var *f;
				const char *field_name = parse_str();
				if (!field_name)
					return 0;
				f = d_get_field(dst, d_add_field(type, field_name));
				if (!expected(':'))
					return 0;
				if (!parse_val(f))
					return 0;
				if (!match(','))
					return expected('}');
			}
		}
	} else if (cur == '"') {
		const char *v = parse_str();
		if (!v)
			return 0;
		d_set_str(dst, raw, v);
	} else if (cur == '-' || (cur >= '0' && cur <= '9')) {
		int sign = match('-') ? -1 : 1;
		long long n = 0;
		for (; cur >= '0' && cur <= '9'; next())
			n = n * 10 + cur - '0';
		//todo . e E
		d_set_int(dst, n * sign);
	} else if (cur == 'n') {
		if (!expected_str("null"))
			return 0;
		d_set_ref(dst, 0);
	} else if (cur == 'f') {
		if (!expected_str("false"))
			return 0;
		d_set_int(dst, 0);
	} else if (cur == 't') {
		if (!expected_str("true"))
			return 0;
		d_set_int(dst, 1);	
	} else
		return 0;
	return 1;
}

int main(int argc, const char **args) {
	int r = 0;
	if (argc >= 3) {
		in = fopen(args[1], "r");
		out = fopen(args[2], "w");
		if (!in || !out) {
			fprintf(stderr, "can't open %s", in ? args[2] : args[1]);
			return -1;
		}
		raw = d_alloc_dom();
		sb_init(&s);
		next();
		parse_val(d_root(raw));

		cml_write(raw, out_putc, 0);

		sb_dispose(&s);
		fclose(in);
		fclose(out);
	} else
		printf("usage in_json_file out_cml_file\n");
	return r;
}
