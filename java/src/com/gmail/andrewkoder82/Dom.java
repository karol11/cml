package com.gmail.andrewkoder82;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;


/**
 
 DOM represents data structure been read from or prepared to be written to CML file.
 It is no more than:
 - a root object of a hierarchy,
 - a map of global names,
 - and a set of possible named structures,
 
 CML document elements can be:
 - String, Boolean, Float, Double, Integer and Long terminal objects
 - byte[] - raw binary data
 - List<Of elements>
 - Dom.Struct-s having:
      - type
      - optional name
      - and map of fields storing elements.
 The DOM needs not to have a tree-like structure.
 It can have diamond-like topology or even have cycles.
 Each Dom.Struct have associated Dom.StructType object, that lists:
 - type name
 - a set of possible fields.

 Also DOM allows structures to have names.
 And it allows to lookup objects by names. 
 
 Dom examples
 Dom d = new Dom(); // a document having null root object.
 
 Dom d = new Dom(); // a document of single integer.
 d.root = 42;

 Dom d = new Dom(); // a document having null root object.
 d.root = new ArrayList<Object>(Arrays.asList("aaa", "bbb"));
 // document containing an array with two strings.

 Dom d = new Dom(); // a document having null root object.
 Dom.StructType t = d.getOrCreateStruct("point");
 t.fields.add("x");
 t.fields.add("y");
 d.root = new Dom.Struct(pointT)
   .setField("x", 10)
   .setField("y", 20);
 // document containing a root structure of type 'point'
 //having two fields 'x' and 'y' with values 10 and 20.
 
 d.setName("base_point", (Dom.Struct) d.root);
 
 */
public class Dom {
	Map<String, StructType> structTypes = new HashMap<String, StructType>();
	Map<String, Struct> named = new HashMap<String, Struct>();

	/**
	 * R/W access to DOM's root object
	 */
	public Object root;
	
	/**
	 * Returns Dom.StructType by name. If none, returns null.
	 */
	public StructType getStruct(String name) { return structTypes.get(name); }

	/**
	 * Returns Dom.StructType by name. If none, creates one.
	 */
	public StructType getOrCreateStruct(String name) {
		StructType r = structTypes.get(name);
		if (r == null)
			structTypes.put(name, r = new StructType(name));
		return r;
	}
	
	/**
	 * Returns a named Dom.Struct by name. If none, returns null.
	 */
	public Struct getNamed(String name) { return named.get(name); }

	/**
	 * Sets name for a Dom.Struct.
	 * A Dom.Struct can have just one name.
	 * And a name can refer just one Dom.Struct.
	 */
	public Struct setName(String name, Struct dst) {
		Struct old = named.remove(name);
		if (old != null)
			old.name = null;
		if (dst != null) {
			named.remove(dst.name);
			named.put(name, dst);
			dst.name = name;
		}
		return dst;
	}

	public static class StructType {
		StructType(String name) {
			this.name = name;
		}
		
		public final Set<String> fields = new HashSet<String>();
		public final String name;
	}

	public static class Struct{
		Struct(StructType type) {
			this.type = type;
		}
		
		public String getName() { return name; }
		
		public Object get(String name) { return fields.get(name); }
		
		public Struct set(String name, Object val) {
			if (!type.fields.contains(name))
				throw new UnsupportedOperationException("struct " + type.name + " doesn't have field " + name);
			fields.put(name, val);
			return this;
		}
		
		public final StructType type;
		String name;
		Map<String, Object> fields = new HashMap<String, Object>();
	}
}
