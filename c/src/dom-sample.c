#include <stdio.h>
#include "dom.h"
#include "cml_dom_writer.h"
#include "string_builder.h"
#include "cml_stax_reader.h"

void dump(d_var *v) {
	switch (d_kind(v)) {
	case CMLD_UNDEFINED: printf("undefined"); break;
	case CMLD_INT: printf("%ld", d_as_int(v, 0)); break;
	case CMLD_STR: printf("'%s'", d_as_str(v, "")); break;
	case CMLD_ARRAY:
		{
			int i = -1, n = d_get_count(v);
			printf("[");
			while (++i < n) {
				if (i) printf(",");
				dump(d_at(v, i));
			}
			printf("]");
		}
	case CMLD_STRUCT:
		if (!d_get_id(v))
			printf("null");
		else {
			d_field *f = d_enumerate_fields(d_get_type(v));
			const char *id = d_get_name(v);
			printf("{'$':'%s'", d_type_name(d_get_type(v)));
			if (id)
				printf(",'#':'%s'", id);
			for (; f; f = d_next_field(f)) {
				printf(",'%s':", d_field_name(f));
				dump(d_peek_field(v, f));
			}
			printf("}");
		}
	}
}

int put_c_test(char c, void *unused){
	putchar(c);
	return 1;
}

int put_sb(char c, void *sb) {
	sb_append(c, (string_builder*) sb);
	return 1;
}
int getc_asciiz(void *s) {
	return *((*(char**)s)++);
}


int main() {
	d_dom *dom = d_alloc_dom();

	d_type *point = d_add_type(dom, "point");
	d_field *px = d_add_field(point, "x");
	d_field *py = d_add_field(point, "y");

	d_var *root = d_set_struct(d_root(dom), point);
	d_set_int(d_get_field(root, px), 5);
	d_set_int(d_get_field(root, py), 42);
/*
	d_set_int(d_root(dom), 11);

	d_gc(dom, 0, 0);
	dump(d_root(dom));*/

	cml_write(dom, put_c_test, 0);

	{
		string_builder sb;
		const char *s;
		cml_stax_reader *rd;
		sb_init(&sb);
		cml_write(dom, put_sb, &sb);
		s = sb_get_str(&sb);
		rd = cml_create_reader(getc_asciiz, (void*)&s);
		for (;;) {
			int r = cmlr_next(rd);
			if (r == CMLR_EOF) break;
			if (r == CMLR_ERROR) {
				printf("error: %s at %d:%d", cmlr_error(rd), cmlr_line_num(rd), cmlr_char_pos(rd));
				break;
			}
			if (*cmlr_field(rd))
				printf(",'%s':", cmlr_field(rd));
			switch (r) {
			case CMLR_INT: printf("%ld", cmlr_int(rd)); break;
			case CMLR_STRING: printf("'%s'", cmlr_str(rd)); break;
			case CMLR_START_STRUCT:
				printf("{'$':'%s'", cmlr_type(rd));
				if (*cmlr_id(rd))
					printf(",'#':'%s'", cmlr_id(rd));
				break;
			case CMLR_END_STRUCT: printf("}"); break;
			case CMLR_REF: printf("{'=':'%s'}", cmlr_id(rd)); break;
			case CMLR_START_ARRAY: printf("["); break;
			case CMLR_END_ARRAY: printf("]"); break;
			}
		}
		cml_dispose_reader(rd);
		sb_dispose(&sb);
	}

	d_dispose_dom(dom);
}
