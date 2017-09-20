#ifndef _CML_DOM_H_
#define _CML_DOM_H_

//
// ------------------ DOM manipulation routines -------------
//

//
// cmld_dom
// Represents Document Object Model been read or to be written to CML file.
// Contains
// - a root object,
// - a directory of named objects and,
// - type information describing structured types and its fields
//
typedef struct cmld_dom_tag cmld_dom;

//
// Create a new DOM instance.
// Ater been used it has to be disposed with cmld_free_dom call.
//
cmld_dom *cmld_alloc_dom();

//
// Destroys DOM and all its objects and type information.
//
void cmld_free_dom(cmld_dom *dom);

//
// ------------------ Methadata manipulation routines -------------
//

//
// cmld_type deines structured data types.
// Each type has a name and a list of named fields.
// Types defined in different DOMs event having the same name
// are represented by different cmld_type structures.
//
typedef struct cmld_type_tag cmld_type;

//
// Returns an existing cmld_type by name, or NULL if none.
//
cmld_type *cmld_lookup_type(cmld_dom *dom, const char *name);

//
// Returns the existing type or creates a new one.
// There is no way to remove type from DOM.
//
cmld_type *cmld_add_type(cmld_dom *dom, const char *name);

//
// Returns a type name.
// Pointer is valid till cmld_free_dom called.
//
const char *cmld_type_name(cmld_type *type);

//
// A cmld_field defines a named field of given type.
// Fields of different types even having the same name are represented
// by different cmld_field structures.
//
typedef struct cmld_field_tag cmld_field;

//
// Returns the existing field of given type, or NULL if no one.
//
cmld_field *cmld_lookup_field(cmld_type *type, const char *field_name);

//
// Returns the existing field having this name or creates a new one.
// There is no way to remove fields 
//
cmld_field *cmld_add_field(cmld_type *type, const char *field_name);

//
// Returns the field's name.
// Pointer is valid untill cmld_free_dom called.
//
const char *cmld_field_name(cmld_field *field);

//
// Iterating all fields of given type:
// for (cmld_field *f = cmld_enumerate_fields(type); f; f = cmld_next_field(f))...
// You should not add fields during iterations, because it can alter the iteration sequence.
// The order of fields in type is undefined.
//
cmld_field *cmld_enumerate_fields(cmld_type *type);

//
// A single step of iteration process. See cmld_enumerate_fields or details.
// Returns NULL on end of list.
//
cmld_field *cmld_next_field(cmld_field *);

//
// ------------------ Data Manipulation Routines -------------
//

//
// cmld_var represents all types of data nodes.
// It can store integer, string, array, structure or be in an undefined state.
//
typedef struct cmld_var_tag cmld_var;

//
// Returns the root data node of given dom.
//
cmld_var *cmld_root(cmld_dom *dom);

enum cmld_kinds {
	CMLD_UNDEFINED,
	CMLD_INT,
	CMLD_STR,
	CMLD_STRUCT,
	CMLD_ARRAY,
};

//
// Returns one of cmld_kinds values
// defining the data stored in the data node.
//
int cmld_kind(cmld_var *v);

//
// Makes cmld_var CMLD_UNDEFINED
//
void cmld_undefine(cmld_var *v);

//
// Makes this node CMLD_INT and sets its int64_t value.
//
void cmld_set_int(cmld_var *dst, long long val);

//
// Returns int64_t value stored in data node
// or def_val if this node is not a CMLD_INT.
//
long long cmld_as_int(cmld_var *src, long long def_val);

//
// Sets node type to CMLD_STR and stores text in it.
// The text is stored as copy in newly allocated buffer.
//
void cmld_set_str(cmld_var *dst, cmld_dom *dom, const char *val);

//
// Returns a string value of given node if it is CMLD_STR or
// def_val otherwise.
// Returned pointer is valid till cmld_free_dom or cmld_gc call.
//
const char *cmld_as_str(cmld_var *src, const char *def_val);


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
cmld_var *cmld_set_array(cmld_var *dst, cmld_dom *dom, int size);

