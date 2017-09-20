#include <stdio.h>
#include "dom.h"

void dump(cmld_var *v) {
	switch (cmld_kind(v)) {
	case CMLD_UNDEFINED: break;
	case CMLD_INT: printf("%ld", cmld_as_int(v, 0)); break;
	case CMLD_STR: printf("'%s'", cmld_as_str(v, "")); break;
	case CMLD_ARRAY:
		{
			int i = -1, n = cmld_get_count(v);
			printf("[");
			while (++i < n) {
				if (i) printf(",");
				dump(cmld_at(v, i));
			}
			printf("]");
		}
	case CMLD_STRUCT:
		{
			cmld_field *f = cmld_enumerate_fields(cmld_get_type(v));
			const char *id = cmld_get_name(v);
			printf("{'$':'%s'", cmld_type_name(cmld_get_type(v)));
			if (id)
				printf(",'#':'%s'", id);
			for (; f; f = cmld_next_field(f)) {
				printf(",'%s':", cmld_field_name(f));
				dump(cmld_peek_field(v, f));
			}
			printf("}");
		}
	}
}

int main() {
	cmld_dom *dom = cmld_alloc_dom();

	cmld_type *point = cmld_add_type(dom, "point");
	cmld_field *px = cmld_add_field(point, "x");
	cmld_field *py = cmld_add_field(point, "y");

	cmld_var *root = cmld_set_struct(cmld_root(dom), point);
	cmld_set_int(cmld_get_field(root, px), 5);
	cmld_set_int(cmld_get_field(root, py), 42);

	dump(cmld_root(dom));

	cmld_free_dom(dom);
}
