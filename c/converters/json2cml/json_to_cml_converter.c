#include <stdio.h>
#include <string.h>
#include <math.h>
#include "../../src/cml_dom.h"
#include "../../src/string_builder.h"
#include "../../src/utf8.h"
#include "../../src/cml_dom_writer.h"
#include "../../src/cml_dom_reader.h"

FILE *in, *out;

char name_buffer[8];
int struct_numerator = 0;
int cur = ' ';
string_builder s;
d_dom *raw;

d_dom *config = 0;
d_var *conf_type_field = 0;
d_var *conf_ref_field = 0;
d_var *conf_id_field = 0;

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

const char *find_name(d_var *s, d_var *names) {
	d_field *ff = d_enumerate_fields(d_get_type(s));
	for (; ff; ff = d_next_field(ff)) {
		int i = -1, n = d_get_count(names);
		while (++i < n) {
			if (strcmp(d_field_name(ff), d_as_str(d_at(names, i), "")) == 0) {
				const char *type_name = d_as_str(d_peek_field(s, ff), 0);
				if (type_name) {
					d_undefine(d_peek_field(s, ff));
					return type_name;
				}
			}
		}
	}
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
			case 'n': sb_append(&s, '\n'); break;
			case 't': sb_append(&s, '\t'); break;
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

long long get_int() {
	long long r = 0;
	for (; cur >= '0' && cur <= '9'; next())
		r = r * 10 + cur - '0';
	return r;
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
				if (!match(',')) {
					const char *tn = find_name(dst, conf_type_field);
					const char *idn = find_name(dst, conf_id_field);
					const char *refn = find_name(dst, conf_ref_field);
					if (tn) {
						d_type *t = d_add_type(raw, tn);
						d_struct *s = d_make_struct(t);
						d_field *f = d_enumerate_fields(type);
						for (; f; f = d_next_field(f))
							d_move(d_ref_get_field(s, d_add_field(t, d_field_name(f))), d_peek_field(dst, f));
						d_set_ref(dst, s);
					}
					if (idn)
						d_set_name(dst, idn);
					if (!tn && !idn && refn) {
						d_struct *target = d_get_named(raw, refn);
						if (!target) {
							printf("unresolved name %s", refn);
							return 0;
						}
						d_set_ref(dst, target);
					}
					return expected('}');
				}
			}
		}
	} else if (cur == '"') {
		const char *v = parse_str();
		if (!v)
			return 0;
		d_set_str(dst, raw, v);
	} else if (cur == '-' || (cur >= '0' && cur <= '9')) {
		int sign = match('-') ? -1 : 1;
		long long n = get_int();
		if (match('.')) {
			double d = (double) n;
			double p = 0.1;
			for (; cur >= '0' && cur <= '9'; next(), p *= 0.1)
				d += p * (cur - '0');
			if (match('e')) {
				int ps = match('-') ? -1 : 1;
				d = d * pow(10, ps * (double) get_int());
			}
			d_set_double(dst, sign * d);
		} else if (match('e')) {
			int ps = match('-') ? -1 : 1;
			d_set_double(dst, sign * (double)n * pow(10, ps * (double) get_int()));
		} else
			d_set_int(dst, n * sign);
	} else if (cur == 'n') {
		if (!expected_str("null"))
			return 0;
		d_set_ref(dst, 0);
	} else if (cur == 'f') {
		if (!expected_str("false"))
			return 0;
		d_set_bool(dst, 0);
	} else if (cur == 't') {
		if (!expected_str("true"))
			return 0;
		d_set_bool(dst, 1);	
	} else
		return 0;
	return 1;
}

void file_err(void *context, const char *error, int line_num, int char_pos) {
	printf("error %s at file %s %d:%d", error, (char*)context, line_num, char_pos);
}

int main(int argc, const char **args) {
	if (argc >= 3) {
		in = fopen(args[1], "r");
		if (!in) {
			fprintf(stderr, "can't read %s", args[1]);
			return -1;
		}
		if (argc >= 4) {
			FILE *cf = fopen(args[3], "r");
			if (!cf) {
				fprintf(stderr, "can't open confid %s", args[3]);
				return -1;
			}
			config = cml_read(getc, cf, file_err, (void*)args[3]);
			fclose(cf);
			conf_type_field = d_peek_field(d_root(config), d_lookup_field(d_lookup_type(config, "config"), "typeFields"));
			conf_ref_field = d_peek_field(d_root(config), d_lookup_field(d_lookup_type(config, "config"), "refFields"));
			conf_id_field = d_peek_field(d_root(config), d_lookup_field(d_lookup_type(config, "config"), "idFields"));
		}

		raw = d_alloc_dom();
		sb_init(&s);
		next();
		if (parse_val(d_root(raw))) {
			out = fopen(args[2], "w");
			if (!out) {
				fprintf(stderr, "can't write %s", args[2]);
				return -1;
			}
			cml_write(raw, out_putc, 0);
			fclose(out);
		}
		d_dispose_dom(config);
		d_dispose_dom(raw);
		sb_dispose(&s);
		fclose(in);
	} else
		printf("usage in_json_file out_cml_file [optional_config]\n");
	return 0;
}
