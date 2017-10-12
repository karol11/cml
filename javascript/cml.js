"use strict";

var cml = (function(){
	return {
		"read" : function(data, factory, names){
			
		},
		"write" : function(root, typer, names){
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
			function writeNode(v, indent, prefix) {
				var t = typeof v;
				if (t === 'number') {
					out += prefix + v + "\n";
				} else if (t === 'string') {
					out += prefix + '"' + v.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\u000a"').replace('\r', '\\u000d"') + '"\n';
				} else if (t === "boolean") {
					out +=  prefix + (v ? "+\n" : "-\n");					
				} else if (Array.isArray(v)) {
					out += prefix + ":\n";
					indent += " ";
					for (var i = 0; i < v.length; i++) {
						if (typeof v[i] === "object" && i > 0)
							out += "\n";
						writeNode(v[i], indent, indent);
					}
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
							  writeNode(v[field], fieldIndent, indent + field + ' ');
						}
					}
				} else
					throw "unexpected node";
			}
			scan(root);
			writeNode(root, "", "");
			return out;
		}
	};
})();
