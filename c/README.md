## How to use CML in C
In C++ you can use [more convenient API](https://github.com/karol11/cml/wiki/Using-CML-DOM-in-CPP)

CML C library contents:

- cml_stax_writer - Writes CML file in streaming STAX mode directly from application data structures.
- cml_stax_reader - Parses CML file in streaming mode, allows to directly create and fill application data structures.
- DOM (Document Object Model) - Supports versatile application-independent data structure that holds the content of the parsed CML file.

DOM reader/writer is the easiest way to work with CML files.
- cml_dom_reader - A single-call function that loads DOM from any streaming sources.
- cml_dom_writer - A single-call function that creates CML file from DOM.

Auxillary modules:
- string_builder - An in-memory string buffer that can be used as sink for cml_*_writer, and it is used internally in cml_stax_reader.
- utf8 - A utf32<->utf8 transcoders, that are used by reader and writer code.

## Streaming

CML library uses pointers to functions along with context for these functions to pull and push characters to- and from streams. This allows using arbitrary data streams (in memory/sockets/files etc.)
In the examples below we'll be using `fopen`/`getc`. For other alternatives see examples in `tests` directory.

## Source code

All CML-related things is always presented in the form of source code (not in obj/lib form).
I beleive anyone has to have ability to inspect, debug and tailor this code for one's needs.

## How to use CML as config file
-----------------------------

1. Add to your project:
	- cml_stax_reader.c
	- dom.c
	- cml_dom_reader.c
	- string_builder.c
	- utf8.c
2. Define a `d_dom *config;` variable.
3. At program start open file and read its content to DOM:
	```C++
	FILE *f = fopen("config.cml", "r");
	config = cml_read(getc, f, 0, 0);
	fclose(f);
	```
4. Now you can access data stored in this file:\
	Let's imagine, that our config stores the array of strings:
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
	- `d_var*` - A pointer to any CML DOM node: (root, array item, structure field etc.)
	- `d_root(dom)` - A function that returns root node of given DOM.
	- `d_get_count(node)` - A function that returns:
		- the number of elements, if the node is an array,
		- or 0 for all other node types.
	- `d_at(node, index)` - A function that returns i-th element of the array node or the null-node otherwise. It's safe to pass null nodes to any functions.
	- `d_as_str(node, default_value)` - A function that returns:
		- a `char*` to internal text buffer, for string node,
		- or `default_value`, for all other node types.
	Despite its simplicity this code handles all unexpected CML format conditions: Expression `d_as_str(d_at(d_root(dom), 2), "def")` returns "def" if CML is empty or is not an array or array doesn't contain 2-nd item, or array item is not a string.

5. PROFIT! We read CML file and accessed its data with just 6 lines of code including fopen and fclose!

What if config file contains other data types?
- `d_as_int`, `d_as_bool`, `d_as_double` - These functions return other primitive data.
- `d_as_binary` Returns binary values as buffer ptr and size.
- `d_peek_field` - Returns a structure field value.

## Error handling

1. Check `fopen` result for NULL. Because libc `getc` crashes if called with NULL file.
2. Last two paramters of `cml_read` function defines an `on_error` function and its context.
	```C++
	void on_error(void *context, const char *error, int line_num, int char_pos) {
		printf("error %s at %s %d:%d\n", error, context, line_num, char_pos);
		exit(-1);
	}
	...
	config = cml_read(getc, f, on_error, "config.cml");
	```
3. Althought `default_value` parameters allows to write code without error checking, you can explicitly check any `d_var*` for `NULL` or use `d_kind(var)` to check for real data types.

## Config having structures

Let's read a more complex CML file:
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
Now it contains structures with fields.
It can be accessed this way.
```C++
config = cml_read(getc, f, 0, 0);
d_type *cloud_t = d_lookup_type(config, "Cloud");
d_field *cloud_pass = d_lookup_field(cloud_t, "password");
char *pass = d_as_str(d_peek_field(d_root(config), cloud_pass), "");
```
Where:
- `d_lookup_type` - returns the `Cloud` structure descriptor contained in the loaded DOM (or NULL if no one).
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
- `d_get_named` returns the direct pointer (`struct_t*`) to the named structure.
- `d_ref_peek_field` acts as `d_peek_field` but accepts `struct_t*` instead of `var_t*`

What is the difference between `var_t` and `struct_t`.
- `var_t` - is an assignable, mutable variable (root object, field of structure, array item), that can hold int, bool... and struct pointer.
- `struct_t` - is a struct pointer - a value, stored in `var_t`.
You can always extract `struct_t` from `var_t` by `d_get_ref` call.

## Cleanup
If the loaded data is no longer needed, just call `d_dispose_dom(config)`.

## Full example
This code:
```C++
#include "dom.h"
#include "cml_dom_reader.h"
void on_error(void *context, const char *error, int line_num, int char_pos) {
   printf("error %s at %s %d:%d\n", error, context, line_num, char_pos);
}
int main() {
   if (FILE *f = fopen("config.cml", "r")) {
      d_dom *d= cml_read((int(*)(void*))getc, f, on_error, "config.cml");
      fclose(f);
      d_type *conf = d_lookup_type(d, "Config");
      d_var *list = d_peek_field(d_root(d), d_lookup_field(conf, "items"));
      for (int i = 0; i < d_get_count(list); i++)
         printf("[%d] = %s\n", i, d_as_str(d_at(list, i), ""));
      printf("timeout = %d\n", (int)d_as_int(
         d_peek_field(d_root(d), d_lookup_field(conf, "timeout")),
         1800));
      d_dispose_dom(d);
   }
}
```
Will read this config
```
Config
timeout 44
items:
	"Linux"
	"Windows"
	"BeeOS"
```
And produce this output
```
[0] = Linux
[1] = Windows
[2] = BeeOS
timeout = 44
```
## See also
- TBD How to load CML from other sources (memory, std::streams, sockets etc.)
- [How to create DOM and write it to CMLs.](https://github.com/karol11/cml/wiki/How-to-create-DOM-and-write-it-to-CMLs-in-C-and-CPP)
- [How to Read-Modify-Write CMLs](https://github.com/karol11/cml/wiki/How-to-Load-Modify-Write-CML-using-DOM-in-C-and-CPP)
- [How to write application data directly to CML in STAX mode bypassing DOM creation](https://github.com/karol11/cml/wiki/STAX-Writer-in-C-and-CPP).
- TBD How to read CML to application structures withoud DOM.
