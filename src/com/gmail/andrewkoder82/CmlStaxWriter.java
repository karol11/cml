package com.gmail.andrewkoder82;

import java.io.IOException;
import java.io.Writer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

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
	public void writeString(String field, String val) throws IOException {
		addPrefix(field);
		out.append('"').append(val.replace("\"", "\"\"")).append("\"\n");		
	}
	public void writeRef(String field, String id) throws IOException {
		addPrefix(field);
		out.append('=').append(id).append('\n');
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
