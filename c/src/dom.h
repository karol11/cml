#ifndef _CML_DOM_H_
#define _CML_DOM_H_

#ifdef __cplusplus
extern "C" {
#endif
//
// ------------------ DOM manipulation routines -------------
//

//
// d_dom
// Represents Document Object Model been read from or to be written to CML file.
// Contains
// - a root object,
// - a directory of named objects,
// - and the type information describing structure types and their fields
//
typedef struct d_dom_tag d_dom;

//
// Create a new DOM instance.
// Ater been used it has to be disposed with d_dispose_dom call.
//
d_dom *d_alloc_dom();

//
// Destroys DOM and all its objects and type information.
//
void d_dispose_dom(d_dom *dom);

//
// ------------------ Metadata manipulation routines -------------
//

//
// 'd_type' defines structure data types.
// Each type has a name and a list of named fields.
// Types of the same name defined in different DOMs
// are represented by different d_type structures.
//
typedef struct d_type_tag d_type;

//
// Returns an existing d_type by name, or NULL if none.
//
d_type *d_lookup_type(d_dom *dom, const char *name);

//
// Returns the existing type or creates a new one.
// There is no way to remove type from DOM.
//
d_type *d_add_type(d_dom *dom, const char *name);

//
// Returns the type name.
// Result pointer is valid till d_dispose_dom called.
//
const char *d_type_name(d_type *type);

//
// A d_field defines a named field of given type.
// Fields of different types even having the same name are represented
// by different d_field structures.
//
typedef struct d_field_tag d_field;

//
// Returns the existing field of given type, or NULL if no one.
//
d_field *d_lookup_field(d_type *type, const char *field_name);

//
// Returns the existing field having this name or creates a new one.
// There is no way to remove fields from type.
//
d_field *d_add_field(d_type *type, const char *field_name);

//
// Returns the field's name.
// Pointer is valid till d_dispose_dom called.
//
const char *d_field_name(d_field *field);

//
// Iterating all fields of given type:
// for (d_field *f = d_enumerate_fields(type); f; f = d_next_field(f))...
// You should not add fields during iterations, because it can alter the iteration sequence.
// The order of fields in a type is unspecified.
//
d_field *d_enumerate_fields(d_type *type);

//
// A single step of iteration process. See d_enumerate_fields for details.
// Returns NULL on the end of the list.
//
d_field *d_next_field(d_field *);

//
// ------------------ Data Manipulation Routines -------------
//

//
// 'd_var' represents all types of data nodes.
// It can store integer, string, array, structure or can be in an undefined state.
// All data manipulation routines treat NULL d_var pointers as having CMLD_UNDEFINED value.
//
typedef struct d_var_tag d_var;

//
// Returns the root data node of given dom.
//
d_var *d_root(d_dom *dom);

//
// Returns the node content kind - one of d_kinds values.
//
int d_kind(d_var *v);

enum d_kinds {
	CMLD_UNDEFINED,
	CMLD_INT,
	CMLD_BOOL,
	CMLD_STR,
	CMLD_STRUCT,
	CMLD_ARRAY,
	CMLD_DOUBLE,
	CMLD_BINARY,
};

//
// Makes d_var UNDEFINED
//
void d_undefine(d_var *v);

//
// Moves src node to dst.
// Old dst value discarded.
// Src node becomes undefined.
//
void d_move(d_var *dst, d_var *src);

//
// Makes this node INT and sets its int64_t value.
//
void d_set_int(d_var *dst, long long val);

//
// Returns int64_t value stored in data node
// or def_val if this node is not a CMLD_INT.
//
long long d_as_int(d_var *src, long long def_val);

//
// Makes this node BOOL and sets its value.
//
void d_set_bool(d_var *dst, int val);

//
// Returns bool value stored in data node
// or def_val if this node is not a CMLD_BOOL.
//
int d_as_bool(d_var *src, int def_val);

typedef struct d_str_tag d_str;
d_str *d_make_str(d_dom *dom, const char *val);

//
// Sets node type to CMLD_STR and stores text in it.
// The text is stored as copy in newly allocated buffer.
//
void d_set_str_ref(d_var *dst, d_str *ref);

static void d_set_str(d_var *dst, d_dom *dom, const char *val) {
	d_set_str_ref(dst, d_make_str(dom, val));
}

//
// Returns a string value of given node if it is CMLD_STR or
// def_val otherwise.
// Returned pointer is valid till d_dispose_dom or d_gc call.
//
d_str *d_as_str_ref(d_var *src);

static const char *d_as_str(d_var *src, const char *def_val) {
	d_str *r = d_as_str_ref(src);
	return r ? (const char*) r : def_val;
}

char *d_as_binary(d_var *src, int *out_size);
void d_set_binary(d_var *dst, d_dom *dom, char *data, int size);

//
// --------------- Data Manipulation Routines for Arrays ----------
//

//
// Makes given data node an array and optionally sets its size.
// dst - node to be assigned the array,
// dom - the dom, this node belongs to,
// size - initial array size.
// If the size is non-zero, the new array is filled with CMLD_UNDEFINED nodes.
// Returns dst.
//
d_var *d_set_array(d_var *dst, d_dom *dom, int size);

//
// Returns array size for CMLD_ARRAY nodes or 0 otherwise.
//
int d_get_count(d_var *array);

//
// Returns the indexed array item. Or NULL for non-arrays.
//
d_var *d_at(d_var *array, int index);

//
// Resizes array, adding 'count' items starting at 'at' index.
// If reallocation takes place, it is registered in given dom.
// Array grows using a reserved gap, thus not all inserts causes reallocations.
// d_insert invalidates all d_var pointers to array items.
// 'at' index should be within the range of array indexes.
//
int d_insert(d_var *array, d_dom *dom, int at, int count);

//
// Deletes 'count' items starting at 'at' index.
// Invalidates all d_var pointers to array items.
// 'at' and 'at+count' indexes should be in the range of array indexes.
//
void d_delete(d_var *array, d_dom *dom, int at, int count);


//
// --------------- Data Manipulation Routines for Structures ----------
//
typedef struct d_struct_tag d_struct;
d_struct *d_make_struct(d_type *type);

//
// Returns structure reference, that remains the same during this struct lifetime.
// It can be used:
// - to check against null reference
// - to check if two d_vars reference the same structure.
// - to check against maps<> and sets<> of structures for graph traversing etc.
// Garbage collection kills unused structures making refs invalid.
// See d_gc for details.
//
d_struct *d_get_ref(d_var *struc);

//
// Makes dst referencing the structure with given id.
// This allows arbitrary data topology beyond the tree-like structures.
// The DOM and the CML format can deal with any topology, even having cycles
// in reference graph.
//
d_var *d_set_ref(d_var *dst, d_struct *src);

//
// Creates a new struct and stores it in given node.
//
static d_var *d_set_struct(d_var *dst, d_type *type) {
	return d_set_ref(dst, d_make_struct(type));
}

//
// Returns the struct type descriptor.
//
d_type *d_ref_get_type(d_struct *struc);

static d_type *d_get_type(d_var *struc) {
	return d_ref_get_type(d_get_ref(struc));
}

//
// Returns the structure field.
// If there are fields added to structure type since given instance has been created,
// peeking such new fields returns NULL.
// This is OK for data acquistion: d_as_int(d_peek_field(my_struct, my_field), -1)
// This example returns -1 if:
// - my_struct is null
// - or not a structure
// - or does not contain field my_field
// - or if its field is not integer
// - or field contains -1.
// The d_peek_field can't be used to store data,
// because d_set_int does nothing for NULL pointer.
// This code has no effect: d_set_int(d_peek_field(my_struct, my_field), 42);
// For storing data use d_get_field instead.
//
d_var *d_ref_peek_field(d_struct *struc, d_field *field);

static d_var *d_peek_field(d_var *struc, d_field *field) {
	return d_ref_peek_field(d_get_ref(struc), field);
}

//
// Returns the structure field.
// If there are fields added to structure type since given instance created,
// the d_get_field will reallocate one, making sure all fields exist.
// Reallocation invalidates all d_var referencing this structure fields.
// The 'd_get_field' is useful for data storing:
// d_set_int(d_get_field(my_struct, my_field), 42);
//
d_var *d_ref_get_field(d_struct *struc, d_field *field);

static d_var *d_get_field(d_var *struc, d_field *field) {
	return d_ref_get_field(d_get_ref(struc), field);
}

//
// Sets tag on structure.
// This is useful for detecting cycles in graph traversing.
//
void d_set_ref_tag(d_struct *struc, size_t tag);

static void d_set_tag(d_var *struc, size_t tag) {
	d_set_ref_tag(d_get_ref(struc), tag);
}

//
// Gets tag.
//
size_t d_get_ref_tag(d_struct *struc);

static size_t d_get_tag(d_var *struc) {
	return d_get_ref_tag(d_get_ref(struc));
}

//
// Remove all tags tracing DOM starting at given d_var.
//
void d_untag(d_var *v);
void d_untag_ref(d_struct *s);

//
// --------------------------- Name Manipulation Routines -----------------
//

//
// Makes struct named.
// If given struct had dad a name, the old name is removed.
// If this name had been assigned to a different struct, it becomes unnamed.
//
void d_set_ref_name(d_struct *target, const char *name);

static void d_set_name(d_var *target, const char *name) {
	d_set_ref_name(d_get_ref(target), name);
}

//
// Get struct by name.
//
d_struct *d_get_named(d_dom *dom, const char *name);

//
// Get struct name.
//
const char *d_get_ref_name(d_struct *target);

static const char *d_get_name(d_var *target) {
	return d_get_ref_name(d_get_ref(target));
}

//
// ------------------------ Garbage Collector
//

//
// Checks all allocaion made for given DOM,
// and reclaims all memory inaccessible from series of references starting at root node.
// It invalidates any d_vars stored outside dom.
// It never reclaims type and field descriptors.
// The 'marker' parameter (if not null) is a call-back function
// that can d_gc_mark any additional structures as gc roots
// (to save needed structures that temporarily went out of root object hierarchy).
// The 'on_dispose' parameter (if not null) is called each time
// the GC tries to delete a structure. It can handle its id invalidation.
// (to handle the invalidation of ids of structures that are been deleted).
//
void d_gc(
	d_dom *dom,
	void (*marker)(void*context),
	void *marker_context,
	void (*on_dispose)(void *context, void *id),
	void *on_dispose_context
	);

//
// Marks the given struct as additional root in gc cycle.
// Can't be called outside the gc d_gc marker function.
//
void d_gc_mark(void *struct_id);


#ifdef CONFIG_LIBC_FLOATINGPOINT

double d_as_double(d_var *src, double def_val);
void d_set_double(d_var *dst, double val);

#endif

#ifdef __cplusplus
}
#endif

#endif
