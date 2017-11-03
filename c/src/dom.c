#include <stdlib.h>
#include <string.h>
#include "cml_config.h"
#include "dom.h"

typedef struct d_struct_tag d_struct;
typedef struct d_array_tag d_array;

struct d_var_tag {
	int type;
	union{
		long long int_val;
		int bool_val;
		const char *str_val;
		d_struct *struct_val;
		d_array *array_val;
		struct{
			char *bin_val;
			int bin_size;
		};
#ifdef CONFIG_CML_FLOATINGPOINT
		double dbl_val;
#endif
	};
};

struct d_field_tag {
	const char *name;
	int index;
	d_type *type;
	d_field *next;
};

struct d_type_tag{
	const char *name;
	d_dom *dom;
	int size;
	d_field *fields;
	d_type *next;
};

struct d_array_tag {
	int size;
	int allocated;
	d_var *items;
};

struct d_struct_tag {
	d_type *type;
	const char *name;
	d_array fields;
	size_t tag;
};

struct d_dom_tag {
	void *allocated;
	d_array named;
	d_var root;
	d_type *types;
};

struct d_str {
	char val_length[1];
};

static void *d_alloc(d_dom *dom, int size) {
	void **r = (void**) malloc(size + sizeof(void*));
	*r = dom->allocated;
	dom->allocated = (void*) r;
	return r + 1;
}

static void init_array(d_array *arr) {
	arr->size = arr->allocated = 0;
	arr->items = 0;
}

d_str *d_make_str(d_dom *dom, const char *s) {
	char *r = (char *) d_alloc(dom, strlen(s) + 1);
	strcpy(r, s);
	return (d_str*) r;
}

static const char *make_md_str(const char *s) {
	char *r = (char *) malloc(strlen(s) + 1);
	strcpy(r, s);
	return r;
}

static int array_insert(d_array *a, d_dom *dom, int at, int count) {
	if (a->size + count < a->allocated) {
		if (at < a->size)
			memmove(a->items + at + count, a->items + at, count * sizeof(d_var));
	} else {
		d_var *d = (d_var*) d_alloc(dom, (a->allocated = a->size + count + 16) * sizeof(d_var));
		if (a->items) {
			memcpy(d, a->items, at * sizeof(d_var));
			memcpy(d + at + count, a->items + at, (a->size - at) * sizeof(d_var));
		}
		a->items = d;
	}
	a->size += count;
	memset(a->items + at, 0, count * sizeof(d_var));
	return at;
}

static void array_delete(d_array *a, d_dom *dom, int at, int count) {
	memmove(a->items + at, a->items + at + count, (a->size - at - count) * sizeof(d_var));
	a->size -= count;
}

d_dom *d_alloc_dom() {
	d_dom *r = (d_dom*) malloc(sizeof(d_dom));
	r->allocated = 0;
	r->root.type = CMLD_UNDEFINED;
	r->types = 0;
	init_array(&r->named);
	return r;
}

void d_dispose_dom(d_dom *dom) {
	if (dom) {
		void **i = (void **) dom->allocated;
		while (i) {
			void **n = (void**) *i;
			free(i);
			i = n;
		}
		{
			d_type *t = dom->types;
			while (t) {
				d_type *nt = t->next;
				d_field *f = t->fields;
				while (f) {
					d_field *nf = f->next;
					free((void*)f->name);
					free(f);
					f = nf;
				}
				free((void*)t->name);
				free(t);
				t = nt;
			}
		}
		free(dom);
	}
}

d_type *d_lookup_type(d_dom *dom, const char *name) {
	d_type *t = dom->types;
	for (; t; t = t->next) {
		if (strcmp(t->name, name) == 0)
			return t;
	}
	return 0;
}

d_type *d_add_type(d_dom *dom, const char *name) {
	d_type *t = d_lookup_type(dom, name);
	if (t)
		return t;
	t = (d_type*) malloc(sizeof(d_type));
	t->name = make_md_str(name);
	t->size = 0;
	t->next = dom->types;
	dom->types = t;
	t->dom = dom;
	t->fields = 0;
	return t;
}

d_field *d_lookup_field(d_type *type, const char *name) {
	if (type) {
		d_field * r = type->fields;
		for (; r; r = r->next) {
			if (strcmp(r->name, name) == 0)
				return r;
		}
	}
	return 0;
}

d_field *d_add_field(d_type *type, const char *name) {
	d_field * r = d_lookup_field(type, name);
	if (r)
		return r;
	r = (d_field*) malloc(sizeof(d_field));
	r->name = make_md_str(name);
	r->index = type->size++;
	r->next = type->fields;
	r->type = type;
	type->fields = r;
	return r;
}

