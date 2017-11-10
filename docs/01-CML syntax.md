##CML lexical rules
- CML is a UTF8-encoded text file.
- it can have four line endings \r | \n | \r\n | \n\r
- indentation can be performed with spaces or tabs
- all file have to have identical identation.

## Informal definition
- *Integer* - Signed decimal format: -1, 0, 44.
- *Floating point* - Exponential and simple form: 0.1, -10.5e-14.
- *Boolean* - `+` or `-`
- *String* - "Quoted" having \\ \" and \uXXXX escapes.
- *Array* - `:` followed by optional array size and indented array items.
- *Structure* - Type_name followed by optional `.` and instance id and a field list.
   - If structure nested in a field of another structure, fields are indented.
   - If structure is an array item (not the last array item), it ended with an empty line. This increases complexity, but makes file look more intuitive.
- *Cross reference* - `-` followed by object id.
  - Object id == `_` - is a null reference
  - Id starting with `_` are local ids for this CML file, they may not be visible in loaded DOM.
  - All local ids refers to a structure early defined in this CML file.
  - Non-local id can be defined in this file (exports) or can be imported from already existing DOM.
- *Binary data* - `#` followed by mandatory bytes-count and indented data in base-64 format.

## BNF

```BNF
cml_file = : last_array_item
node =
  : '-' | '+'  ; boolean value
  | '-'? number ('.' number)? (('e'|'E') '-'? number)?  ; numeric value
  | '"' (ANY_LETTER | '\"' | '\\' | '\u' HEX_DIGIT{4})* '"'  ; string value
  | '=' id(struct_name)  ; reference value
  | '#' number INDENT ('a'..'z' | 'A'..'Z' | '0'..'9' | '+' | '\' | '=' | NEW_LINE)* OUTDENT ; binary value
  | ':' (number(array_size))? (INDENT array_item (NEW_LINE array_item)* NEW_LINE last_array_item)? ; array value
field_value =
  : node
  | id(struct_type_name) ('.' id(struct_name))? (INDENT id(field_name) field_value (NEW_LINE id(field_name) field_value)*? OUTDENT
array_item =
  : node
  | id(struct_type_name) ('.' id(struct_name))? (NEW_LINE id(field_name) field_value)* NEW_LINE
last_array_item
  : node
  | id(struct_type_name) ('.' id(struct_name))? (NEW_LINE id(field_name) field_value)*
number = : ('0'..'9')+
first_id_letter = : 'a'..'z' | 'A'..'Z' | '_' | '\' | '/'
id = : first_id_letter (first_id_letter | '0'..'9')*
```

Special names
-------------

- '_' - null reference
- names started with '_' - local names, that won't be visible outside CML file.
- all other names are accessible with DOM naming.
