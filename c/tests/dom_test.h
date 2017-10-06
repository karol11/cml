#ifndef _CML_DOM_TEST_H_
#define _CML_DOM_TEST_H_

#include "../src/dom.h"
#include "../src/tests.h"
#include "../src/string_builder.h"

static void to_string(string_builder *r, d_var *v, const char *field) {
	if (d_kind(v) != CMLD_UNDEFINED && field) {
		sb_puts(r, field);
		sb_append(r, ':');
	}
	switch (d_kind(v)) {
	case CMLD_UNDEFINED: break;
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
		d_dispose_dom(d);
		sb_dispose(&r);
	}
/*	d_type *page = d_add_type(d, "Page");
	d_field *p_items = d_add_field(page, "items");

	d_type *text_box = d_add_type(d, "TextBox");
	d_field *tb_x = d_add_field(text_box, "align");
	d_field *tb_y = d_add_field(text_box, "size");
	d_field *tb_content = d_add_field(text_box, "items");

	d_type *image = d_add_type(d, "Image");
	d_field *im_x = d_add_field(image, "align");
	d_field *im_y = d_add_field(image, "size");
	d_field *im_url = d_add_field(image, "url");

	d_type *span = d_add_type(d, "Span");
	d_field *sp_text = d_add_field(span, "text");
	d_field *sp_style = d_add_field(span, "style");

	d_type *text_style = d_add_type(d, "TextStyle");
	d_field *sf_family = d_add_field(span, "family");
	d_field *sf_weight = d_add_field(span, "weight");
	d_field *sf_size = d_add_field(span, "italic");
	d_field *sf_size = d_add_field(span, "size");
	d_field *sf_color = d_add_field(span, "color");
	d_field *sf_parent = d_add_field(span, "parent");
*/
/*
Page
items:
	Page.header
	align 1
	size 20
	items:
		Image.logo
		align 2
		size 20
		url "logo.gif"

		TextBox.title
		content:
			Span
			text "Title"
			style TextStyle
				parent TextStyle.main_style
					family "Arial"
					weight 400
					size 12
					color 0
				size 24
				color 16436877

	TextBox.mainText
	content:
		Span
		text "Hello "
		style=main_style

		Span
		text "world!"
		style TextStyle.bold
			weight 600
			parent=main_style
*/

}

#endif
