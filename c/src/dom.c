#include <stdlib.h>
#include <string.h>
#include "dom.h"

typedef struct cml_struct_tag cml_struct;
typedef struct cml_array_tag cml_array;

struct cml_variant_tag {
	int type;
	union{
		long long int_val;
		const char *str_val;
		cml_struct *struct_val;
		cml_array *array_val;
	};
};

struct cml_field_tag {
	const char *name;
	int index;
	cml_field *next;
};

struct cml_type_tag{
	const char *name;
	cml_dom *dom;
	int size;
	cml_field *fields;
	cml_type *next;
};

struct cml_array_tag {
	int size;
	int allocated;
	cml_variant *items;
};

struct cml_struct_tag {
	cml_type *type;
	const char *name;
	cml_array fields;
};

struct cml_dom_tag {
	void *allocated;
	cml_array named;
	cml_variant root;
	cml_type *types;
};

static void *cml_alloc(cml_dom *dom, int size) {
	void **r = (void**) malloc(size + sizeof(void*));
	*r = dom->allocated;
	dom->allocated = (void*) r;
	return r + 1;
}

static void init_array(cml_array *arr) {
	arr->size = arr->allocated = 0;
	arr->items = 0;
}

static const char *make_str(cml_dom *dom, const char *s) {
	char *r = (char *) cml_alloc(dom, strlen(s) + 1);
	strcpy(r, s);
	return r;
}

static const char *make_md_str(const char *s) {
	char *r = (char *) malloc(strlen(s) + 1);
	strcpy(r, s);
	return r;
}

static int array_insert(cml_array *a, cml_dom *dom, int at, int count) {
	if (a->size + count < a->allocated) {
		if (at < a->size)
			memmove(a->items + at + count, a->items + at, count * sizeof(cml_variant));
	} else {
		cml_variant *d = (cml_variant*) cml_alloc(dom, (a->allocated = a->size + count + 16) * sizeof(cml_variant));
		if (a->items) {
			memcpy(d, a->items, at * sizeof(cml_variant));
			memcpy(d + at + count, a->items + at, (a->size - at) * sizeof(cml_variant));
		}
		a->items = d;
	}
	a->size += count;
	memset(a->items + at, 0, count * sizeof(cml_variant));
	return at;
}

static void array_delete(cml_array *a, cml_dom *dom, int at, int count) {
	memmove(a->items + at, a->items + at + count, (a->size - at - count) * sizeof(cml_variant));
	a->size -= count;
}

cml_dom *cml_alloc_dom() {
	cml_dom *r = (cml_dom*) malloc(sizeof(cml_dom));
	r->allocated = 0;
	r->root.type = CML_V_UNDEFINED;
	r->types = 0;
	init_array(&r->named);
	return r;
}

