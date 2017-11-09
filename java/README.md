## How to use CML in Java

CML Java library contents:

- CmlStaxWriter - writes CML file in streaming STAX mode directly from application data structures.
- CmlStaxReader - parses CML file in streaming mode, allowing to directly create and fill-in application data structures.
- Dom (Document Object Model) - supports versatile application-independent data structures that can hold the content of parsed CML file.
Using DOM is the easiest way of reading and writing CML files.
- DomQuery - Helper class that makes it easier to access DOM structures.
- CmlDomReader - single-call function loading DOM from any streaming sources.
- CmlDomWriter - single-call function creating CML from DOM.

## Source code

All CML-related things is always presented in the form of source code (not in compiled class form).
I beleive anyone has to have ability to inspect, debug and tailor this code for one's projects.

## How to use CML as config file
-----------------------------

1. Add to your project:
	- CmlStaxReader.java
	- Dom.java
	- CmlDomReader.java
	- DomQuery.java
  - add `static import DomQuery.query;`
2. define `Dom config;` where you'll need access to you config.
3. At start open file and read its content to DOM:
	```Java
	config = CmlDomReader.read(new FileReader("config.cml"));
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

	```Java
	for (DomQuery i: query(config.root))
	    System.out.println(i.asStr(""));
	```
	Where:
	- DomQuery - a wrapper simplifying access to DOM nodes.
	- query - DomQuery constructor function. 
	- config.root - returns the root node of given DOM
	- DomQuery.asStr - extracts string data from node (if node cannot be converted to string returns its defVal parameter).
	So despite its simplicity this code handles all unexpected CML format conditions.

5. PROFIT, we did it using just 3 lines of code.

What if config file contains other data types?

Using `DomQuery`:
- `asInt`, `asLong`, `asFloat`, `asDouble`, `asBool` - return other primitive data.
- `asRaw` returns binary values as buffer ptr and size.
- `field(name)` - returns a structure field value.
- `at(index`, `size()` - access to arrays.

## Accessing DOM without DomQuery
```Java
if (config.root instanceof List<?>)
  for (Object i: (List<?>)config.root)
      System.out.println(i instanceof String ? (String)i : "");
```

## Error handling

Parser throws RuntimeException on syntax errors.

Not very handy, but you can always modify CmlStaxReader.error method...

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
```Java
config = CmlDomReader.read(new FileReader("config.cml"));
String pass = query(config.root).checkType("Cloud").field("password").asStr("");

// Or without DomQuery
String pass = "";
if (config.root instanceof Dom.Struct) {
  Dom.Struct s = (Dom.Struct) config.root;
  if (s.type.name.equals("Cloud")) {
    Object pf = s.get("password");
    if (pf instanceof String)
      pass = (String) pf;
  }
}
```
Where:
- `Dom.Struct` - Data type of CML structures. It contains field map, name and type information.
- `DomQuery.checkType` - checks if CML node is a struct of given type.
- `DomQuery.field` - returns field value.

The last expression `String pass = query(config.root).checkType("Cloud").field("password").asStr("");`
checks:
- if the root node holds a structure,
- if this structure is of type `Cloud`
- if it has a field `password`
- if this field holds data of string type.
If any condition fails, `pass` will point to `defVal` parameter of `asStr` which  is "" in this example. 

## Access to named objects
In the above example there is a named object `mainCtl`.

Your application can directly access objects by names:
```Java
String mainCtlFqdn = query(config.getNamed("mainCtl")).field("fqdn").asStr("");
```
Where `getNamed` returns named struct.

## The Complete Example
Reading config
```
Config
timeout 42
items:
	"Linux"
	"Windows"
	"BeeOS"
```
Java code
```Java
Dom d = CmlDomReader.read(new FileReader("config.cml"));
System.out.writeln("timeout is " + query(d.root).checkType("Config").field("timeout").asInt(1800));
for (DomQuery i: query(d.root).checkType("Config").field("items"))
   System.out.writeln(i.asStr("unknown"));
```
## See also
- [How to create and modify DOM and write it to CMLs](https://github.com/karol11/cml/wiki/How-to-create-DOM-and-write-it-to-CMLs-in-Java).
- [How to Load Modify Write CML using DOM in Java](https://github.com/karol11/cml/wiki/How-to-Load-Modify-Write-CML-using-DOM-in-Java).
- TBD How to write application data directly to CML in STAX mode bypassing DOM creation.
- TBD How to read CML to application structures withoud DOM.
