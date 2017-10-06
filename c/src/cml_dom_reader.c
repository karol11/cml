#include <stdlib.h>
#include <string.h>
#include "cml_dom_writer.h"
#include "dom.h"
#include "cml_stax_reader.h"
#include "cml_dom_reader.h"

static const char *parse_var(d_var *v, d_dom *d, cml_stax_reader *r, int type) {
	const char *result = 0;
	switch(type) {
	case CMLR_INT: d_set_int(v, cmlr_int(r)); break;
	case CMLR_STRING: d_set_str(v, d, cmlr_str(r)); break;
	case CMLR_START_STRUCT:
		{
			d_type *struct_type = d_add_type(d, cmlr_type(r));
			const char *id = cmlr_id(r);
			d_set_struct(v, struct_type);
			if (*id)
				d_set_name(v, id);
			for (;;) {
				type = cmlr_next(r);
				if (type == CMLR_END_STRUCT)
					break;
				if ((result = parse_var(d_get_field(v, d_add_field(struct_type, cmlr_field(r))), d, r, type)))
					return result;
			}
		}
		break;
	case CMLR_REF:
		if (strcmp(cmlr_id(r), "$") == 0) 
			d_set_ref(v, 0);
		else {
			d_struct *id = d_get_named(d, cmlr_id(r));
			if (id)
				d_set_ref(v, id);
			else
				return "unresolved name";
		}
		break;
	case CMLR_START_ARRAY:
		d_set_array(v, d, 0);
		for (;;) {
			type = cmlr_next(r);
			if (type == CMLR_END_ARRAY)
				break;
			if ((result = parse_var(d_at(v, d_insert(v, d, d_get_count(v), 1)), d, r, type)))
				return result;
		}
		break;
	case CMLR_EOF: break;
	default:
		return "unexpected node";
	}
	return 0;
}

d_dom *cml_read(
	int (*getc)(void *context),
	void *getc_context,
	void (*on_error)(void *context, const char *error, int line_num, int char_pos),
	void *on_error_context)
{
	d_dom *d = d_alloc_dom();
	const char *err;
	cml_stax_reader *r = cmlr_create(getc, getc_context);
	if ((err = parse_var(d_root(d), d, r, cmlr_next(r)))) {
		if (*cmlr_error(r))
			err = cmlr_error(r);
		if (on_error)
			on_error(on_error_context, err, cmlr_line_num(r), cmlr_char_pos(r));
		d_dispose_dom(d);
		d = 0;
	}
	cmlr_dispose(r);
	return d;
}