void cml_free_dom(cml_dom *dom) {
	void **i = (void **) dom->allocated;
	while (i) {
		void **n = (void**) *i;
		free(i);
		i = n;
	}
	{
		cml_type *t = dom->types;
		while (t) {
			cml_type *nt = t->next;
			cml_field *f = t->fields;
			while (f) {
				cml_field *nf = f->next;
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

cml_type *cml_lookup_type(cml_dom *dom, const char *name) {
	cml_type *t = dom->types;
	for (; t; t = t->next) {
		if (strcmp(t->name, name) == 0)
			return t;
	}
	return 0;
}

cml_type *cml_add_type(cml_dom *dom, const char *name) {
	cml_type *t = cml_lookup_type(dom, name);
	if (t)
		return t;
	t = (cml_type*) malloc(sizeof(cml_type));
	t->name = make_md_str(name);
	t->size = 0;
	t->next = dom->types;
	dom->types = t;
	t->dom = dom;
	t->fields = 0;
	return t;
}

cml_field *cml_lookup_field(cml_type *type, const char *name) {
	cml_field * r = type->fields;
	for (; r; r = r->next) {
		if (strcmp(r->name, name) == 0)
			return r;
	}
	return 0;
}

cml_field *cml_add_field(cml_type *type, const char *name) {
	cml_field * r = cml_lookup_field(type, name);
	if (r)
		return r;
	r = (cml_field*) malloc(sizeof(cml_field));
	r->name = make_md_str(name);
	r->index = type->size++;
	r->next = type->fields;
	type->fields = r;
	return r;
}

cml_field *cml_enumerate_fields(cml_type *type) {
	return type->fields;
}

cml_field *cml_next_field(cml_field *f) {
	return f->next;
}

const char *cml_type_name(cml_type *type) {
	return type ? type->name : 0;
}

const char *cml_field_name(cml_field *field) {
	return field ? field->name : 0;
}


cml_type *cml_get_type(cml_variant *s) {
	return s && s->type == CML_V_STRUCT ? s->struct_val->type : 0;
}

cml_variant *cml_peek_field(cml_variant *s, cml_field *field) {
	return s && s->type == CML_V_STRUCT && s->struct_val->fields.size > field->index ?
		s->struct_val->fields.items + field->index :
		0;
}

cml_variant *cml_get_field(cml_variant *s, cml_field *field) {
	if (!s || s->type != CML_V_STRUCT)
		return 0;
	if (field->index >= s->struct_val->fields.size)
		array_insert(
			&s->struct_val->fields,
			s->struct_val->type->dom,
			s->struct_val->fields.size,
			field->index - s->struct_val->fields.size + 1);
	return s->struct_val->fields.items + field->index;
}

cml_variant *cml_get_at(cml_variant *s, int index) {
	return s && s->type == CML_V_ARRAY && s->array_val->size < index ?
		s->array_val->items + index :
		0;
}

int cml_get_count(cml_variant *s) {
	return s && s->type == CML_V_ARRAY ? s->array_val->size : 0;
}

long long cml_as_int(cml_variant *v, long long def_val) {
	return v && v->type == CML_V_INT ? v->int_val : def_val;
}

const char *cml_as_str(cml_variant *v, const char *def_val) {
	return v && v->type == CML_V_STR ? v->str_val : def_val;
}

void cml_set_int(cml_variant *dst, long long val) {
	if (dst) {
		dst->type = CML_V_INT;
		dst->int_val = val;
	} 
}

void cml_set_str(cml_variant *dst, cml_dom *dom, const char *val) {
	if (dst) {
		dst->type = CML_V_STR;
		dst->str_val = make_str(dom, val);
	}
}

cml_variant *cml_set_array(cml_variant *dst, cml_dom *dom, int size) {
	if (dst) {
		dst->type = CML_V_ARRAY;
		dst->array_val = (cml_array*) cml_alloc(dom, sizeof(cml_array));
		init_array(dst->array_val);
		if (size > 0)
			array_insert(dst->array_val, dom, 0, size);
	}
	return dst;
}

cml_variant *cml_set_struct(cml_variant *dst, cml_type *type) {
	if (dst) {
		dst->type = CML_V_STRUCT;
		dst->struct_val = (cml_struct*) cml_alloc(type->dom, sizeof(cml_struct));
		dst->struct_val->name = 0;
		dst->struct_val->type = type;
		init_array(&dst->struct_val->fields);
	}
	return dst;
}

cml_variant *cml_set_ref(cml_variant *dst, cml_variant *src) {
	if (dst) {
		if (!src)
			dst->type = CML_V_UNDEFINED;
		else {
			dst->type = src->type;
			dst->int_val = src->int_val;
		}
	}
	return dst;
}

void *cml_get_id(cml_variant *s) {
	return s && s->type == CML_V_STRUCT ? s->struct_val : 0;
}

int cml_var_kind(cml_variant *v) {
	return v ? v->type : CML_V_UNDEFINED;
}

cml_variant *cml_root(cml_dom *dom) {
	return &dom->root;
}

static int get_named_index(cml_dom *dom, const char *name) {
	int i = dom->named.size;
	while (--i >= 0)
		if (strcmp(dom->named.items[i].struct_val->name, name) == 0)
			return i;
	return -1;
}

cml_variant *cml_get_named(cml_dom *dom, const char *name) {
	int i = get_named_index(dom, name);
	return i < 0 ? 0 : &dom->named.items[i];
}

const char *cml_get_name(cml_variant *target) {
	return target && target->type == CML_V_STRUCT ? target->struct_val->name : 0;
}
void cml_set_name(cml_variant *struc, const char *name) {
	if (struc && struc->type == CML_V_STRUCT) {
		cml_struct* target = struc->struct_val;
		cml_dom *dom = target->type->dom;
		int my_prev_name = target->name ? get_named_index(target->type->dom, target->name) : -1;
		int old_name_bind = get_named_index(dom, name);
		if (my_prev_name < 0) {
			if (old_name_bind < 0) {
				int i = array_insert(&dom->named, dom, dom->named.size, 1);
				dom->named.items[i].type = CML_V_STRUCT;
				dom->named.items[i].struct_val = target;
				target->name = make_str(dom, name);
			} else {
				cml_struct *old = dom->named.items[old_name_bind].struct_val;
				target->name = old->name;
				old->name = 0;
				dom->named.items[old_name_bind].struct_val = target;
			}
		} else {
			if (old_name_bind < 0) {
				target->name = make_str(dom, name);
			} else {
				cml_struct *old = dom->named.items[old_name_bind].struct_val;
				target->name = old->name;
				old->name = 0;
				array_delete(&dom->named, dom, old_name_bind, 1);
			}
		}
	}
}

int cml_insert(cml_variant *arr, cml_dom *dom, int at, int count) {
	return arr && arr->type == CML_V_ARRAY ? array_insert(arr->array_val, dom, at, count) : -1;
}
void cml_delete(cml_variant *arr, cml_dom *dom, int at, int count) {
	if (arr && arr->type == CML_V_ARRAY)
		array_delete(arr->array_val, dom, at, count);
}

static void mark(cml_variant *i) {
	if (!i)
		return;
	switch (i->type) {
	case CML_V_STR:
		((int*)i->str_val)[-1] |= 1;
		break;
	case CML_V_ARRAY:
		if ((((int*)i->array_val)[-1] & 1) == 0) {
			int cnt = i->array_val->size + 1;
			((int*)i->array_val)[-1] |= 1;
			for (i = i->array_val->items; --cnt; i++)
				mark(i);
		}
	case CML_V_STRUCT:
		if ((((int*)i->struct_val)[-1] & 1) == 0) {
			int cnt = i->struct_val->fields.size + 1;
			((int*)i->struct_val)[-1] |= 1;
			if (i->struct_val->name) {
				cml_dom *dom = i->struct_val->type->dom;
				dom->named.items[dom->named.size++].struct_val = i->struct_val;
			}
			for (i = i->struct_val->fields.items; --cnt; i++)
				mark(i);
		}
	default:
		break;
	}
}
void cml_gc(cml_dom *dom, void (*marker)(void*context), void *marker_context)
{
	dom->named.size = 0;
	mark(&dom->root);
	if (marker)
		marker(marker_context);
	{
		int **p = (int**) &dom->allocated;
		for (;;) {
			int *c = *p;
			if (!*c)
				break;
			if (*c & 1) {
				*c &= ~1;
				p = (int**) *p;
			} else {
				*p = (int*) *c;
				free(c);
			}
		}
	}
}
