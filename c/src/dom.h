#ifndef _CML_DOM_H_
#define _CML_DOM_H_

typedef struct cml_dom_tag cml_dom;
typedef struct cml_type_tag cml_type;
typedef struct cml_field_tag cml_field;
typedef struct cml_variant_tag cml_variant;

cml_dom *cml_alloc_dom();
void cml_free_dom(cml_dom *dom);
void cml_gc(
	cml_dom *dom,
	void (*marker)(void*context),
	void *marker_context);

void cml_set_name(cml_variant *target, const char *name);
cml_variant *cml_get_named(cml_dom *dom, const char *name);
const char *cml_get_name(cml_variant *target);
cml_variant *cml_root(cml_dom *dom);

cml_type *cml_lookup_type(cml_dom *dom, const char *name);
cml_type *cml_add_type(cml_dom *dom, const char *name);
cml_field *cml_lookup_field(cml_type *type, const char *field_name);
cml_field *cml_add_field(cml_type *type, const char *field_name);
cml_field *cml_enumerate_fields(cml_type *type);
cml_field *cml_next_field(cml_field *);
const char *cml_type_name(cml_type *type);
const char *cml_field_name(cml_field *field);

cml_type *cml_get_type(cml_variant *src_struct);
cml_variant *cml_peek_field(cml_variant *src_struct, cml_field *field);
cml_variant *cml_get_field(cml_variant *src_struct, cml_field *field);

cml_variant *cml_get_at(cml_variant *array, int index);
int cml_get_count(cml_variant *array);
int cml_insert(cml_variant *array, cml_dom *dom, int at, int count);
void cml_delete(cml_variant *array, cml_dom *dom, int at, int count);

long long cml_as_int(cml_variant *src, long long def_val);
const char *cml_as_str(cml_variant *src, const char *def_val);

void cml_set_int(cml_variant *dst, long long val);
void cml_set_str(cml_variant *dst, cml_dom *dom, const char *val);
cml_variant *cml_set_array(cml_variant *dst, cml_dom *dom, int size);
cml_variant *cml_set_struct(cml_variant *dst, cml_type *type);
cml_variant *cml_set_ref(cml_variant *dst, cml_variant *src);
void *cml_get_id(cml_variant *struc);

enum cml_variant_kinds {
	CML_V_UNDEFINED,
	CML_V_INT,
	CML_V_STR,
	CML_V_STRUCT,
	CML_V_ARRAY,
};

int cml_var_kind(cml_variant *v);


#endif
