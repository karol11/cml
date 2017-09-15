package com.gmail.andrewkoder82;

import java.io.IOException;
import java.io.OutputStreamWriter;
import java.io.StringReader;
import static com.gmail.andrewkoder82.DomQuery.query;

public class Sample {
	public static void main(String[] params) {
		
		try {
			testStaxReader();
			testWriter();
			testReader();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	private static void testStaxReader() throws IOException {
		CmlStaxReader d = new CmlStaxReader(new StringReader(
				"Polygon\n"+
				"name \"Test text\"\n"+
				"points:\n"+
				"  Point.p1\n"+
				"  x 1\n"+
				"  y 42\n"+
				"ordered:\n"+
				"  =p1"));

		StringBuilder json = new StringBuilder();
		boolean neededComma = false;
		String prefix = "\n";
		for (;;) {
			int i = d.next();
			if (i == CmlStaxReader.R_EOF)
				break;
			if (i == CmlStaxReader.R_ARRAY_END || i == CmlStaxReader.R_STRUCT_END)
				neededComma = false;
			String f = d.getField();
			if (f != null)
				json.append("," + prefix + "\"" + f + "\":");
			else if (neededComma)
				json.append("," + prefix);
			else {
				neededComma = true;
				json.append(prefix);
			}
			switch (i) {
			case CmlStaxReader.R_INT:
				json.append(d.getLongVal());
				break;    
			case CmlStaxReader.R_STRING:
				json.append("\"" + d.getStrVal().replace("\"", "\\\"") + "\"");
				break; 
			case CmlStaxReader.R_STRUCT_START:
				{
					prefix += "\t";
					json.append("{" + prefix + "\"$\":\"" + d.getType()+"\"");
					String id = d.getId();
					if (id != null)
						json.append(","+prefix+"\"#\":\"" + id + "\"");
				}
				break;
			case CmlStaxReader.R_STRUCT_END:
				json.setLength(json.length()-1);
				json.append("}");
				prefix = prefix.substring(0, prefix.length() - 1);
				break;
			case CmlStaxReader.R_REF:
				json.append("{\"=\": \"" + d.getId() + "\"}");
				break;
			case CmlStaxReader.R_ARRAY_START:
				json.append("[");
				prefix += "\t";
				neededComma = false;
				break;
			case CmlStaxReader.R_ARRAY_END:
				json.setLength(json.length()-1);
				json.append("]");
				prefix = prefix.substring(0, prefix.length() - 1);
				break;
			}
		}
		System.out.println(json.toString());
	}

	private static void testReader() throws IOException {
		Dom d = CmlReader.read(new StringReader(
			"Polygon\n"+
			"name \"Test text\"\n"+
			"points:\n"+
			"  Point.p1\n"+
			"  x 1\n"+
			"  y 42\n"+
			"ordered:\n"+
			"  =p1"));

		CmlWriter.write(d, new OutputStreamWriter(System.out));				

		for (DomQuery i: query(d.root).field("points"))
			System.out.println("x=" + i.field("x").asInt(-1) + " y=" + i.field("y").asInt(-1));
	}

	private static void testWriter() throws IOException {
		Dom dom = new Dom();
		Dom.StructType tPoint = dom.getOrCreateStruct("point");
		tPoint.fields.add("x");
		tPoint.fields.add("y");
		dom.root = new Dom.Struct(tPoint)
			.set("x", 10)
			.set("y", 20);
		CmlWriter.write(dom, new OutputStreamWriter(System.out));				
	}
}
