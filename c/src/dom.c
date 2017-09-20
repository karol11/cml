#include <stdlib.h>
#include <string.h>
#include "dom.h"

typedef struct cmld_struct_tag cmld_struct;
typedef struct cmld_array_tag cmld_array;

struct cmld_var_tag {
	int type;
	union{
		long long int_val;
		const char *str_val;
		cmld_struct *struct_val;
		cmld_array *array_val;
	};
};

struct cmld_field_tag {
	const char *name;
	int index;
	cmld_field *next;
};

struct cmld_type_tag{
	const char *name;
	cmld_dom *dom;
	int size;
	cmld_field *fields;
	cmld_type *next;
};

struct cmld_array_tag {
	int size;
	int allocated;
	cmld_var *items;
};

struct cmld_struct_tag {
	cmld_type *type;
	const char *name;
	cmld_array fields;
};

struct cmld_dom_tag {
	void *allocated;
	cmld_array named;
	cmld_var root;
	cmld_type *types;
};

static void *cmld_alloc(cmld_dom *dom, int size) {
	void **r = (void**) malloc(size + sizeof(void*));
	*r = dom->allocated;
	dom->allocated = (void*) r;
	return r + 1;
}

static void init_array(cmld_array *arr) {
	arr->size = arr->allocated = 0;
	arr->items = 0;
}

static const char *make_str(cmld_dom *dom, const char *s) {
	char *r = (char *) cmld_alloc(dom, strlen(s) + 1);
	strcpy(r, s);
	return r;
}

static const char *make_md_str(const char *s) {
	char *r = (char *) malloc(strlen(s) + 1);
	strcpy(r, s);
	return r;
}

static int array_insert(cmld_array *a, cmld_dom *dom, int at, int count) {
	if (a->size + count < a->allocated) {
		if (at < a->size)
			memmove(a->items + at + count, a->items + at, count * sizeof(cmld_var));
	} else {
		cmld_var *d = (cmld_var*) cmld_alloc(dom, (a->allocated = a->size + count + 16) * sizeof(cmld_var));
		if (a->items) {
			memcpy(d, a->items, at * sizeof(cmld_var));
			memcpy(d + at + count, a->items + at, (a->size - at) * sizeof(cmld_var));
		}
		a->items = d;
	}
	a->size += count;
	memset(a->items + at, 0, count * sizeof(cmld_var));
	return at;
}

static void array_delete(cmld_array *a, cmld_dom *dom, int at, int count) {
	memmove(a->items + at, a->items + at + count, (a->size - at - count) * sizeof(cmld_var));
	a->size -= count;
}

cmld_dom *cmld_alloc_dom() {
	cmld_dom *r = (cmld_dom*) malloc(sizeof(cmld_dom));
	r->allocated = 0;
	r->root.type = CMLD_UNDEFINED;
	r->types = 0;
	init_array(&r->named);
	return r;
}

