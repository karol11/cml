package com.gmail.andrewkoder82;

import java.io.IOException;
import java.io.Writer;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class CmlWriter {
	
	static void write(Dom dom, Writer out) throws IOException {
		CmlWriter w = new CmlWriter(dom, out);
		w.traverse(dom.root, "", true);
		out.flush();
	}
	
	final Dom dom;
	final Writer out;
	final Map<Dom.Struct, String> visited = new HashMap<Dom.Struct, String>();
	int idNumerator;
	
	public CmlWriter(Dom dom, Writer out) {
		this.dom = dom;
		this.out = out;
	}
	
	public void traverse(Object o, String prefix, boolean inArray) throws IOException {
		if (o instanceof Integer || o instanceof Long) {
			out.append(o.toString()).append("\n");
		} else if (o instanceof String)
			out.append('"').append(((String)o).replace("\"", "\"\"")).append("\"\n");
		else if (o instanceof List<?>) {
			out.append(":\n");
			String innerIndent = prefix + "\t";
			for (Object i: (List<?>)o) {
				out.append(innerIndent);
				traverse(i, innerIndent, true);
			}
		} else if (o instanceof Dom.Struct) {
			Dom.Struct s = (Dom.Struct) o;
			String id = visited.get(s);
			if (id != null) {
				out.append('=').append(id).append('\n');
			} else {
				visited.put(s, s.name == null ? "$" + idNumerator++ : s.name);
				String innerIndent = inArray ? prefix : (prefix + "\t");
				out.append(s.type.name).append(s.name != null ? "." + s.name : "").append('\n');
				for (Map.Entry<String, Object> i: s.fields.entrySet()) {
					out.append(prefix).append(i.getKey()).append(' ');
					traverse(i.getValue(), innerIndent, false);
				}
				if (inArray)
					out.append('\n');
			}
		}
	}
}
