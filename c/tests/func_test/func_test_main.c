#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../../src/cml_dom.h"
#include "../../src/cml_dom_writer.h"
#include "../../src/cml_dom_reader.h"
#include "../../src/string_builder.h"

int getc_asciiz(void *context) {
	unsigned char **c = (unsigned char**) context;
	return *(*c)++;
}

int putc_sb(void *context, char c) {
	sb_append((string_builder*)context, c);
	return 1;
}

void on_error(void *context, const char *error, int line_num, int char_pos) {
	printf("error %s at %s %d:%d\n", error, context, line_num, char_pos);
	exit(-1);
}

void on_error2(void *context, const char *error, int line_num, int char_pos) {
	printf("error %s at %d:%d in text:\n", error, line_num, char_pos);
	puts((char*) context);
	exit(-1);
}

string_builder path;
size_t ids_numerator;

void assert(int c, const char *msg, ...) {
	if (!c) {
		va_list l;
		va_start(l, msg);
		printf("mismatch at %s\n", sb_get_str(&path));
		vprintf(msg, l);
		va_end(l);
		exit(-1);
	}
}

char index_buf[32];

void match(d_var *a, d_var *b);

void match_fields(d_struct *ra, d_struct *rb) {
	int path_pos = sb_size(&path);
	d_type *ta = d_ref_get_type(ra);
	d_type *tb = d_ref_get_type(rb);
	d_field *fa = d_enumerate_fields(ta);
	for (; fa; fa = d_next_field(fa)) {
		d_field *fb = d_lookup_field(tb, d_field_name(fa));
		assert(fb != 0, "no field %s", d_field_name(fa));
		sb_append(&path, '.');
		sb_puts(&path, d_field_name(fa));
		match(d_ref_peek_field(ra, fa), d_ref_peek_field(rb, fb));
		sb_trunc(&path, path_pos);
	}
}

void match(d_var *a, d_var *b) {
	assert(d_kind(a) == d_kind(b), "kind %d vs %d", d_kind(a), d_kind(b));
	switch(d_kind(a)) {
	case CMLD_UNDEFINED: break;
	case CMLD_INT: assert(d_as_int(a, -1) == d_as_int(b, -1), "int_val %d vs %d", d_as_int(a, -1), d_as_int(b, -1)); break;
	case CMLD_BOOL: assert(d_as_bool(a, -1) == d_as_bool(b, -1), "bool val %d vs $d", d_as_bool(a, -1), d_as_bool(b, -1)); break;
	case CMLD_STR: assert(strcmp(d_as_str(a, ""), d_as_str(b, "")) == 0, "str val %s vs %s", d_as_str(a, ""), d_as_str(b, "")); break;
	case CMLD_DOUBLE: assert(fabs(d_as_double(a, 0) - d_as_double(b, 0)) < 0.00001, "double val %llg vs %llg", d_as_double(a, 0), d_as_double(b, 0)); break;
	case CMLD_BINARY:
		{
			int i = -1, sa, sb;
			char *da = d_as_binary(a, &sa);
			char *db = d_as_binary(b, &sb);
			assert(sa == sb, "size %d vs %d", sa, sb);
			for (;++i < sa; da++, db++)
				assert(*da == *db, "byte[%d] %d vs %d", i, *da, *db);
		}
		break;
	case CMLD_ARRAY:
		{
			int path_pos = sb_size(&path);
			int i = -1, s = d_get_count(a);
			assert(s == d_get_count(b), "size %d vs %d", s, d_get_count(b));
			while (++i < s) {
				sprintf(index_buf, "[%d]", i);
				sb_puts(&path, index_buf);
				match(d_at(a, i), d_at(b, i));
				sb_trunc(&path, path_pos);
			}
		}
		break;
	case CMLD_STRUCT:
		{
			d_struct *ra = d_get_ref(a);
			d_struct *rb = d_get_ref(b);
			assert((ra == 0) == (rb == 0), "is_null %p %p", ra, rb);
			assert(d_ref_get_tag(ra) == d_ref_get_tag(rb), "topology ids %u %u", d_ref_get_tag(ra), d_ref_get_tag(rb));
			if (d_ref_get_tag(ra))
				return;
			{
				d_type *ta = d_ref_get_type(ra);
				d_type *tb = d_ref_get_type(rb);
				const char *na = d_ref_get_name(ra);
				const char *nb = d_ref_get_name(rb);
				d_ref_set_tag(ra, ids_numerator);
				d_ref_set_tag(rb, ids_numerator++);
				assert(strcmp(d_type_name(ta), d_type_name(tb)) == 0, "types %s vs %s", d_type_name(ta), d_type_name(tb));
				if (!na)
					na = "";
				if (!nb)
					nb = "";
				assert(strcmp(na, nb) == 0, "names %s vs %s", na, nb);
				match_fields(ra, rb);
				match_fields(rb, ra);
			}
		}
		break;
	}
}

void check_file(const char *name) {
	FILE *in = fopen(name, "r");
	d_dom *a, *b;
	string_builder sb;
	const char *b_data;
	printf("%s --------------\n", name);
	printf("\treading DOM from file\n");
	if (!in) {
		printf("can't read %s", name);
		exit(-1);
	}
	a = cml_read(getc, in, on_error, (void*)name);
	fclose(in);

	printf("\twriting DOM to an in-memory cml\n");
	sb_init(&sb);
	if (cml_write(a, putc_sb, &sb) < 0)
		printf("internal encoder error\n");

	printf("\treading a new copy of DOM from buffer\n");
	b_data = sb_get_str(&sb);
	b = cml_read(getc_asciiz, (void*) &b_data, on_error2, (void*)sb_get_str(&sb));

	sb_dispose(&sb);

	printf("\tcomparing two copies\n");
	sb_clear(&path);
	ids_numerator = 0;
	sb_init(&path);
	match(d_root(a), d_root(b));
	sb_dispose(&path);
	
	d_dispose_dom(a);
	d_dispose_dom(b);

	printf("success\n");
}

int main() {
	FILE *f = fopen("config.cml", "r");
	d_dom *config;
	int i, n;
	d_var *list;
	if (!f) {
		printf("can't open config.cml");
		exit(-1);
	}
	config = cml_read(getc, f, on_error, (void*)"config.cml");
	fclose(f);
	list = d_peek_field(d_root(config), d_lookup_field(d_lookup_type(config, "TestConfig"), "cmlList"));
	for (i = -1, n = d_get_count(list); ++i < n;)
		check_file(d_as_str(d_at(list, i), ""));
	d_dispose_dom(config);
	return 0;
}
