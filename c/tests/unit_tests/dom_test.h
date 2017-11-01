#ifndef _CML_DOM_TEST_H_
#define _CML_DOM_TEST_H_

#include "../../src/dom.h"
#include "../../src/cml_config.h"
#include "../../src/string_builder.h"

static void to_string(string_builder *r, d_var *v, const char *field) {
	if (d_kind(v) != CMLD_UNDEFINED && field) {
		sb_puts(r, field);
		sb_append(r, ':');
	}
	switch (d_kind(v)) {
	case CMLD_UNDEFINED: break;
	case CMLD_BOOL:
		sb_puts(r, d_as_bool(v, 0) ? "true" : "false");
		break;
#ifdef CONFIG_CML_FLOATINGPOINT
	case CMLD_DOUBLE:
		{
			char buf[32];
			sprintf(buf, "%lg", d_as_double(v, 0));
			sb_puts(r, buf);
		}
		break;
#endif
	case CMLD_INT:
		{
			char buf[32];
			sprintf(buf, "%lld", d_as_int(v, 0));
			sb_puts(r, buf);
		}
		break;
	case CMLD_STR:
		sb_append(r, '"');
		sb_puts(r, d_as_str(v, ""));
		sb_append(r, '"');		
		break;
	case CMLD_BINARY:
		{
			int size;
			char buf[32];
			char *d = d_as_binary(v, &size);
			for (++size; --size; d++) {
				sprintf(buf, "%02x", *d & 0xff);
				sb_puts(r, buf);
			}
		}
		break;
	case CMLD_ARRAY:
		{
			int i = -1, n = d_get_count(v);
			sb_append(r, '[');
			while (++i < n) {
				if (i) sb_append(r, ',');
				to_string(r, d_at(v, i), 0);
			}
			sb_append(r, ']');
		}
		break;
	case CMLD_STRUCT:
		if (!d_get_ref(v))
			sb_puts(r, "null");
		else if (d_get_tag(v)) {
			sb_append(r, '=');
			sb_puts(r, d_get_name(v));
		} else {
			d_field *f = d_enumerate_fields(d_get_type(v));
			const char *id = d_get_name(v);
			sb_puts(r, d_type_name(d_get_type(v)));
			d_set_tag(v, 1);
			if (id) {
				sb_append(r, ':');
				sb_puts(r, id);
			}
			sb_append(r, '{');
			for (; f; f = d_next_field(f))
				to_string(r, d_peek_field(v, f), d_field_name(f));
			sb_append(r, '}');
		}
	}
}

void dom_test() {
	d_dom *d = d_alloc_dom();
	d_set_int(d_root(d), 42);
	ASSERT(d_as_int(d_root(d), 0) == 42);
	d_dispose_dom(d);
	
	d = d_alloc_dom();
	{
		d_type *task = d_add_type(d, "Task");
		d_field *t_desc = d_add_field(task, "description");
		d_field *t_subtasks = d_add_field(task, "subtasks");

		d_var *r = d_set_struct(d_root(d), task);
		d_var *sub = d_set_array(d_get_field(r, t_subtasks), d, 2);
		d_var *t1 = d_set_struct(d_at(sub, 0), task);
		d_var *t2 = d_set_struct(d_at(sub, 1), task);
		d_set_str(d_get_field(r, t_desc), d, "N-slave the World");
		d_set_str(d_get_field(t1, t_desc), d, "Buy Cola");
		d_set_str(d_get_field(t2, t_desc), d, "Sell half price");
	}
	{
		string_builder r;
		sb_init(&r);
		to_string(&r, d_root(d), 0);
		ASSERT(strcmp(sb_get_str(&r), "Task{subtasks:[Task{description:\"Buy Cola\"},Task{description:\"Sell half price\"}]description:\"N-slave the World\"}") == 0);

		d_set_str(d_root(d), d, "Hello");
		sb_clear(&r);
		d_gc(d, 0, 0, 0, 0);
		to_string(&r, d_root(d), 0);
		ASSERT(strcmp(sb_get_str(&r), "\"Hello\"") == 0);

		d_set_binary(d_root(d), d, 0, 25);
		{
			int i;
			char *v = d_as_binary(d_root(d), 0);
			for (i = 0; i < 25; i++)
				v[i] = i*10;
		}
		sb_clear(&r);
		to_string(&r, d_root(d), 0);
		ASSERT(strcmp(sb_get_str(&r), "000a141e28323c46505a646e78828c96a0aab4bec8d2dce6f0") == 0);

		d_dispose_dom(d);
		sb_dispose(&r);
	}
}

#endif
