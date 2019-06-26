"use strict";

var cml = (function(){
	return {
		"parse" : function(data, factory, names){
			var pos = 0;
			var charPos = 0;
			var lineNumber = 0;
			var cur = 0x0d;
			var wasIndented = false;
			var indentWithTabs = false;
			var indentPos = 0;
			var wasEmptyLine = false;
			if (!names)
				names = new Map();
			
			function error(msg) {
				throw {
					"type": "error",
					"message": msg,
					"line": lineNumber,
					"pos": charPos
				};
			}
			function nextChar() {
				charPos++;
				return cur = pos < data.length ? data.charCodeAt(pos++) : 0;
			}
			function atEoln() {
				while (cur == 0x20 || cur == 0x09) 
					nextChar();
				return cur == 0x0a || cur == 0x0d;
			}
			function skipWs() {
				while (cur == 0x20 || cur == 0x09) 
					nextChar();
				if (cur == 0x3b) {//;
					while (!atEoln() && cur > 0)
						nextChar();
				}
			}
			function expectedNewLine() {
				skipWs();
				wasEmptyLine = false;
				for (;;) {
					if (cur == 0x0a) {
						if (nextChar() == 0x0d)
							nextChar();
					} else if (cur == 0x0d) {
						if (nextChar() == 0x0a)
							nextChar();
					} else if (cur > 0)
						error("expected new line");
					lineNumber++;
					charPos = 1;
					if (cur == 0x20 || cur == 0x09) {
						if (!wasIndented) {
							wasIndented = true;
							indentWithTabs = cur == 0x09;
						}
						for (; cur == 0x20 || cur == 0x09; nextChar()) {				
							if (indentWithTabs != (cur == 0x09))
								error("mixed tabs and spaces");
						}
					}
					skipWs();
					if (atEoln()) {
						wasEmptyLine = true;
						continue;
					}
					break;
				}
				indentPos =  cur > 0 ? charPos : 0;
			}
			function match(c) {
				skipWs();
				if (c == cur) {
					nextChar();
					return true;
				}
				return false;
			}
			function isFirstIdLetter(c) {
				return (c >= 0x41 && c <= 0x5a) || // A-Z
					(c >= 0x61 && c <= 0x7a) || // a-z
					c == 0x24 || c == 0x5f || c == 0x2f || c == 0x5c; // '$_/\'
			}
			function isDigit(c) {
				return c >= 0x30 && c <= 0x39; //0-9
			}
			function isIdLetter(c) {
				return isFirstIdLetter(c) || isDigit(c);
			}
			function getId(kind) {
				skipWs();
				if (!isFirstIdLetter(cur))
					error("expected " + kind + " id");
				var r = "";
				do
					r += String.fromCharCode(cur);
				while (isIdLetter(nextChar()));
				return r;
			}
			function parseInt() {
				var r = 0;
				for (; isDigit(cur); nextChar()) {
					var n = r * 10 + cur - 0x30; // '0'
					if (n < r)
						error("long overflow");
					r = n;
				}
				return r;
			}
			function parseNum(sign) {
				var r = parseInt();
				if (match(0x2e)) {  // '.'
					for (var p = 0.1; isDigit(cur); p *= 0.1, nextChar())
						r += p * (cur - 0x30); // '0'
				}
				if (match(0x65) || match(0x45)) { //Ee
					r *= Math.pow(10, (match(0x2d) ? -1 : 1) * parseInt()); //'-'
				}
				expectedNewLine();
				return r * sign;
			}
			function nextHex() {
				nextChar();
				if (cur >= 0x30 && cur <= 0x39)
					return cur - 0x30;
				if (cur >= 0x61 && cur <= 0x66)
					return cur - 0x61 + 10;
				error("unexpected hexadecimal char " + String.fromCharCode(cur));
				return 0;
			}
			function char2code(termIndent) {
				for (;;) {
					skipWs();
					var c = cur;
					if (c == 0x0a || c == 0x0d) {
						expectedNewLine();
						if (indentPos <= termIndent)
							return -1;
						continue;
					}
					nextChar();
					if (c >= 0x41 && c <= 0x5a) return c - 0x41; // A-Z
					if (c >= 0x61 && c <= 0x7a) return c - 0x61 + 26; // a-z
					if (c >= 0x30 && c <= 0x39) return c - 0x30 + 52; // 0-9
					if (c == 0x2b) return 62;//+
					if (c == 0x2f) return 63;///
					if (c < 0x20 || c == 0x3d)  return -1; //=
				}
				return -1;
			}
			function parseNode(inArray) {
				if (match(0x3d)) { //'='
					var id = getId("object id");
					expectedNewLine();
					if (id === '_')
						return null;
					var obj = names.get(id);
					if (!obj)
						obj = localNames.get(id);
					if (!obj)
						error("unresolved name " + id);
					return obj;
				}
				if (match(0x3a)) { // ':'
					if (isDigit(cur))
						parseInt();
					var arrayIndent = indentPos;
					expectedNewLine();
					var r = [];
					while (indentPos > arrayIndent) {
						r.push(parseNode(true));
						while (atEoln())
							expectedNewLine();
					}
					return r;
				}
				if (match(0x23)) { //#
					skipWs();
					var size = parseInt();
					var r = new ArrayBuffer(size);
					var arr = new Uint8Array(r);
					var arrayIndent = indentPos;
					expectedNewLine();
					for (var dst = 0;;) {
						var a = 0, b = 0;
						if ((a = char2code(arrayIndent)) < 0 || (b = char2code(arrayIndent)) < 0) break;
						arr[dst++] = a << 2 | b >> 4;
						if ((a = char2code(arrayIndent)) < 0) break;
						arr[dst++] = b << 4 | a >> 2;
						if ((b = char2code(arrayIndent)) < 0) break;
						arr[dst++] = a << 6 | b;
					}
					if (match(0x3d)) {
						while (match(0x3d)){} //=
						expectedNewLine();
					}
					return r;
				}
				if (match(0x22)) { //'"'
					var r = "";
					for (;; nextChar()) {
						if (cur == 0x5c) { //'\'
							switch (nextChar()) {
							case 0x5c: r += '\\'; break;
							case 0x22: r += '\"'; break;
							case 0x75: //u
								r += String.fromCharCode(nextHex() << 12 | nextHex() << 8 | nextHex() << 4 | nextHex());
								break;
							default:
								error("unexpected string escape "+ String.fromCharCode(cur));
							}
						} else if (cur == 0x22) {
							break;
						} else if (cur <= 0)
							error("string not terminated");
						else
							r += String.fromCharCode(cur);
					}
					nextChar();
					expectedNewLine();
					return r;
				}
				if (isDigit(cur))
					return parseNum(1);
				if (match(0x2d)) {//'-'
					if (isDigit(cur))
						return parseNum(-1);
					else {
						expectedNewLine();
						return false;
					}
				} if (match(0x2b)) {//'+'
					expectedNewLine();
					return true;
				}
				var type = getId("type");
				var id = match(0x2e) ? getId("object") : null;//'.'
				var objectIndent = indentPos;
				var effectiveIndent = inArray ? objectIndent - 1 : objectIndent;
				expectedNewLine();
				var r = factory instanceof Function ? factory(type) :
					typeof factory === "string" ? (function(){ var r = {}; r[factory]=type; return r; })() : {};
				if (id)
					names.set(id, r);
				while (!wasEmptyLine && indentPos > effectiveIndent)
					r[getId("field")] = parseNode(false);
				if (indentPos == objectIndent && inArray)
					wasEmptyLine = false;
				return r;
			}
			expectedNewLine();
			return parseNode(true);
		},
		"stringify" : function(root, typer, names){
			var out = "";
			var typeNumerator = 0;
			var revNames = new Map();
			if (names) {
				for (const i of names)
					revNames.set(i[1], i[0]);
			}
			var visited = new Map(); // undefined = not visited, 0 = referenced once, >0 referenced twice+ =id, <0 written abs() = id
			var idNumerator = 0;
			function scan(v) {
				if (Array.isArray(v)) {
					for (var i = 0; i < v.length; i++)
						scan(v[i]);
				} else if (typeof v === "object") {
					var id = visited.get(v);
					id = id === undefined ? 0 : ++idNumerator;
					visited.set(v, id);
					if (id == 0) {
						for (var field in v) {
							if (v.hasOwnProperty(field) && field != "prototype")
							   scan(v[field]);
						}
					}
				}
			}
			function getId(id, v) {
				var n = revNames.get(v);
				return n === undefined && id != 0 ? '_' + Math.abs(id) : n;
			}
			function code2char(c) {
				c &= 0x3f;
				if (c < 26) return String.fromCharCode(0x41 + c);
				if (c < 52) return String.fromCharCode(0x61 + c - 26);
				if (c < 62) return String.fromCharCode(0x30 + c - 52);
				return c == 62 ? '+' : '/';
			}
			function writeNode(v, indent, prefix, atMidArray) {
				var t = typeof v;
				if (t === 'number') {
					out += prefix + v + "\n";
				} else if (t === 'string') {
					out += prefix + '"' + v.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\u000a').replace('\r', '\\u000d').replace('\t', '\\u0009')+ '"\n';
				} else if (t === "boolean") {
					out +=  prefix + (v ? "+\n" : "-\n");					
				} else if (Array.isArray(v)) {
					out += prefix + ":" + v.length + "\n";
					indent += " ";
					for (var i = 0; i < v.length; i++)
						writeNode(v[i], indent, indent, i < v.length - 1);
				} else if (v instanceof ArrayBuffer) {
					var arr = new Uint8Array(v);
					out += prefix + "#" + arr.length;
					var src = 0;
					var srcSize = arr.length;
					var perLine = 1;
					for (; srcSize >= 3; srcSize -= 3) {
						if (--perLine == 0) {
							perLine = 32;
							out += "\n" + indent;
						}
						var a = arr[src++];
						var b = arr[src++];
						var c = arr[src++];
						out += code2char(a >> 2);
						out += code2char(a << 4 | b >> 4);
						out += code2char(b << 2 | c >> 6);
						out += code2char(c);
					}
					if (srcSize != 0) {
						var a = arr[src++];
						out += code2char(a >> 2);
						if (srcSize == 1) {
							out += code2char(a << 4);
							out += '=';
						} else {
							var b = arr[src++];
							out += code2char(a << 4 | b >> 4);
							out += code2char(b << 2);
						}
						out += '=';
					}
					out += '\n';
				} else if (t === "object") {
					var id = visited.get(v);
					if (id < 0)
						out += prefix + '=' + getId(id, v) + "\n";
					else {
						var excludedField = null;
						var t =
							typer instanceof Function ? typer(v) :
							typeof typer === "string" ? v[excludedField = typer] : null;
						if (!t) {
							t = '$' + typeNumerator++;
							excludedField = null;
						}
						out += prefix + t;
						var n = getId(id, v);
						out += n ? '.' + n + '\n' : '\n';
						if (id > 0)
							visited.set(v, -id);
						var fieldIndent = indent + ' ';
						for (var field in v) {
						   if (v.hasOwnProperty(field) && field != "prototype" && field != excludedField)
							  writeNode(v[field], fieldIndent, indent + field + ' ', false);
						}
						if (atMidArray)
							out += "\n";
					}
				} else
					throw "unexpected node";
			}
			scan(root);
			writeNode(root, "", "", false);
			return out;
		}
	};
})();
