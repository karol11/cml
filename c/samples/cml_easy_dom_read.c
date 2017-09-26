#include <stdio.h>
#include "dom.h"
#include "cml_dom_reader.h"

int getc_asciiz(void *s) {
	return *((*(char**)s)++);
}

int main() {
	const char *text =
		"polygon\n"
		"points:\n"
		"   point\n"
		"   x 10\n"
		"   y 10\n"
		"\n"
		"   point\n"
		"   x 1\n"
		"   y 42\n";
	d_dom *d = cml_read(getc_asciiz, (void*)&text, 0, 0);
	d_type *point = d_lookup_type(d, "point");
	d_field *px= d_lookup_field(point, "x");
	d_field *py= d_lookup_field(point, "y");
	d_var *points = d_peek_field(d_root(d), d_lookup_field(d_lookup_type(d, "polygon"), "points"));
	int i = -1, n = d_get_count(points);
	while (++i < n) {
		d_var *p = d_at(points, i);
		printf("x = %lld; y = %lld\n", d_as_int(d_peek_field(p, px), -1), d_as_int(d_peek_field(p, py), -1));
	}
	d_dispose_dom(d);
}