d_field *d_enumerate_fields(d_type *type) {
	return type ? type->fields : 0;
}

d_field *d_next_field(d_field *f) {
	return f ? f->next : 0;
}

const char *d_type_name(d_type *type) {
	return type ? type->name : 0;
}

const char *d_field_name(d_field *field) {
	return field ? field->name : 0;
}


d_type *d_ref_get_type(d_struct *s) {
	return s ? s->type : 0;
}

d_var *d_ref_peek_field(d_struct *s, d_field *field) {
	return s && field && field->type == s->type && field->index < s->fields.size ?
		s->fields.items + field->index :
		0;
}

d_var *d_ref_get_field(d_struct *s, d_field *field) {
	if (!s || !field || field->type != s->type)
		return 0;
	if (s->fields.size != s->type->size)
		array_insert(
			&s->fields,
			s->type->dom,
			s->fields.size,
			s->type->size - s->fields.size);
	return s->fields.items + field->index;
}

d_var *d_at(d_var *s, int index) {
	return s && s->type == CMLD_ARRAY && index < s->array_val->size ?
		s->array_val->items + index :
		0;
}

int d_get_count(d_var *s) {
	return s && s->type == CMLD_ARRAY ? s->array_val->size : 0;
}

long long d_as_int(d_var *v, long long def_val) {
	return v && v->type == CMLD_INT ? v->int_val :
#ifdef CONFIG_CML_FLOATINGPOINT
		v && v->type == CMLD_DOUBLE ? (long long) v->dbl_val : 
#endif
		def_val;
}

int d_as_bool(d_var *v, int def_val) {
	return v && v->type == CMLD_BOOL ? v->bool_val : def_val;
}

d_str *d_as_str_ref(d_var *v) {
	return v && v->type == CMLD_STR ? (d_str*) v->str_val : 0;
}

char *d_as_binary(d_var *v, int *out_size) {
	if (!v || v->type != CMLD_BINARY)
		return 0;
	if (out_size)
		*out_size = v->bin_size;
	return v->bin_val;
}

void d_set_binary(d_var *dst, d_dom *dom, char *data, int size) {
	if (dst) {
		dst->type = CMLD_BINARY;
		dst->bin_size = size;
		dst->bin_val = d_alloc(dom, size);
		if (data)
			memcpy(dst->bin_val, data, size);
	}
}

void d_set_int(d_var *dst, long long val) {
	if (dst) {
		dst->type = CMLD_INT;
		dst->int_val = val;
	} 
}

void d_set_bool(d_var *dst, int val) {
	if (dst) {
		dst->type = CMLD_BOOL;
		dst->bool_val = val;
	}
}

void d_set_str_ref(d_var *dst, d_str *ref) {
	if (dst && ref) {
		dst->type = CMLD_STR;
		dst->str_val = (char*) ref;
	}
}

d_var *d_set_array(d_var *dst, d_dom *dom, int size) {
	if (dst) {
		dst->type = CMLD_ARRAY;
		dst->array_val = (d_array*) d_alloc(dom, sizeof(d_array));
		init_array(dst->array_val);
		if (size > 0)
			array_insert(dst->array_val, dom, 0, size);
	}
	return dst;
}

d_struct *d_make_struct(d_type *type) {
	if (type) {
		d_struct *r = (d_struct*) d_alloc(type->dom, sizeof(d_struct));
		r->name = 0;
		r->type = type;
		r->tag = 0;
		init_array(&r->fields);
		return r;
	}
	return 0;
}

d_var *d_set_ref(d_var *dst, d_struct *src) {
	if (dst) {
		dst->type = CMLD_STRUCT;
		dst->struct_val = src;
	}
	return dst;
}

d_struct *d_get_ref(d_var *s) {
	return s && s->type == CMLD_STRUCT ? s->struct_val : 0;
}

void d_set_ref_tag(d_struct *s, size_t tag) {
	if (s)
		s->tag = tag;
}

size_t d_get_ref_tag(d_struct *s) {
	return s ? s->tag : 0;
}

void d_untag(d_var *v) {
	if (v && v->type == CMLD_ARRAY) {
		int c = v->array_val->size + 1;
		for (v = v->array_val->items; --c;)
			d_untag(v++);
	} else if (v->type == CMLD_STRUCT)
		d_untag_ref(v->struct_val);
}
void d_untag_ref(d_struct *s) {
	if (s && s->tag) {
		int c = s->fields.size + 1;
		d_var *v = s->fields.items;
		s->tag = 0;
		while (--c)
			d_untag(v++);
	}
}

void d_undefine(d_var *v) {
	if (v)
		v->type = CMLD_UNDEFINED;
}

