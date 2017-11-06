#include <stdio.h>
#include "cml_config.h"
#include "cml_stax_writer.h"
#include "dom.h"
#include "cml_dom_writer.h"

enum node_states {
	N_VISITED = 1,
	N_DOUBLE_REF,
	N_WRITTEN
};

static const char *extract_id(d_var *v, char *id_buf) {
	const char *name = d_get_name(v);
	if (name)
		return name;
	if (d_get_tag(v) == N_VISITED)
		return 0;
	sprintf(id_buf, "_%p", d_get_ref(v));
	return id_buf;
}

struct cml_stax_writer_data {
	char id_buf[10];
	cml_stax_writer *w;
};

static int traverse(struct cml_stax_writer_data *d, d_var *v, const char *field) {
	switch (d_kind(v)) {
	case CMLD_UNDEFINED:
		if (field == 0)
			return cmlw_ref(d->w, 0, "_");
		break;
	case CMLD_INT: return cmlw_int(d->w, field, d_as_int(v, 0));
	case CMLD_BOOL: return cmlw_bool(d->w, field, d_as_bool(v, 0));
	case CMLD_STR: return cmlw_str(d->w, field, d_as_str(v, ""));
	case CMLD_BINARY:
		{
			int size;
			char *data = d_as_binary(v, &size);
			return cmlw_bin(d->w, field, data, size);
		}
#ifdef CONFIG_CML_FLOATINGPOINT
	case CMLD_DOUBLE:
		return cmlw_double(d->w, field, d_as_double(v, 0));
#endif
	case CMLD_ARRAY:
		{
			int i = -1, n = d_get_count(v);
			int prev_state = cmlw_array(d->w, field, n);
			if (prev_state < 0)
				return prev_state;
			while (++i < n) {
				int r = traverse(d, d_at(v, i), 0);
				if (r < 0)
					return r;
			}
			return cmlw_end_array(d->w, prev_state);
		}
	case CMLD_STRUCT:
		if (!d_get_ref(v))
			return cmlw_ref(d->w, field, "_");
		else if (d_get_tag(v) == N_WRITTEN) {
			return cmlw_ref(d->w, field, extract_id(v, d->id_buf));
		} else {
			d_field *f = d_enumerate_fields(d_get_type(v));
			int prev_state = cmlw_struct(d->w, field, d_type_name(d_get_type(v)), extract_id(v, d->id_buf));
			if (prev_state < 0)
				return prev_state;
			d_set_tag(v, N_WRITTEN);
			for (; f; f = d_next_field(f)) {
				int r = traverse(d, d_peek_field(v, f), d_field_name(f));
				if (r < 0)
					return r;
			}
			return cmlw_end_struct(d->w, prev_state);
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

int cml_write(d_dom *dom, int (*putc)(void *context, char c), void *putc_context) {
	struct cml_stax_writer_data data;
	data.w = cmlw_create(putc, putc_context);
	check_multi_ref(d_root(dom));
	{
		int r = traverse(&data, d_root(dom), 0);
		cmlw_dispose(data.w);
		d_untag(d_root(dom));
		return r;
	}
}
