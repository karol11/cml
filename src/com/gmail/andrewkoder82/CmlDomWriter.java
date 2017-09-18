package com.gmail.andrewkoder82;

import java.io.IOException;
import java.io.Writer;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class CmlDomWriter {
	
	static void write(Dom dom, Writer out) throws IOException {
		CmlDomWriter w = new CmlDomWriter(dom, out);
		w.traverse(dom.root, null);
		out.flush();
	}
	
	final Dom dom;
	final Map<Dom.Struct, String> visited = new HashMap<Dom.Struct, String>();
	int idNumerator;
	CmlStaxWriter out;
	
	public CmlDomWriter(Dom dom, Writer out) {
		this.dom = dom;
		this.out = new CmlStaxWriter(out);
	}
	
	public void traverse(Object o, String field) throws IOException {
		if (o instanceof Integer)
			out.writeInt(field, (int)o);
		else if (o instanceof Long)
			out.writeInt(field, (long)o);
		else if (o instanceof String)
			out.writeString(field, (String)o);
		else if (o instanceof List<?>) {
			out.startArray(field, ((List<?>)o).size());
			for (Object i: (List<?>)o)
				traverse(i, null);
			out.endArray();
		} else if (o instanceof Dom.Struct) {
			Dom.Struct s = (Dom.Struct) o;
			String id = visited.get(s);
			if (id != null) {
				out.writeRef(field, id);
			} else {
				out.startObject(field, s.type.name, s.name);
				visited.put(s, s.name == null ? "$" + idNumerator++ : s.name);
				for (Map.Entry<String, Object> i: s.fields.entrySet())
					traverse(i.getValue(), i.getKey());
				out.endObject();
			}
		}
	}
}
