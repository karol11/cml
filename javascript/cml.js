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
				if (cur == 0x23) {//#
					while (!atEoln() && cur > 0)
						nextChar();
				}
			}
			function expectedNewLine() {
				skipWs();
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
				var hasFractionPart = match(0x2e);//'.'
				var frac = r + 0.0;
				if (hasFractionPart) {
					for (var p = 0.1; isDigit(cur); r *= 0.1, nextChar())
						frac += p * (cur - 0x30); // '0'
				}
				if (match(0x65)) { //'e'
					hasFractionPart = true;
					frac = frac * Math.pow(10, (match(0x2d) ? -1 : 1) * parseInt()); //'-'
				}
				expectedNewLine();
				if (hasFractionPart)
					return frac * sign;
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
			function parseNode(inArray) {
				if (match(0x3d)) { //'='
					var id = getId("object id");
					expectedNewLine();
					if (id === '$')
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
				if (inArray)
					objectIndent--;
				expectedNewLine();
				var r = factory instanceof Function ? factory(type) :
					typeof factory === "string" ? (function(){ var r = {}; r[factory]=type; return r; })() : {};
				if (id)
					names.set(id, r);
				while (indentPos > objectIndent)
					r[getId("field")] = parseNode(false);
				while (atEoln())
					expectedNewLine();
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
				for (var i in names)
					revNames.set(names.get(i), i);				
			}
			var visited = new Map(); // undefined = not visited, 0 = referenced once, >0 referenced twice+ =id, <0 written abs() = id
			function scan(v) {
				if (Array.isArray(v)) {
					for (var i = 0; i < v.length; i++)
						scan(v[i]);
				} else if (typeof v === "object") {
					var id = visited.get(v);
					visited.set(v, id === undefined ? 0 : id+ 1);
					for (var field in v) {
					   if (v.hasOwnProperty(field) && field != "prototype")
						  scan(v[field]);
					}					
				}
			}
			function getId(id, v) {
				var n = revNames.get(v);
				return n === undefined && id != 0 ? '$' + Math.abs(id) : n;
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
				} else if (t === "object") {
					var id = visited.get(v);
					if (id < 0)
						out += prefix + '=' + getId(id, v);
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