void cmld_free_dom(cmld_dom *dom) {
	void **i = (void **) dom->allocated;
	while (i) {
		void **n = (void**) *i;
		free(i);
		i = n;
	}
	{
		cmld_type *t = dom->types;
		while (t) {
			cmld_type *nt = t->next;
			cmld_field *f = t->fields;
			while (f) {
				cmld_field *nf = f->next;
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

cmld_type *cmld_lookup_type(cmld_dom *dom, const char *name) {
	cmld_type *t = dom->types;
	for (; t; t = t->next) {
		if (strcmp(t->name, name) == 0)
			return t;
	}
	return 0;
}

cmld_type *cmld_add_type(cmld_dom *dom, const char *name) {
	cmld_type *t = cmld_lookup_type(dom, name);
	if (t)
		return t;
	t = (cmld_type*) malloc(sizeof(cmld_type));
	t->name = make_md_str(name);
	t->size = 0;
	t->next = dom->types;
	dom->types = t;
	t->dom = dom;
	t->fields = 0;
	return t;
}

cmld_field *cmld_lookup_field(cmld_type *type, const char *name) {
	cmld_field * r = type->fields;
	for (; r; r = r->next) {
		if (strcmp(r->name, name) == 0)
			return r;
	}
	return 0;
}

cmld_field *cmld_add_field(cmld_type *type, const char *name) {
	cmld_field * r = cmld_lookup_field(type, name);
	if (r)
		return r;
	r = (cmld_field*) malloc(sizeof(cmld_field));
	r->name = make_md_str(name);
	r->index = type->size++;
	r->next = type->fields;
	type->fields = r;
	return r;
}

cmld_field *cmld_enumerate_fields(cmld_type *type) {
	return type->fields;
}

cmld_field *cmld_next_field(cmld_field *f) {
	return f->next;
}

const char *cmld_type_name(cmld_type *type) {
	return type ? type->name : 0;
}

const char *cmld_field_name(cmld_field *field) {
	return field ? field->name : 0;
}


cmld_type *cmld_get_type(cmld_var *s) {
	return s && s->type == CMLD_STRUCT ? (cmld_type*)(((size_t)s->struct_val->type) & ~1) : 0;
}

cmld_var *cmld_peek_field(cmld_var *s, cmld_field *field) {
	return s && s->type == CMLD_STRUCT && s->struct_val->fields.size > field->index ?
		s->struct_val->fields.items + field->index :
		0;
}

cmld_var *cmld_get_field(cmld_var *s, cmld_field *field) {
	if (!s || s->type != CMLD_STRUCT)
		return 0;
	if (s->struct_val->fields.size != s->struct_val->type->size)
		array_insert(
			&s->struct_val->fields,
			s->struct_val->type->dom,
			s->struct_val->fields.size,
			field->index - s->struct_val->fields.size + 1);
	return s->struct_val->fields.items + field->index;
}

cmld_var *cmld_at(cmld_var *s, int index) {
	return s && s->type == CMLD_ARRAY && s->array_val->size < index ?
		s->array_val->items + index :
		0;
}

int cmld_get_count(cmld_var *s) {
	return s && s->type == CMLD_ARRAY ? s->array_val->size : 0;
}

long long cmld_as_int(cmld_var *v, long long def_val) {
	return v && v->type == CMLD_INT ? v->int_val : def_val;
}

const char *cmld_as_str(cmld_var *v, const char *def_val) {
	return v && v->type == CMLD_STR ? v->str_val : def_val;
}

void cmld_set_int(cmld_var *dst, long long val) {
	if (dst) {
		dst->type = CMLD_INT;
		dst->int_val = val;
	} 
}

void cmld_set_str(cmld_var *dst, cmld_dom *dom, const char *val) {
	if (dst) {
		dst->type = CMLD_STR;
		dst->str_val = make_str(dom, val);
	}
}

cmld_var *cmld_set_array(cmld_var *dst, cmld_dom *dom, int size) {
	if (dst) {
		dst->type = CMLD_ARRAY;
		dst->array_val = (cmld_array*) cmld_alloc(dom, sizeof(cmld_array));
		init_array(dst->array_val);
		if (size > 0)
			array_insert(dst->array_val, dom, 0, size);
	}
	return dst;
}

cmld_var *cmld_set_struct(cmld_var *dst, cmld_type *type) {
	if (dst) {
		dst->type = CMLD_STRUCT;
		dst->struct_val = (cmld_struct*) cmld_alloc(type->dom, sizeof(cmld_struct));
		dst->struct_val->name = 0;
		dst->struct_val->type = type;
		init_array(&dst->struct_val->fields);
	}
	return dst;
}

cmld_var *cmld_set_ref(cmld_var *dst, void *src_id) {
	if (dst) {
		dst->type = CMLD_STRUCT;
		dst->struct_val = (cmld_struct*) src_id;
	}
	return dst;
}

void *cmld_get_id(cmld_var *s) {
	return s && s->type == CMLD_STRUCT ? s->struct_val : 0;
}

void cmld_tag(cmld_var *s) {
	if (s && s->type == CMLD_STRUCT)
		*(size_t*)s->struct_val->type |= 1;
}
void cmld_untag(cmld_var *s) {
	if (s && s->type == CMLD_STRUCT)
		*(size_t*)s->struct_val->type &= ~1;
}

int cmld_is_tagged(cmld_var *s) {
	return s && s->type == CMLD_STRUCT ? *(size_t*)s->struct_val->type & 1 : 0;
}

void cmld_undefine(cmld_var *v) {
	if (v)
		v->type = CMLD_UNDEFINED;
}

int cmld_kind(cmld_var *v) {
	return v ? v->type : CMLD_UNDEFINED;
}

cmld_var *cmld_root(cmld_dom *dom) {
	return &dom->root;
}

static int get_named_index(cmld_dom *dom, const char *name) {
	int i = dom->named.size;
	while (--i >= 0)
		if (strcmp(dom->named.items[i].struct_val->name, name) == 0)
			return i;
	return -1;
}

cmld_var *cmld_get_named(cmld_dom *dom, const char *name) {
	int i = get_named_index(dom, name);
	return i < 0 ? 0 : &dom->named.items[i];
}

const char *cmld_get_name(cmld_var *target) {
	return target && target->type == CMLD_STRUCT ? target->struct_val->name : 0;
}
void cmld_set_name(cmld_var *struc, const char *name) {
	if (struc && struc->type == CMLD_STRUCT) {
		cmld_struct* target = struc->struct_val;
		cmld_dom *dom = target->type->dom;
		int my_prev_name = target->name ? get_named_index(target->type->dom, target->name) : -1;
		int old_name_bind = get_named_index(dom, name);
		if (my_prev_name < 0) {
			if (old_name_bind < 0) {
				int i = array_insert(&dom->named, dom, dom->named.size, 1);
				dom->named.items[i].type = CMLD_STRUCT;
				dom->named.items[i].struct_val = target;
				target->name = make_str(dom, name);
			} else {
				cmld_struct *old = dom->named.items[old_name_bind].struct_val;
				target->name = old->name;
				old->name = 0;
				dom->named.items[old_name_bind].struct_val = target;
			}
		} else {
			if (old_name_bind < 0) {
				target->name = make_str(dom, name);
			} else {
				cmld_struct *old = dom->named.items[old_name_bind].struct_val;
				if (old != target) {
					target->name = old->name;
					old->name = 0;
					array_delete(&dom->named, dom, old_name_bind, 1);
				}
			}
		}
	}
}

int cmld_insert(cmld_var *arr, cmld_dom *dom, int at, int count) {
	return arr && arr->type == CMLD_ARRAY ? array_insert(arr->array_val, dom, at, count) : -1;
}
void cmld_delete(cmld_var *arr, cmld_dom *dom, int at, int count) {
	if (arr && arr->type == CMLD_ARRAY)
		array_delete(arr->array_val, dom, at, count);
}

void cmld_tag(cmld_var *struc);
void cmld_untag(cmld_var *struc);
int cmld_is_tagged(cmld_var *struc);


static void mark(cmld_var *i) {
	if (!i)
		return;
	switch (i->type) {
	case CMLD_STR:
		((size_t*)i->str_val)[-1] |= 1;
		break;
	case CMLD_ARRAY:
		if ((((size_t*)i->array_val)[-1] & 1) == 0) {
			int cnt = i->array_val->size + 1;
			((size_t*)i->array_val)[-1] |= 1;
			for (i = i->array_val->items; --cnt; i++)
				mark(i);
		}
		break;
	case CMLD_STRUCT:
		cmld_gc_mark(i->struct_val);
		break;
	default:
		break;
	}
}

void cmld_gc_mark(void *struct_id) {
	cmld_struct *s = (cmld_struct*) struct_id;
	if ((((size_t*)s)[-1] & 1) == 0) {
		((size_t*)s)[-1] |= 1;
		if (s->name) {
			cmld_dom *dom = s->type->dom;
			dom->named.items[dom->named.size++].struct_val = s;
			((size_t*)s->name)[-1] |= 1;
		}
		{
			cmld_var *i = s->fields.items;
			int cnt = s->fields.size + 1;
			for (; --cnt; i++)
				mark(i);
		}
	}
}

void cmld_gc(cmld_dom *dom, void (*marker)(void*context), void *marker_context)
{
	dom->named.size = 0;
	mark(&dom->root);
	if (marker)
		marker(marker_context);
	{
		size_t **p = (size_t**) &dom->allocated;
		for (;;) {
			size_t *c = *p;
			if (!*c)
				break;
			if (*c & 1) {
				*c &= ~1;
				p = (size_t**) *p;
			} else {
				*p = (size_t*) *c;
				free(c);
			}
		}
	}
}