//
// Returns array size for CMLD_ARRAY nodes or 0 otherwise.
//
int cmld_get_count(cmld_var *array);

//
// Returns the indexed array item. Or NULL for non-arrays.
//
cmld_var *cmld_at(cmld_var *array, int index);

//
// Resizes array, adding 'count' items starting at 'at' index.
// If reallocation takes place it is registered in given dom.
// Array grows uses reserved gap, thus not all inserts causes reallocations.
// cmld_insert invalidates all cmld_var pointers to array items.
// 'at' index should be within the range of array indexes.
//
int cmld_insert(cmld_var *array, cmld_dom *dom, int at, int count);

//
// Deletes 'count' items starting at 'at' index.
// Invalidates all cmld_var pointers to array items.
// 'at' and 'at+count' indexes should be in the range of array indexes.
//
void cmld_delete(cmld_var *array, cmld_dom *dom, int at, int count);

//
// --------------- Data Manipulation Routines for Structures ----------
//

//
// Creates a new struct and stores it in given node.
//
cmld_var *cmld_set_struct(cmld_var *dst, cmld_type *type);

//
// Returns the structure type descriptor.
//
cmld_type *cmld_get_type(cmld_var *struc);

//
// Returns the structure field. If no field in structure, returns NULL.
// Useful for data acquistion: cmld_as_int(cmld_peek_field(my_struct, my_field), -1)
// This example returns -1 if:
// - my_struct is null
// - or not a structure
// - or does not contain field my_field
// - or if its field is not integer
// - or field contains -1.
// The cmld_peek_field can't be used to store data,
// because cmld_set_int does nothing for NULL pointer.
// This code won't work cmld_set_int(cmld_peek_field(my_struct, my_field), 42);
//
cmld_var *cmld_peek_field(cmld_var *struc, cmld_field *field);

//
// Returns the structure field.
// If there are fields added to structure type since this instance created,
// reallocates one, making sure the the fields presented.
// If reallocated, invalidates all cmld_var referencing this structure fields.
// Useful for data storing: cmld_set_int(cmld_get_field(my_struct, my_field), 42);
//
cmld_var *cmld_get_field(cmld_var *struc, cmld_field *field);

//
// Returns structure id, that persists all struct lifetime.
// Can be used
// - to check against null reference
// - to check if two cmld_vars reference the same structures.
// - to check against maps and sets of structures for data traversing etc.
//
void *cmld_get_id(cmld_var *struc);

//
// Makes dst referencing the structure with given id.
// This allows arbitrary data topology beyond the tree-like structures.
// DOM and CML can deal event with structures having cycles.
//
cmld_var *cmld_set_ref(cmld_var *dst, void *src_id);

//
// Sets one-bit flag on structure.
// This is useful for detecting cycles in graph traversing.
//
void cmld_tag(cmld_var *struc);

//
// Clears one-bit flag on structure
//
void cmld_untag(cmld_var *struc);

//
// Checks if one-bit flag is set on structure
//
int cmld_is_tagged(cmld_var *struc);

//
// --------------------------- Name Manipulation Routines -----------------
//

//
// Makes struct named.
// If given struct had dad a name, old name is removed.
// If this name had been assigned to a different struct, it becomes unnamed. 
//
void cmld_set_name(cmld_var *target, const char *name);

//
// Get struct by name.
//
cmld_var *cmld_get_named(cmld_dom *dom, const char *name);

//
// Get struct name.
//
const char *cmld_get_name(cmld_var *target);

//
// ------------------------ Garbage Collector
//

//
// Checks all allocaion made for given DOM,
// and reclaims all memory inaccessible from series of references starting at root node.
// It invalidates any cmld_vars stored outside dom.
// It never reclaims type and field descriptors.
// The 'marker' parameter (if not null) is a call-back function
// that can cmld_gc_mark any additional structures as gc roots.
//
void cmld_gc(
	cmld_dom *dom,
	void (*marker)(void*context),
	void *marker_context);

//
// Marks the given struct as additional root in gc cycle.
// Can't be called outside the gc cmld_gc marker punction.
//
void cmld_gc_mark(void *struct_id);

#endif
