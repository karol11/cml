package com.gmail.andrewkoder82;

import java.io.IOException;
import java.io.Reader;
import java.util.ArrayList;
import java.util.List;

public class CmlStaxReader {
	static public final int R_INT = 0;
	static public final int R_STRING = 1;
	static public final int R_STRUCT_START = 2;
	static public final int R_STRUCT_END = 3;
	static public final int R_REF = 4;
	static public final int R_ARRAY_START = 5;
	static public final int R_ARRAY_END = 6;
	static public final int R_EOF = 7;
	
	public CmlStaxReader(Reader in) throws IOException {
		this.in = in;
		cur = '\n';
		expectedNewLine();
	}

	public int next() throws IOException {
		if (indentPos < curStateIndent || cur < 0) {
			field = null;
			if (states.size() == 0)
				return R_EOF;
			boolean wasInArray = inArray;
			int s = states.remove(states.size()-1);
			inArray = (s & 1) != 0;
			curStateIndent = s >> 1;
			return wasInArray ? R_ARRAY_END : R_STRUCT_END;
		}
		field = inArray ? null : getId("field");
		if (match('=')) {
			strVal = getId("object");
			expectedNewLine();
			return R_REF;
		}
		if (match(':')) {
			int arrayIndent = indentPos;
			expectedNewLine();
			pushState(true, indentPos <= arrayIndent ? indentPos + 1 : indentPos);
			return R_ARRAY_START;
		}
		if (match('"')) {
			StringBuilder r = new StringBuilder();
			for (;; nextChar()) {
				if (cur == '"') {
					nextChar();
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
			strVal = r.toString();
			return R_STRING;
		}
		if (isDigit(cur))
			return parseInt(1);
		if (cur == '-')
			return parseInt(-1);
		type = getId("type");
		strVal = match('.') ? getId("object") : null;
		int objectIndent = indentPos;
		expectedNewLine();
		boolean hasFields = inArray ? indentPos == objectIndent : indentPos > objectIndent;
		pushState(false, hasFields ? indentPos : indentPos + 1);
		return R_STRUCT_START;
	}

	public String getField() {
		return field;
	}

	public String getType() {
		return type;
	}

	public String getStrVal() {
		return strVal;
	}

	public String getId() {
		return strVal;
	}

	public long getLongVal() {
		return longVal;
	}

	public void error(String message) {
		throw new RuntimeException(message + " at " + lineNumber + ":" + charPos);		
	}

	Reader in;
	boolean wasIndented;
	boolean indentWithTabs;
	int lineNumber = 0;
	int indentPos = 0;
	int charPos = 0;
	int cur;
	int curStateIndent = 0;
	boolean inArray = true;
	List<Integer> states = new ArrayList<Integer>();
	
	String field, type, strVal;
	long longVal;
		
	void pushState(boolean newInArray, int newIndent) {
		states.add(curStateIndent << 1 | (inArray ? 1 : 0));
		inArray = newInArray;
		curStateIndent = newIndent;
	}

	int nextChar() throws IOException {
		charPos++;
		return cur = in.read();
	}
	boolean atEoln() throws IOException {
		skipWs();
		return cur == '\n' || cur == '\r' || cur < 0;
	}
	void expectedNewLine() throws IOException {
		if (cur == '\n') {
			if (nextChar() == '\r')
				nextChar();
		} else if (cur == '\r') {
			if (nextChar() == '\n')
				nextChar();
		} else if (cur > 0)
			error("expected new line");
		lineNumber++;
		charPos = 1;
		if (cur == ' ' || cur == '\t') {
			if (!wasIndented) {
				wasIndented = true;
				indentWithTabs = cur == '\t';
			}
			for (; cur == ' ' || cur == '\t'; nextChar()) {				
				if (indentWithTabs != (cur == '\t'))
					error("mixed tabs and spaces");
			}
		}
		indentPos = charPos;
	}
	void skipWs() throws IOException {
		while (cur == ' ' || cur == '\t') 
			nextChar();
		if (cur == '#') {
			while (!atEoln())
				nextChar();
		}
	}
	boolean match(char c) throws IOException {
		skipWs();
		if (c == cur) {
			nextChar();
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
		while (isIdLetter(nextChar()));
		return r.toString();
	}
	int parseInt(int sign) throws IOException {
		long r = 0;
		for (; isDigit(cur); nextChar()) {
			long n = r * 10 + cur - '0';
			if (n < r)
				error("long overflow");
			r = n;
		}
		expectedNewLine();
		longVal = r * sign;
		return R_INT;
	}
}
