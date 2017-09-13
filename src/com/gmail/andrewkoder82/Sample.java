package com.gmail.andrewkoder82;

import java.io.IOException;
import java.io.OutputStreamWriter;
import java.io.StringReader;
import java.util.List;

public class Sample {
	public static void main(String[] params) {
		
		try {
			{
				Dom dom = new Dom();
				Dom.StructType tPoint = dom.getOrCreateStruct("point");
				tPoint.fields.add("x");
				tPoint.fields.add("y");
				dom.root = new Dom.Struct(tPoint)
					.setField("x", 10)
					.setField("y", 20);
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
				
				Dom.Struct s = (Dom.Struct) d.root;
				List<?> pts = (List<?>) s.getField("points");
				Dom.Struct p0 = (Dom.Struct) pts.get(0);
				assert((Long) p0.getField("y") == 42);
				//TODO: create helper class cml(d.root)._("points")._(0)._("y")._int(-1)
				
			}
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
}
