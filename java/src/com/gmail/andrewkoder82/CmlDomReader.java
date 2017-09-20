package com.gmail.andrewkoder82;

import java.io.IOException;
import java.io.Reader;
import java.util.ArrayList;
import java.util.Map;

public class CmlDomReader {
	
	public static Dom read(Reader in) throws IOException {
		Dom r = new Dom();
		new CmlDomReader(r, in);
		return r;
	}
	
	CmlStaxReader stax;
	Dom dom;
	Map<String, Dom.Struct> named;
	
	CmlDomReader(Dom dom, Reader in) throws IOException {
		this.dom = dom;
		this.stax = new CmlStaxReader(in);
		dom.root = parseNode(stax.next());
	}
	
	Object parseNode(int type) throws IOException {
		switch (type) {
		case CmlStaxReader.R_INT: return stax.getLongVal();
		case CmlStaxReader.R_STRING: return stax.getStrVal();
		case CmlStaxReader.R_REF: 
			{
				String id = stax.getId(); 
				if (id.equals("$"))
					return null;
				Object r = id.startsWith("$") ? named.get(id) : dom.getNamed(id);
				if (r == null)
					stax.error("unresolved name " + id);
				return r;				
			}
		case CmlStaxReader.R_ARRAY_START:
			{
				ArrayList<Object> r = new ArrayList<Object>();
				for (;;) {
					type = stax.next();
					if (type == CmlStaxReader.R_ARRAY_END)
						break;
					r.add(parseNode(type));
				}
				return r;
			}
		case CmlStaxReader.R_STRUCT_START:
			{
				Dom.Struct r = new Dom.Struct(dom.getOrCreateStruct(stax.getType()));
				String id = stax.getId();
				if (id != null) {
					if (id.startsWith("$"))
						named.put(id,  r);
					else
						dom.setName(id, r);
				}
				for (;;) {
					type = stax.next();
					if (type == CmlStaxReader.R_STRUCT_END)
						break;
					String field = stax.getField();
					r.type.fields.add(field);
					r.set(field, parseNode(type));
				}
				return r;
			}
		}
		stax.error("unexpected node " + type);
		return null;
	}
}
