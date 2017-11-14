package com.gmail.andrewkoder82;

import java.io.IOException;
import java.io.Writer;
import java.lang.reflect.Array;
import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.util.List;

public class CmlReflectWriter {
	CmlStaxWriter w;
	Writer fw;
	private int objNumerator;
	
	public CmlReflectWriter(Writer w) {
		this.fw = w;
		this.w = new CmlStaxWriter(w);
	}
	
	public void write(Object o) throws IOException, IllegalArgumentException, IllegalAccessException {
		writeNode(null, o);
		fw.flush();
	}
	
	private void writeNode(String field, Object o) throws IOException, IllegalArgumentException, IllegalAccessException {
		if (o == null)
			w.writeRef(field, "_");
		else if (o instanceof Integer)
			w.writeInt(field, (int)o);
		else if (o instanceof Character)
			w.writeInt(field, (char)o);
		else if (o instanceof Long)
			w.writeInt(field, (long)o);
		else if (o instanceof Byte)
			w.writeInt(field, (byte)o);
		else if (o instanceof Short)
			w.writeInt(field, (short)o);
		else if (o instanceof Boolean)
			w.writeBool(field, (boolean)o);
		else if (o instanceof Double)
			w.writeFloat(field, (double)o);
		else if (o instanceof Float)
			w.writeFloat(field, (float)o);
		else if (o instanceof String)
			w.writeString(field, (String)o);
		else if (o instanceof byte[])
			w.writeBin(field, (byte[])o);
		else if (o instanceof List<?>) {
			List<?> l = (List<?>) o;
			w.startArray(field, l.size());
			for (Object i: l)
				writeNode(null, i);
			w.endArray();
		} else if (o.getClass().isArray()) {
			w.startArray(field, Array.getLength(o));
			for (int i = 0, n = Array.getLength(o); i < n; i++)
				writeNode(null,  Array.get(o, i));
			w.endArray();
		} else {
			String id = getObjName(o);
			if (id == null)
				id = "_" + ++objNumerator;
			w.startObject(field, getTypeName(o.getClass()), id);
			for (Field f: o.getClass().getDeclaredFields()) {
				int m = f.getModifiers();
				if (!Modifier.isTransient(m) && !Modifier.isStatic(m)) {
					f.setAccessible(true);
					Object fo = f.get(o);
					f.setAccessible(false);
					writeNode(f.getName(), fo);
				}
			}
			w.endObject();
		}
	}

	public String getTypeName(Class<?> c) {
		return c.getSimpleName();
	}
	public String getObjName(Object o) {
		return null;
	}
}
