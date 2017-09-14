package com.gmail.andrewkoder82;

import java.io.IOException;
import java.io.OutputStreamWriter;
import java.io.StringReader;
import java.util.List;
import static com.gmail.andrewkoder82.DomQuery.query;

public class Sample {
	public static void main(String[] params) {
		
		try {
			{
				Dom dom = new Dom();
				Dom.StructType tPoint = dom.getOrCreateStruct("point");
				tPoint.fields.add("x");
				tPoint.fields.add("y");
				dom.root = new Dom.Struct(tPoint)
					.set("x", 10)
					.set("y", 20);
				CmlWriter.write(dom, new OutputStreamWriter(System.out));				
			}
			{
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
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
}
