package com.gmail.andrewkoder82;

import java.io.IOException;
import java.io.Writer;
import java.util.ArrayList;
import java.util.List;

public class CmlStaxWriter {	
	boolean inArray = true;
	String indent = "";
	final Writer out;
	final List<Boolean> states = new ArrayList<Boolean>();
	
	public CmlStaxWriter(Writer out) {
		this.out = out;
	}

	public void startObject(String field, String typeName, String id) throws IOException{
		addPrefix(field);
		out.append(typeName);
		if (id != null)
			out.append('.').append(id);
		out.append('\n');
		pushState(false);
	}

	public void endObject() throws IOException{
		popState();
		if (inArray)
			out.append('\n');
	}
	public void startArray(String field, int size) throws IOException {
		addPrefix(field);
		out.append(":");
		if (size >= 0)
			out.append(Integer.toString(size));
		out.append('\n');
		pushState(true);		
	}
	public void endArray() {
		popState();
	}
	public void writeInt(String field, long val) throws IOException {
		addPrefix(field);
		out.append(Long.toString(val)).append("\n");		
	}
	public void writeBool(String field, boolean val) throws IOException {
		addPrefix(field);
		out.append(val ? "+" : "-").append("\n");		
	}
	public void writeFloat(String field, double val) throws IOException {
		addPrefix(field);
		out.append(Double.toString(val)).append("\n");		
	}
	public void writeString(String field, String val) throws IOException {
		addPrefix(field);
		StringBuilder r = new StringBuilder();
		for (int i = 0, n = val.length(); i < n; ++i) {
			char c = val.charAt(i);
			if (c <= 0x1f || (c >= 0x7f && c <= 0x9f) || c == 0x2028 || c == 0x2029)
				r.append("\\u").append(String.format("%04X ", c));				
			else if (c == '"')
				r.append("\\\"");
			else if (c == '\\')
				r.append("\\\\");
			else
				r.append(c);
		}
		out.append('"').append(r).append("\"\n");
	}
	public void writeRef(String field, String id) throws IOException {
		addPrefix(field);
		out.append('=').append(id == null ? "_" : id).append('\n');
	}
	
	private char code2char(int c) {
		c &= 0x3f;
		if (c < 26) return (char)('A' + c);
		if (c < 52) return (char)('a' + c - 26);
		if (c < 62) return (char)('0' + c - 52);
		return c == 62 ? '+' : '/';
	}
	
	public void writeBin(String field, byte[] val) throws IOException {
		writeBin(field, val, 0, val.length);
	}
	
	public void writeBin(String field, byte[] val, int at, int length) throws IOException {
		out.append('#').append(Integer.toString(length));
		int perRow = 1;
		for (; length >= 3; length -= 3, at += 3) {
			if (--perRow == 0) {
				out.append('\n').append(indent).append('\t');
				perRow = 64;
			}
			int a = val[at];
			int b = val[at + 1];
			int c = val[at + 2];
			out
				.append(code2char(a >> 2))
				.append(code2char(a << 4 | b >> 4))
				.append(code2char(b << 2 | c >> 6))
				.append(code2char(c));
		}
		if (length != 0) {
			int a = val[at];
			out.append(code2char(a >> 2));
			if (length == 1) {
				out
					.append(code2char(a << 4))
					.append('=');
			} else {
				int b = val[at + 1];
				out
					.append(code2char(a << 4 | b >> 4))
					.append(code2char(b << 2));
			}
			out.append('=');
		}
		out.append('\n');
	}
	
	private void pushState(boolean newInArray) {
		states.add(inArray);
		inArray = newInArray;
		indent += "\t";	
	}
	private void popState() {
		inArray = states.remove(states.size() - 1);
		indent = indent.substring(1);
	}
	private void addPrefix(String field) throws IOException {
		out.append(indent);
		if ((field == null) != inArray)
			throw new RuntimeException("array item should have no field name");
		if (field != null)
			out.append(field).append(' ');
	}
}
