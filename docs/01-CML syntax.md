CML lexical rules
-----------------
- CML is a UTF8-encoded text file.
- it can have four line endings \r | \n | \r\n | \n\r
- indentation can be performed with spaces or tabs
- all file have to have identical identation.

BNF
---

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
