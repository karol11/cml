#include <stdio.h>
#include "tests.h"
#include "cml_stax_writer.h"
#include "dom.h"
#include "cml_dom_writer.h"

char id_buf[10];

enum node_states {
	N_VISITED = 1,
	N_DOUBLE_REF,
	N_WRITTEN
};

static const char *extract_id(d_var *v) {
	const char *name = d_get_name(v);
	if (name)
		return name;
	if (d_get_tag(v) == N_VISITED)
		return 0;
	sprintf(id_buf, "$%p", d_get_id(v));
	return id_buf;
}

static int traverse(cml_stax_writer *w, d_var *v, const char *field) {
	switch (d_kind(v)) {
	case CMLD_UNDEFINED: break;
	case CMLD_INT: return cml_write_int(w, field, d_as_int(v, 0));
	case CMLD_STR: return cml_write_str(w, field, d_as_str(v, ""));
	case CMLD_ARRAY:
		{
			int i = -1, n = d_get_count(v);
			int prev_state = cml_write_array(w, field);
			if (prev_state < 0)
				return prev_state;
			while (++i < n) {
				int r = traverse(w, d_at(v, i), 0);
				if (r < 0)
					return r;
			}
			return cml_write_end_array(w, prev_state);
		}
	case CMLD_STRUCT:
		if (!d_get_id(v))
			return cml_write_ref(w, field, "$");
		else if (d_get_tag(v) == N_WRITTEN) {
			return cml_write_ref(w, field, extract_id(v));
		} else {
			d_field *f = d_enumerate_fields(d_get_type(v));
			int prev_state = cml_write_struct(w, field, d_type_name(d_get_type(v)), extract_id(v));
			if (prev_state < 0)
				return prev_state;
			d_set_tag(v, N_WRITTEN);
			for (; f; f = d_next_field(f)) {
				int r = traverse(w, d_peek_field(v, f), d_field_name(f));
				if (r < 0)
					return r;
			}
			return cml_write_end_struct(w, prev_state);
		}
	}
	return 0;
}

static void check_multi_ref(d_var *v) {
	int k = d_kind(v);
	if (k == CMLD_ARRAY) {
		int i = -1, n = d_get_count(v);
		while (++i < n)
			check_multi_ref(d_at(v, i));
	} else if (k == CMLD_STRUCT) {
		size_t t = d_get_tag(v);
		if (t == N_VISITED)
			d_set_tag(v, N_DOUBLE_REF);
		else if (t == 0) {
			d_field *f = d_enumerate_fields(d_get_type(v));
			d_set_tag(v, N_VISITED);
			for (; f; f = d_next_field(f))
				check_multi_ref(d_peek_field(v, f));
		}
	}
}

int cml_write(d_dom *dom, int (*putc)(char c, void *context), void *putc_context) {
	cml_stax_writer *w = cml_create_writer(putc, putc_context);
	check_multi_ref(d_root(dom));
	{
		int r = traverse(w, d_root(dom), 0);
		cml_dispose_writer(w);
		d_untag(d_root(dom));
		return r;
	}
}
