# cml

CML - is a text-based object serialization data format.
Its nearest analogs are YAML, XML and JSON.
The main features that distinguish CML from competitors are:
- CML stores explicit type information,
- Its data formats are not limited to tree-like structures, CML can store arbitrary cross-references,
- CML has simple, obvious and unambiguous syntax ideally suitable both for human reader and computer parsing.

This repository contains libraries for JavaScript, Java and C that helps both read and write CML files in STAX and DOM modes.

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

  Submenu.resents
  caption "Recent files"
```

