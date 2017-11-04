## Using CML in JavaScript

Add CML support
```HTML
<script src="cml.js"></script>
```

## Parse CML

Example
```Javascript
var domRoot = cml.parse(t, "type");
```

Formal definition
```
cml.parse(string_source, factory, name_map)
```
Where
- `source` - string containig data in CML format.
- `factory`
  - If function, it is called for each structure. It receives type name and returns object instance.
  - If string, a object `{}` is created, and `factory` string becomes the field name storing type.
- `name_map`
  - If Map, it is used to resolve external names, and filled with new names.
  - If undefined, no names exported and imported.
  
Returns data:

 CML type | JavaScript type
--- | ---
int, doble | number
boolean | boolean
'=' ref | reference to `{}` object
"string" | "string"
`:` array | `[]` array
`#` binary | ArrayBuffer
Struct | resulrt of `factory` function or `{}` Object having `factory` field

Unlike JSON.parse function cml.parse can return non-tree data structure. Because CML file can contain cross references.

On parsing errors throws
```
{
  "type": "error",
  "message": msg,
  "line": lineNumber,
  "pos": charPos
}
```

## Creating CML

Example
```Javascript
var cml = cml.stringify(domRoot, "type");
```

Formal definition
```
cml.stringify(rootObject, typer, name_map)
```
Where
- `rootObject` - Root object to be written to CML.
- `typer`
  - If function, it is called for each structure. It receives object to be written and returns its type name.
  - If string, it defines a field name storing type (this field itself gets excluded from serialization).
- `name_map`
  - If Map, it is used to resolve external names, and filled with new names.
  - If undefined, no names exported and imported.

Returns string with CML encoded data.

## Examples
cml/javascript/func_test_index.html
