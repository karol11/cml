package com.gmail.andrewkoder82;

import java.io.FileReader;
import java.io.IOException;
import java.io.StringReader;
import java.io.StringWriter;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.gmail.andrewkoder82.Dom.Struct;

import static com.gmail.andrewkoder82.DomQuery.query;

public class FuncTest {

	public static void main(String[] params) throws IOException {
		Dom config = CmlDomReader.read(new FileReader("config.cml"));
		for (DomQuery i: query(config.root).field("cmlList")) {
			try{
				processFile(i.asStr(""));				
			} catch (RuntimeException e) {
				System.err.println(e + " parsing " + i.asStr(""));
				break;
			}
		}
		System.out.println("passed");
	}

	private static void processFile(String name) {
		try {
			Dom a = CmlDomReader.read(new FileReader(name));
			StringWriter s = new StringWriter();
			CmlDomWriter.write(a, s);
			Dom b = CmlDomReader.read(new StringReader(s.toString()));
			match(a.root, b.root, "");
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

	static Map<Object, Object> topology = new HashMap<Object, Object>();
	
	private static void match(Object a, Object b, String path) {
		if (a == null || b == null) {
			if (a != b)
				throw new RuntimeException("null vs not null at " + path);
			return;
		}
		if (a.getClass() != b.getClass())
			throw new RuntimeException(a.getClass().getSimpleName() + " vs " + b.getClass().getSimpleName() + " at " + path);
		if (a instanceof Boolean) {
			if ((boolean)a != (boolean)b)
				throw new RuntimeException("bool val " + a + " vs " + b + " at " + path);
		} else if (a instanceof Integer) {
			if ((int)a != (int)b)
				throw new RuntimeException("int val " + a + " vs " + b + " at " + path);				
		} else if (a instanceof Long) {
			if ((long)a != (long)b)
				throw new RuntimeException("long val " + a + " vs " + b + " at " + path);				
		} else if (a instanceof Float) {
			if ((float)a != (float)b)
				throw new RuntimeException("float val " + a + " vs " + b + " at " + path);				
		} else if (a instanceof Double) {
			if ((double)a != (double)b)
				throw new RuntimeException("double val " + a + " vs " + b + " at " + path);				
		} else if (a instanceof String) {
			if (!a.equals(b))
				throw new RuntimeException("str val " + a + " vs " + b + " at " + path);				
		} else if (a instanceof byte[]) {
			byte[] xa = (byte[]) a;
			byte[] xb = (byte[]) b;
			if (xa.length != xb.length)
				throw new RuntimeException("bin length " + xa.length + " vs " + xb.length + " at " + path);				
			for (int i = 0; i < xa.length; i++)
				if (xa[i] != xb[i])
					throw new RuntimeException("byte[" + i + "] " + xa[i] + " vs " + xb[i] + " at " + path);				
		} else if (a instanceof List<?>) {
			List<?> la = (List<?>) a;
			List<?> lb = (List<?>) b;
			if (la.size() != lb.size())
				throw new RuntimeException("array size " + la.size() + " vs " + lb.size() + " at " + path);				
			for (int i = 0, n = la.size(); i < n; i++)
				match(la.get(i), lb.get(i), path + "[" + i + "]");
		} else if (a instanceof Dom.Struct) {
			Object ka = topology.get(a);
			Object kb = topology.get(b);
			if (ka == null && kb == null)
				topology.put(a,  b);
			else if (ka == null ? kb != a : ka != b)
				throw new RuntimeException("topology " + a.hashCode() + " vs " + b.hashCode() + " at " + path);				
			Dom.Struct da = (Dom.Struct) a;
			Dom.Struct db = (Dom.Struct) b;
			String an = da.name == null ? "" : da.name;
			String bn = db.name == null ? "" : db.name;
			if (!an.equals(bn))
				throw new RuntimeException("names " + an + " vs " + bn + " at " + path);
			matchFields(da, db, path);
			matchFields(db, da, path);
		}
	}

	private static void matchFields(Struct da, Struct db, String path) {
		for (Map.Entry<String, Object> af: da.fields.entrySet()) {
			if (!db.fields.containsKey(af.getKey()))
				throw new RuntimeException("no field " + af.getKey() + " at " + path);
			match(af.getValue(), db.fields.get(af.getKey()), path + "." + af.getKey());
		}
	}
}
