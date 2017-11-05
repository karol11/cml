## How to use CML in C

CML C library contents:

- cml_stax_writer - writes CML file in streaming STAX mode directly from application data structures.
- cml_stax_reader - parses CML file in streaming mode, allowing to directly create and fill-in application data structures.
- DOM (Document Object Model) - supports versatile application-independent data structures that can hold the content of parsed CML file.
Using DOM is the easiest way of reading and writing CML files.
- cml_dom_reader - single-call function loading DOM from any streaming sources.
- cml_dom_writer - single-call function creating CML from DOM.
- string_builder - in-memory string buffer that can be used as sink for cml_*_writer, and it is used internally in cml_stax_reader.
- utf8 - utf32<->utf8 transcoders, used internally in reader and writer.


## Streaming

CML library uses pointers to getc and putc functions along with context for these functions to pull and push characters to- and from streams. This allows using arbitrary data streams.
In below examples fopen/getc is used to obtain data. Other alternatives explained here (TBD).

## Source code

All CML-related things is always presented in the form of source code (not in obj/lib form).
I beleive anyone has to have ability to inspect, debug and tailor this code for one projects.

## How to use CML as config file
-----------------------------

1. Add to your project:
	- cml_stax_reader.c
	- dom.c
	- cml_dom_reader.c
	- string_builder.c
	- utf8.c
2. define `d_dom *config;` as global variable (or where you'll need access to you config).
3. At start open file and read its content to DOM:
	```C++
	FILE *f = fopen("config.cml", "r");
	config = cml_read(getc, f, 0, 0);
	fclose(f);
	```
4. When configuration parameters are needed, read their values:\
	Let's imagine, we have config storing the array of strings:
	```
	:
	  "apples"
	  "oranges"
	  "pears"
	```
	It can be accessed using this code:

	```c++
	d_var *list = d_root(config);
	for (int i = 0; i < d_get_count(list); i++)
		printf("[%d] = %s\n", i, d_as_str(d_at(list, i), ""));

	```
	Where:
	- d_var* - is a pointer to any CML DOM node - a root node, array item, struct field etc.
	- d_root - returns the root node of given DOM
	- d_get_count - returns elements count of an array node (or 0 for all other nodes, so it's safe)
	- d_at returns i-th element of the array node (or an undefined node otherwise, so it's also safe)
	- d_as_str returns char* to internal text buffer of given string node (or its second parameter if it is not a string node).
	So despite its simplicity this code handles all unexpected CML format conditions.

5. PROFIT, we did it using just 6 lines of code including fopen and fclose!

What if config file contains other data types?
- `d_as_int`, `d_as_bool`, `d_as_double` - return other primitive data.
- `d_as_binary` returns binary values as buffer ptr and size.
- `d_peek_field` - returns a structure field value.

## Error handling

1. Check `fopen` for errors, because libc `getc` doesn't do it for you.
2. Pass `on_error` function and its context as last two parameters of `cml_read` and/or check its result for NULL.
	```C++
	void on_error(void *context, const char *error, int line_num, int char_pos) {
		printf("error %s at %s %d:%d\n", error, context, line_num, char_pos);
		exit(-1);
	}
	...
	config = cml_read(getc, f, on_error, "config.cml");
	```
3. Althought it is not needed, you can explicitly check all `d_var*` for NULL and/or use `d_kind(var)` to check for real data types.

## Config having structures

This time our config will be looking like this:
```
Cloud
description "Controller + N Compute Topology - x86 KVM"
password "MyCloudPassword"
database_service_type Db2
messaging_service_type Rabbitmq
nodes:
  Controller.mainCtl
  description "Cloud controller node"
  fqdn "controllername.company.com"

  Kvm_compute
  description "Cloud KVM compute node"
  fqdn "kvmcomputename.company.com"
  identity_file "/root/identity.pem"
```
Now it contains structures having fields.
It can be accessed this way.
```C++
config = cml_read(getc, f, 0, 0);
d_type *cloud_t = d_lookup_type(config, "Cloud");
d_field *cloud_pass = d_lookup_field(cloud_t, "password");
char *pass = d_as_str(d_peek_field(d_root(config), cloud_pass), "");
```
Where:
- `d_lookup_type` - returns the `Cloud` structure descriptor contained in  the loaded DOM (or NULL if no one).
- `d_lookup_field` - returns the descriptor of `password` field of `Cloud` structure (or NULL if no such type or field).
- `d_peek_field` - returns field value of given structure object.

The last expression of this example checks:
- if the root node holds a structure,
- if this structure is of type `Cloud`
- if it has a field `password`
- if this field holds data of string type.
If any condition fails, `pass` will point to `def_val` parameter of `d_as_str` which  is "" in this example. 

## Access to named objects
In the above example there is a named object `mainCtl`.

Your application can directly access objects by names:
```C++
d_var *mainCtl_fqdn = d_ref_peek_field(
	d_get_named(config, "mainCtl"),
	d_lookup_field(d_lookup_type(config, "Controller"), "fqdn"));
```
Where:
- `d_get_named` returns direct pointer to named struct as `struct_t`
- `d_ref_peek_field` acts as `d_peek_field` but accepts `struct_t` instead of `var_t`

What is the difference between var_t and struct_t.
- `var_t` - is an assignable, mutable variable (root object, field of structure, array item), that can hold int, bool... and struct pointer.
- `struct_t` - is a struct pointer.
You can always extract struct_t from var_t by `d_get_ref` call.

## Cleanup
If the loaded data is no more needed call `d_dispose_dom(config)`.

## See also
- TBD How to load CML from other sources (memory, std::streams, sockets etc.)
- [How to create and modify DOM and write it to CMLs.](https://github.com/karol11/cml/wiki/How-to-create-DOM-and-write-it-to-CMLs-in-C-and-CPP)
- TBD How to write application data directly to CML in STAX mode bypassing DOM creation.
- TBD How to read CML to application structures withoud DOM.
