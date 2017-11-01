package com.gmail.andrewkoder82;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.Reader;
import java.util.ArrayList;
import java.util.List;

public class CmlStaxReader {
	static public final int R_LONG = 0;
	static public final int R_STRING = 1;
	static public final int R_STRUCT_START = 2;
	static public final int R_STRUCT_END = 3;
	static public final int R_REF = 4;
	static public final int R_ARRAY_START = 5;
	static public final int R_ARRAY_END = 6;
	static public final int R_BOOL = 7;
	static public final int R_DOUBLE = 8;
	static public final int R_BINARY = 9;
	static public final int R_EOF = 9;

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
	int sizeVal;
	double dblVal;
	boolean boolVal;
	byte[] binVal;

	public CmlStaxReader(Reader in) throws IOException {
		this.in = in;
		cur = '\n';
		expectedNewLine();
	}

	public int next() throws IOException {
		while (cur == ';') {
			skipWs();
			expectedNewLine();
		}
		boolean emptyLine = cur == '\n' || cur == '\r';
		if (emptyLine) {
			do {
				expectedNewLine();
				skipWs();
			} while (cur == '\n' || cur == '\r');
			for (int i = states.size(); --i >= 0 && states.get(i) >> 1 >= indentPos;) {
				int s = states.get(i);
				if (s >> 1 == indentPos && (s & 1) == 0) {
					states.set(i, s + 2);
					break;
				}
			}
		}
		if (indentPos < curStateIndent || cur <= 0 || emptyLine) {
			field = null;
			if (states.isEmpty()) {
				if (cur > 0)
					error("text after eof");
				return R_EOF; 
			}
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
			sizeVal = isDigit(cur) ? (int)parseInt() : -1;
			expectedNewLine();
			pushState(true, indentPos <= arrayIndent ? indentPos + 1 : indentPos);
			return R_ARRAY_START;
		}
		if (match('#')) {
			int arrayIndent = indentPos;
			skipWs();
			ByteArrayOutputStream r = new ByteArrayOutputStream((int)parseInt());
			expectedNewLine();
			for (;;) {
				int a, b;
				if ((a = base64char2code(arrayIndent)) < 0 || (b = base64char2code(arrayIndent)) < 0) break;
				r.write(a << 2 | b >> 4);
				if ((a = base64char2code(arrayIndent)) < 0) break;
				r.write(b << 4 | a >> 2);
				if ((b = base64char2code(arrayIndent)) < 0) break;
				r.write(a << 6 | b);
			}
			binVal = r.toByteArray();
			return R_BINARY;
		}
		if (match('"')) {
			StringBuilder r = new StringBuilder();
			for (;; nextChar()) {
				if (cur == '\\') {
					switch (nextChar()) {
					case '\\': r.append('\\'); break;
					case '\"': r.append('\"'); break;
					case 'u':
						r.append((char) (nextHex() << 12 | nextHex() << 8 | nextHex() << 4 | nextHex()));
						break;
					default:
						error("unexpected string escape "+ (char)cur);
					}
				} else if (cur == '"') {
					break;
				} else if (cur == -1)
					error("string not terminated");
				else
					r.append((char) cur);
			}
			nextChar();
			expectedNewLine();
			strVal = r.toString();
			return R_STRING;
		}
		if (isDigit(cur))
			return parseNum(1);
		if (match('-')) {
			if (isDigit(cur))
				return parseNum(-1);
			else {
				boolVal = false;
				expectedNewLine();
				return R_BOOL;
			}
		} if (match('+')) {
			boolVal = true;
			expectedNewLine();
			return R_BOOL;			
		}
		type = getId("type");
		strVal = match('.') ? getId("object") : null;
		int objectIndent = indentPos;
		expectedNewLine();
		boolean hasFields = inArray ? indentPos == objectIndent : indentPos > objectIndent;
		pushState(false, hasFields ? indentPos : indentPos + 1);
		return R_STRUCT_START;
	}

	private int nextHex() throws IOException {
		nextChar();
		if (cur >= '0' && cur <= '9')
			return cur - '0';
		if (cur >= 'a' && cur <= 'f')
			return cur - 'a' + 10;
		error("unexpected hexadecimal char " + (char) cur);
		return 0;
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

	public double getDblVal() {
		return dblVal;
	}

	public boolean getBoolVal() {
		return boolVal;
	}
	
	public int getSize() {
		return sizeVal;
	}
	public byte[] getBinary() {
		return binVal;
	}

	public void error(String message) {
		throw new RuntimeException(message + " at " + lineNumber + ":" + charPos);		
	}

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
		skipWs();
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
		if (cur == ';') {
			while (cur != '\n' && cur != '\r' && cur > 0)
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
			c == '$' || c == '_' || c == '/' || c == '\\';
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
	long parseInt() throws IOException {
		long r = 0;
		for (; isDigit(cur); nextChar()) {
			long n = r * 10 + cur - '0';
			if (n < r)
				error("long overflow");
			r = n;
		}
		return r;
	}
	int parseNum(int sign) throws IOException {
		long r = parseInt();
		for (; isDigit(cur); nextChar()) {
			long n = r * 10 + cur - '0';
			if (n < r)
				error("long overflow");
			r = n;
		}
		boolean hasFractionPart = match('.');
		double frac = r;
		if (hasFractionPart) {
			for (double p = 0.1; isDigit(cur); r *= 0.1, nextChar())
				frac += p * (cur - '0');
		}
		if (match('e')) {
			hasFractionPart = true;
			frac = frac * Math.pow(10, (match('-') ? -1 : 1) * parseInt());
		}
		expectedNewLine();
		if (hasFractionPart) {
			dblVal = frac * sign;
			return R_DOUBLE;
		}
		longVal = r * sign;
		return R_LONG;
	}
	int base64char2code(int basePos) throws IOException {
		for (;;) {
			skipWs();
			int c = cur;
			if (c == '\n' || c == '\r') {
				expectedNewLine();
				if (indentPos <= basePos)
					return -1;
				continue;
			}
			nextChar();
			if (c >= 'A' && c <= 'Z') return c - 'A';
			if (c >= 'a' && c <= 'z') return c - 'a' + 26;
			if (c >= '0' && c <= '9') return c - '0' + 52;
			if (c == '+') return 62;
			if (c == '/') return 63;
			if (c <= 0 || c == '=') return -1;
		}
	}
}
