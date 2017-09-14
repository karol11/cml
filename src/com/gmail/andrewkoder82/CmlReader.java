package com.gmail.andrewkoder82;

import java.io.IOException;
import java.io.Reader;
import java.util.ArrayList;
import java.util.Map;

public class CmlReader {
	
	public static Dom read(Reader in) throws IOException {
		Dom r = new Dom();
		r.root = new CmlReader(r, in).parseNode(false);
		return r;
	}
	
	Reader in;
	boolean wasIndented;
	boolean indentWithTabs;
	int lineNumber = 0;
	int indentPos = 0;
	int charPos = 0;
	int cur;
	Dom dom;
	Map<String, Dom.Struct> named;
	
	CmlReader(Dom dom, Reader in) throws IOException {
		this.dom = dom;
		this.in = in;
		cur = '\n';
		expectedNewLine();
	}
	
	void error(String message) {
		throw new RuntimeException(message + " at " + lineNumber + ":" + charPos);		
	}
	
	Object parseNode(boolean isInField) throws IOException {
		if (match('=')) {
			String id = getId("object");
			if (id.equals("$"))
				return null;
			Object r = id.startsWith("$") ? named.get(id) : dom.getNamed(id);
			if (r == null)
				error("unresolved name " + id);
			expectedNewLine();
			return r;
		}
		if (match(':')) {
			int arrayIndent = indentPos;
			expectedNewLine();
			ArrayList<Object> r = new ArrayList<Object>();
			if (indentPos > arrayIndent) {
				int itemsIndent = indentPos;
				do
					r.add(parseNode(false));
				while (itemsIndent == indentPos);
			}
			return r;
		}
		if (match('"')) {
			StringBuilder r = new StringBuilder();
			for (;; next()) {
				if (cur == '"') {
					next();
					if (cur == '"')
						r.append('"');
					else
						break;
				} else if (cur == -1)
					error("string not terminated");
				else
					r.append((char) cur);
			}
			expectedNewLine();
			return r.toString();
		}
		if (isDigit(cur))
			return parseInt();
		if (cur == '-')
			return -parseInt();
		Dom.Struct r = new Dom.Struct(dom.getOrCreateStruct(getId("struct")));
		if (match('.')) {
			String id = getId("object");
			if (id.startsWith("$"))
				named.put(id,  r);
			else
				dom.setName(id, r);
		}
		int objectIndent = indentPos;
		expectedNewLine();
		if (isInField ? indentPos > objectIndent : indentPos == objectIndent) {
			int fieldIndent = indentPos;
			do {
				if (atEoln())
					break;
				String field = getId("field");
				r.type.fields.add(field);
				r.set(field, parseNode(true));
			} while (fieldIndent == indentPos);
		}
		return r;
	}

	int next() throws IOException {
		charPos++;
		return cur = in.read();
	}
	boolean atEoln() throws IOException {
		skipWs();
		return cur == '\n' || cur == '\r' || cur < 0;
	}
	void expectedNewLine() throws IOException {
		if (cur == '\n') {
			if (next() == '\r')
				next();
		} else if (cur == '\r') {
			if (next() == '\n')
				next();
		} else if (cur > 0)
			error("expected new line");
		lineNumber++;
		charPos = 1;
		if (cur == ' ' || cur == '\t') {
			if (!wasIndented) {
				wasIndented = true;
				indentWithTabs = cur == '\t';
			}
			for (; cur == ' ' || cur == '\t'; next()) {				
				if (indentWithTabs != (cur == '\t'))
					error("mixed tabs and spaces");
			}
		}
		indentPos = charPos;
	}
	void skipWs() throws IOException {
		while (cur == ' ' || cur == '\t') 
			next();
		if (cur == '#') {
			while (!atEoln())
				next();
		}
	}
	boolean match(char c) throws IOException {
		skipWs();
		if (c == cur) {
			next();
			return true;
		}
		return false;
	}
	boolean isFirstIdLetter(int c) {
		return
			(c >= 'a' && c <= 'z') ||
			(c >= 'A' && c <= 'Z') ||
			c == '$' || c == '_';
	}
	boolean isDigit(int c) {
		return c >= '0' && c <= '9';
	}
	boolean isIdLetter(int c) {
		return isFirstIdLetter(c) || isDigit(c);
	}
	String getId(String kind) throws IOException {
		skipWs();
		if (!isFirstIdLetter(cur))
			error("expected " + kind + " id");
		StringBuilder r = new StringBuilder();
		do
			r.append((char)cur);
		while (isIdLetter(next()));
		return r.toString();
	}
	long parseInt() throws IOException {
		long r = 0;
		for (; isDigit(cur); next()) {
			long n = r * 10 + cur - '0';
			if (n < r)
				error("long overflow");
			r = n;
		}
		expectedNewLine();
		return r;
	}
}
