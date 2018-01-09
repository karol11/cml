package com.github.karol11.cml;

import java.io.IOException;
import java.io.Reader;
import java.lang.reflect.Array;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class CmlReflectReader {
	CmlStaxReader r;
	Map<String, Object> named = new HashMap<>();

	public CmlReflectReader(Reader r) throws IOException {
		this.r = new CmlStaxReader(r);
	}
	
	public Object read(Class<?> expected) throws IOException {
		return readNode(null, 0, r.next(), expected);
	}

	long checkLimit(long truncVal, long srcVal, String type) {
		if (truncVal != srcVal)
			r.error("value is out of " + type + " range ");
		return truncVal;
	}
	double checkLimit(double truncVal, double srcVal, String type) {
		if (Math.abs(truncVal - srcVal) > Double.MIN_NORMAL)
			r.error("value is out of " + type + " range ");
		return truncVal;
	}
	
	Object readNode(Field fieldToStore, int arrayDepth, int type, Class<?> expected) throws IOException {
		switch (type) {
		case CmlStaxReader.R_STRUCT_START:
			try {
				Object o = factory(r.getType());
				if (o == null)
					r.error("unexpected struc type " + r.getType());
				Class<?> c = o.getClass();
				if (r.getId() != null)
					named.put(r.getId(), o);
				for (int i = 0; (i = r.next()) != CmlStaxReader.R_STRUCT_END;) {
					String fieldName = r.getField();
					if (fieldName == null)
						r.error("expected field");
					Field field = c.getDeclaredField(fieldName);
					Object v = readNode(field, 0, i, field.getType());
					field.setAccessible(true);
					field.set(o, v);
					field.setAccessible(false);
				}
				return o;
			} catch (IllegalAccessException e) {
				e.printStackTrace();
			} catch (NoSuchFieldException e) {
				r.error("unexpected field " + r.getField());
				e.printStackTrace();
			} catch (SecurityException e) {
				e.printStackTrace();
			}
			break;
		case CmlStaxReader.R_LONG: {
			long l = r.getLongVal();
			return
				expected == int.class || expected == Integer.class ? (int) checkLimit((int)l, l, "int") :
				expected == long.class || expected == Long.class ? l :
				expected == char.class || expected == Character.class ? (char) checkLimit((char)l, l, "char") :
				expected == byte.class || expected == Byte.class ? (byte) checkLimit((byte)l, l, "byte") :
				expected == short.class || expected == Short.class ? (short) checkLimit((short)l, l, "short") :
				expected == String.class ? Long.toString(l) :
				expected == double.class || expected == Double.class ? (double) checkLimit((long)(double)l, l, "double") :
				expected == float.class || expected == Float.class ? (float) checkLimit((long)(float) l, l, "float"):
				expected == boolean.class || expected == Boolean.class ? l != 0 :
				l;
		}
		case CmlStaxReader.R_STRING: {
			String s = r.getStrVal();
			return 
				expected == int.class || expected == Integer.class ? Integer.parseInt(s) :
				expected == long.class || expected == Long.class ? Long.parseLong(s) :
				expected == char.class || expected == Character.class ? (char) Integer.parseInt(s) :
				expected == byte.class || expected == Byte.class ? (byte) Byte.parseByte(s) :
				expected == short.class || expected == Short.class ? (short) Short.parseShort(s) :
				expected == String.class ? s :
				expected == double.class || expected == Double.class ? Double.parseDouble(s) :
				expected == float.class || expected == Float.class ? Float.parseFloat(s) :
				expected == boolean.class || expected == Boolean.class ? s.length() > 0 && !s .equals("false") :
				s;
		}
		case CmlStaxReader.R_BOOL: {
			boolean b = r.getBoolVal();
			return
				expected == int.class || expected == Integer.class ? Integer.valueOf(b ? 1 : 0) :
				expected == long.class || expected == Long.class ? Long.valueOf(b ? 1 : 0) :
				expected == char.class || expected == Character.class ? Character.valueOf((char) (b ? 1 : 0)) :
				expected == byte.class || expected == Byte.class ? Byte.valueOf((byte) (b ? 1 : 0)) :
				expected == short.class || expected == Short.class ? Short.valueOf((short) (b ? 1 : 0)) :
				expected == String.class ? (b ? "true" : "false") :
				expected == double.class || expected == Double.class ? Double.valueOf((double) (b ? 1 : 0)) :
				expected == float.class || expected == Float.class ? Float.valueOf((float) (b ? 1 : 0)) :
				b;
		}
		case CmlStaxReader.R_DOUBLE: {
			double d = r.getDblVal();
			return
				expected == int.class || expected == Integer.class ? (int) checkLimit((double)(int)d, d, "int"):
				expected == long.class || expected == Long.class ? (long) checkLimit((double)(long)d, d, "long") :
				expected == char.class || expected == Character.class ? (char) checkLimit((double)(char)d, d, "char") :
				expected == byte.class || expected == Byte.class ? (byte) checkLimit((double)(byte)d, d, "byte") :
				expected == short.class || expected == Short.class ? (short) checkLimit((double)(short)d, d, "short") :
				expected == String.class ? Double.toString(d) :
				expected == double.class || expected == Double.class ? d :
				expected == float.class || expected == Float.class ? (float) checkLimit((double)(float)d, d, "float") :
				d;
		}
		case CmlStaxReader.R_BINARY:
			return r.getBinary();
		case CmlStaxReader.R_ARRAY_START: {
			Class<?> componentType = getComponentType(fieldToStore, arrayDepth, expected);
			int s = r.getSize();
			List<Object> arr = s < 0 ? new ArrayList<>() : new ArrayList<>(s);
			for (int t = 0; (t = r.next()) != CmlStaxReader.R_ARRAY_END;)
				arr.add(readNode(fieldToStore, arrayDepth + 1, t, componentType));
			return arrayFactory(fieldToStore, arrayDepth, arr, expected, componentType);
		}
		case CmlStaxReader.R_REF:
			return named.get(r.getId());
		default:
			r.error("unexpected node");
		}
		return null;
	}

	public Object factory(String type) {
		try {
	        Constructor<?> constructor = Class.forName(type.replace("/", ".")).getDeclaredConstructor(new Class[0]);
	        constructor.setAccessible(true);
	        Object r = constructor.newInstance(new Object[0]);
	        constructor.setAccessible(false);
			return r;
		} catch (InstantiationException | IllegalAccessException | ClassNotFoundException |
				InvocationTargetException | IllegalArgumentException | NoSuchMethodException | SecurityException e) {
			e.printStackTrace();
		}
		return null;
	}

	public Class<?> getComponentType(Field fieldToStore, int arrayDepth, Class<?> expected) {
		return expected.isArray() ? expected.getComponentType() : Object.class;
	}

	public Object arrayFactory(Field fieldToStore, int arrayDepth, List<Object> arr, Class<?> expected, Class<?> componentType) {
		if (expected.isArray()) {
			Object r = Array.newInstance(componentType, arr.size());
			for (int i = 0, n = arr.size(); i < n; i++)
				Array.set(arr, i, arr.get(i));
			return r;
		} else
			return arr;
	}
}
