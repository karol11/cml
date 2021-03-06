package com.github.karol11.cml;

import java.util.Iterator;
import java.util.List;
import java.util.Map;

public class DomQuery implements Iterable<DomQuery> {
	Object target;
	static DomQuery dummy = new DomQuery(null);

	static DomQuery query(Object target) {
		return target == null ? dummy : new DomQuery(target);
	}

	public boolean isUndefined() { return this == dummy; }
	public boolean isInt() { return target instanceof Integer || target instanceof Long; }
	public boolean isFloat() { return target instanceof Float || target instanceof Double; }
	public boolean isString() { return target instanceof String; }
	public boolean isArray() { return target instanceof List<?>; }
	public boolean isRef() { return target == null || target instanceof Dom.Struct; }
	public boolean isStruct() { return target instanceof Dom.Struct; }
	public boolean isBool() { return target instanceof Boolean; }
	public boolean isBin() { return target instanceof byte[]; }

	DomQuery(Object target) {
		this.target = target;
	}

	public DomQuery field(String field) {
		return target instanceof Dom.Struct ? query(((Dom.Struct)target).get(field)) : dummy;
	}
	public DomQuery checkType(String structType) {
		return typeName().equals(structType) ? this : dummy;
	}
	public String typeName() {
		return target instanceof Dom.Struct ? ((Dom.Struct)target).type.name : "";
	}
	public String name() {
		String r = target instanceof Dom.Struct ? ((Dom.Struct)target).name : null;
		return r == null ? "" : r;
			
	}
	public DomQuery set(String field, Object val) {
		if (target instanceof Dom.Struct)
			((Dom.Struct)target).set(field, val);
		return this;
	}
	public DomQuery at(int index) {
		if (target instanceof List<?>) {
			List<?> l = (List<?>)target;
			return index < l.size() ? query(l.get(index)) : dummy;
		}
		return dummy;
	}
	public DomQuery set(int index, Object val) {
		if (target instanceof List) {
			@SuppressWarnings("unchecked")
			List<Object> l = (List<Object>)target;
			l.set(index, val);
			return this;
		}
		return dummy;
	}
	public DomQuery insert(int index, Object val) {
		if (target instanceof List) {
			@SuppressWarnings("unchecked")
			List<Object> l = (List<Object>)target;
			l.add(index, val);
			return this;
		}
		return dummy;
	}
	public DomQuery add(Object val) {
		if (target instanceof List) {
			@SuppressWarnings("unchecked")
			List<Object> l = (List<Object>)target;
			l.add(val);
			return this;
		}
		return dummy;
	}
	public DomQuery remove(int index) {
		if (target instanceof List) 
			((List<?>)target).remove(index);
		return dummy;
	}
	public int size() {
		return target instanceof List<?> ? ((List<?>)target).size() : 0;
	}
	public boolean exists() {
		return this != dummy;
	}
	public Object get() { return target; }
	
	public int asInt(int defVal) {
		if (target instanceof Boolean)
			return (boolean)target ? 1 : 0;
		if (target instanceof Integer)
			return (int)target;
		if (target instanceof Long) {
			long l = (long)target;
			return l > Integer.MIN_VALUE && l < Integer.MAX_VALUE ? (int) l : defVal;
		}
		if (target instanceof String) {
			try {
				return Integer.parseInt((String) target);
			} catch (NumberFormatException e) {
				return defVal;
			}
		}
		return defVal;
	}
	public long asLong(long defVal) {
		if (target instanceof Boolean)
			return (boolean)target ? 1 : 0;
		if (target instanceof Integer)
			return (int)target;
		if (target instanceof Long)
			return (long)target;
		if (target instanceof String) {
			try {
				return Long.parseLong((String) target);
			} catch (NumberFormatException e) {
				return defVal;
			}
		}
		return defVal;
	}
	public float asFloat(float defVal) {
		if (target instanceof Boolean)
			return (boolean)target ? 1 : 0;
		if (target instanceof Integer)
			return (int)target;
		if (target instanceof Float)
			return (float)target;
		if (target instanceof Long) {
			long l = (long)target;
			return l > Float.MIN_VALUE && l < Float.MAX_VALUE ? (float) l : defVal;
		}
		if (target instanceof Double) {
			double l = (double)target;
			return l > Float.MIN_VALUE && l < Float.MAX_VALUE ? (float) l : defVal;
		}
		if (target instanceof String) {
			try {
				return Float.valueOf((String) target);
			} catch (NumberFormatException e) {
				return defVal;
			}
		}
		return defVal;
	}
	public double asDouble(double defVal) {
		if (target instanceof Boolean)
			return (boolean)target ? 1 : 0;
		if (target instanceof Integer)
			return (int)target;
		if (target instanceof Float)
			return (float)target;
		if (target instanceof Long) {
			long l = (long)target;
			return l;
		}
		if (target instanceof Double) {
			double l = (double)target;
			return l;
		}
		if (target instanceof String) {
			try {
				return Double.valueOf((String) target);
			} catch (NumberFormatException e) {
				return defVal;
			}
		}
		return defVal;
	}
	public String asStr(String defVal) {
		if (target instanceof Integer || target instanceof Long || target instanceof Float || target instanceof Double || target instanceof Boolean)
			return target.toString();
		if (target instanceof String)
			return (String) target;
		return defVal;		
	}
	public byte[] asRaw(byte[] defVal) {
		return target instanceof byte[] ? (byte[]) target : defVal;		
	}
	public boolean asBool(boolean defVal) {
		if (target instanceof Integer)
			return (int) target != 0;
		if (target instanceof Long)
			return (long) target != 0;
		if (target instanceof String)
			return
				!((String)target).equals("false") &&
				!((String)target).equals("") &&
				!((String)target).equals("0");
		return defVal;		
	}
	
	@Override
	public Iterator<DomQuery> iterator() {
		if (target instanceof List<?>) {
			final Iterator<?> i = ((List<?>)target).iterator();
			return new Iterator<DomQuery>() {
				@Override
				public boolean hasNext() { return i.hasNext(); }

				@Override
				public DomQuery next() { return query(i.next()); }
			};
		}
		return new Iterator<DomQuery>() {
			@Override
			public boolean hasNext() { return false; }
			@Override
			public DomQuery next() { return dummy; }			
		};
	}
		
	Iterable<Field> fields() {
		return new Iterable<Field> () {
			@Override public Iterator<Field> iterator() {
				return target != null && target instanceof Dom.Struct ? 
					new Iterator<Field>(){
						Iterator<Map.Entry<String, Object>> i = ((Dom.Struct)target).fields.entrySet().iterator();
						@Override public boolean hasNext() { return i.hasNext(); }
						@Override public Field next() { return new Field(i.next()); }
					} :
					new Iterator<Field> (){
						@Override public boolean hasNext() { return false;}
						@Override public Field next() { return null; }
					};
			}
		};
		
	}
	public static class Field{
		final Map.Entry<String, Object> e;
		Field(Map.Entry<String, Object> e) {
			this.e = e;
		}
		public String getField() { return e.getKey(); }
		public DomQuery getValue() { return new DomQuery(e.getValue()); }
	};
}
