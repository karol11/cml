How to use CML in C
-------------------

CML C library contents:

- cml_stax_writer - writes CML file in streaming STAX mode directly from application data structures.
- cml_stax_reader - parses CML file in streaming mode, allowas to directly create and fill-in application data structures.
- DOM (Document Object Model) - supports versatile application-independent data structures that can hold the content of parsed CML file.
Using DOM is the easiest way of reading and writing CML files.
- cml_dom_reader - single-call function loading DOM from any streaming sources.
- cml_dom_writer - single-call function creating CML from DOM.
- string_builder - in-memory string buffer that can be used as sink for cml_*_writer, and it is used internally in cml_stax_reader.
- utf8 - utf32<->utf8 tracscoders, used internally in reader and writer.


Streaming
---------

CML library uses pointers to getc and putc functions along with context for these functions to pull and push. TBD


How to use CML as config file
-----------------------------

1. Add to your project:
    - cml_stax_reader.c
	- dom.c
	- cml_dom_reader.c
	- string_builder.c
	- utf8.c
2. define `d_dom *config;` as global variable (or where you'll need access to you config).
3. At start open file and read its contents to DOM:
```C++
FILE *f = fopen("config.cml", "r");
config = cml_read(getc, f, 0, 0);
fclose(f);
```
4. When configuration parameters are needed, read their values:
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
- d_get_count - returns elements count for array node (or 0 for all other nodes, so it's safe)
- d_at returns i-th element of array node (or an undefined node otherwise, so it's also safe)
- d_as_str returns char* to internal text buffer of given string node (or its second parameter if it is not a string node).
So despite its simplicity this code handles all unexpected CML format conditions.

5. PROFIT, we did it using just 6 lines of code including fopen and fclose!

What if config file contains other data types?
- d_as_int, d_as_bool, d_as_double - return other primitive data.
- d_as_binary returns binary values as buffer ptr and size.
- d_peek_field - returns a structure field value.


Config having structures needs more explanation and a separate example

This time our config have looks like this:
```
ProcessingConfig
src "http://aaa.com/nnn.mp4"
rules:  
  Crop
  left 10
  right -10
  top 20
  bottom -42
  
  Deinterlace
  
  

```

Right after 
TBD