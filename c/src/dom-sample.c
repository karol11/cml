#include <stdio.h>
#include "dom.h"

void dump(cml_variant *v) {
	switch (cml_var_kind(v)) {
	case CML_V_UNDEFINED: break;
	case CML_V_INT: printf("%ld", cml_as_int(v, 0)); break;
	case CML_V_STR: printf("'%s'", cml_as_str(v, "")); break;
	case CML_V_ARRAY:
		{
			int i = -1, n = cml_get_count(v);
			printf("[");
			while (++i < n) {
				if (i) printf(",");
				dump(cml_get_at(v, i));
			}
			printf("]");
		}
	case CML_V_STRUCT:
		{
			cml_field *f = cml_enumerate_fields(cml_get_type(v));
			const char *id = cml_get_name(v);
			printf("{'$':'%s'", cml_type_name(cml_get_type(v)));
			if (id)
				printf(",'#':'%s'", id);
			for (; f; f = cml_next_field(f)) {
				printf(",'%s':", cml_field_name(f));
				dump(cml_peek_field(v, f));
			}
			printf("}");
		}
	}
}

int main() {
	cml_dom *dom = cml_alloc_dom();

	cml_type *point = cml_add_type(dom, "point");
	cml_field *px = cml_add_field(point, "x");
	cml_field *py = cml_add_field(point, "y");

	cml_variant *root = cml_set_struct(cml_root(dom), point);
	cml_set_int(cml_get_field(root, px), 5);
	cml_set_int(cml_get_field(root, py), 42);

	dump(cml_root(dom));

	cml_free_dom(dom);
}
