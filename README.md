CML
---
CML - is a text-based object representation data format.

Its nearest analogs are YAML, XML and JSON.

The main features that distinguish CML from competitors are:
- CML stores explicit type information,
- Its data formats are not limited to tree-like structures, CML can store arbitrary cross-references,
- CML has simple, obvious and unambiguous syntax ideally suitable both for human reader and computer parsing.

This repository contains libraries for
- JavaScript,
- Java
- and C
that helps both read and write CML files in STAX and DOM modes.

Here is a simple CML example:
```python
Menu.file
caption "File"
items:
  Command.new
  caption "New..."
  hotkey "Ctrl+N"

  Command.open
  caption "Open..."

  Divider

  Submenu.recents
  caption "Recent files"
```
And here is a more complex example showing all CML features
```
; this is a comment line
Polygon                        ; <- class name of root object
description "A test polygon"   ; <- a string field
scale 1.5                      ; <- a floating-point number field
visible +                      ; <- a boolean field
pivot Point                    ; <- a field containing a nested object of class Point
  x 100                        ;    <- this nested point has two integer fields x and y
  y 42
points:                        ; <- another field of root polygon containing an array
  Point                        ;    <- array contents - three point objects
  x -10
  y 0

  Point.p1                     ;    <- this is a named object p1
  x 10
  y 10.5
	
  Point
  x 0
  y 0
focused=p1                     ; <- this field is a *reference* to the Point.p1
background# 177                ; <- this field contains binary data
	R0lGODdhGAAgAKIAAAAAAIAAAICAgMDAwP8AAP
	///wAAAAAAACwAAAAAGAAgAAAEfpDISau9VhTM
	6/hdSAybyBWleaGDuhatOwkkLE8wGd8FnfK6m0
	RnExJ6IOPAZyQIfD/X87O7PafCJVVg4jqXtI+X
	E74KAACzJwk2n9Hn8PYpoVMG8LjblE8L32dVKg
	FTAUpaY1JLAEuGUmlocDJvkX6PlZZSAWiERgIB
	hIIVEQA7