int d_kind(d_var *v) {
	return v ? v->type : CMLD_UNDEFINED;
}

d_var *d_root(d_dom *dom) {
	return &dom->root;
}

void d_move(d_var *dst, d_var *src) {
	if (dst && src) {
		dst->type = src->type;
		dst->int_val = src->int_val;
		src->type = CMLD_UNDEFINED;
	}
}

static int get_named_index(d_dom *dom, const char *name) {
	int i = dom->named.size;
	while (--i >= 0)
		if (strcmp(dom->named.items[i].struct_val->name, name) == 0)
			return i;
	return -1;
}

d_struct *d_get_named(d_dom *dom, const char *name) {
	int i = get_named_index(dom, name);
	return i < 0 ? 0 : dom->named.items[i].struct_val;
}

const char *d_get_ref_name(d_struct *target) {
	return target ? target->name : 0;
}

void d_set_ref_name(d_struct *target, const char *name) {
	if (target) {
		d_dom *dom = target->type->dom;
		int my_prev_name = target->name ? get_named_index(target->type->dom, target->name) : -1;
		int old_name_bind = get_named_index(dom, name);
		if (my_prev_name < 0) {
			if (old_name_bind < 0) {
				int i = array_insert(&dom->named, dom, dom->named.size, 1);
				dom->named.items[i].type = CMLD_STRUCT;
				dom->named.items[i].struct_val = target;
				target->name = (char*) d_make_str(dom, name);
			} else {
				d_struct *old = dom->named.items[old_name_bind].struct_val;
				target->name = old->name;
				old->name = 0;
				dom->named.items[old_name_bind].struct_val = target;
			}
		} else {
			if (old_name_bind < 0) {
				target->name = (char*) d_make_str(dom, name);
			} else {
				d_struct *old = dom->named.items[old_name_bind].struct_val;
				if (old != target) {
					target->name = old->name;
					old->name = 0;
					array_delete(&dom->named, dom, old_name_bind, 1);
				}
			}
		}
	}
}

int d_insert(d_var *arr, d_dom *dom, int at, int count) {
	return arr && arr->type == CMLD_ARRAY ? array_insert(arr->array_val, dom, at, count) : -1;
}
void d_delete(d_var *arr, d_dom *dom, int at, int count) {
	if (arr && arr->type == CMLD_ARRAY)
		array_delete(arr->array_val, dom, at, count);
}

static int gc_visited(void *block) {
	size_t prev = ((size_t*)block)[-1];
	if ((prev & 1) == 0)
		((size_t*)block)[-1] = prev | 1;
	return prev & 1;
}
static void mark(d_var *i) {
	if (!i)
		return;
	switch (i->type) {
	case CMLD_STR:
		gc_visited((void*) i->str_val);
		break;
	case CMLD_BINARY:
		gc_visited((void*) i->bin_val);
		break;
	case CMLD_ARRAY:
		if (!gc_visited(i->array_val)) {
			int cnt = i->array_val->size + 1;
			for (i = i->array_val->items; --cnt; i++)
				mark(i);
		}
		break;
	case CMLD_STRUCT:
		d_gc_mark(i->struct_val);
		break;
	default:
		break;
	}
}

void d_gc_mark(void *struct_id) {
	d_struct *s = (d_struct*) struct_id;
	if (!gc_visited(s)) {
		if (s->name) {
			d_dom *dom = s->type->dom;
			dom->named.items[dom->named.size++].struct_val = s;
			gc_visited((void*)s->name);
		}
		{
			d_var *i = s->fields.items;
			int cnt = s->fields.size + 1;
			if (i)
				gc_visited(i);
			for (; --cnt; i++)
				mark(i);

		}
	}
}

void d_gc(d_dom *dom,
	void (*marker)(void*context), void *marker_context,
	void (*on_dispose)(void *context, void *id), void *on_dispose_context)
{
	dom->named.size = 0;
	mark(&dom->root);
	if (marker)
		marker(marker_context);
	{
		size_t **p = (size_t**) &dom->allocated;
		size_t *c = *p;
		while (c) {
			size_t n = *c;
			if (n & 1) {
				*c = n &= ~1;
				p = (size_t**) c;
			} else {
				*p = (size_t*) n;
				if (on_dispose)
					on_dispose(on_dispose_context, c);
				free(c);
			}
			c = (size_t*) n;
		}
	}
}

#ifdef CONFIG_CML_FLOATINGPOINT

double d_as_double(d_var *src, double def_val) {
	return src && src->type == CMLD_DOUBLE ? src->dbl_val : def_val;
}

void d_set_double(d_var *dst, double val) {
	if (dst) {
		dst->type = CMLD_DOUBLE;
		dst->dbl_val = val;
	} 

}

#endif
