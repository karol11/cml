#include <stdio.h>
#include "../../src/cml_stax_reader.h"
#include "../../src/utf8.h"

FILE *in, *out;
cml_stax_reader *rd;
int tabs = 1;

const char *typeField;
const char *idField;
const char *refField;

int getc_in(void *unused) {
	return getc(in);
}
int putc_out(void *unused, char c) {
	return putc(c, out);
}
int getc_char_ptr(void *ctx) {
	unsigned char **c = (unsigned char**) ctx;
	return *(*c)++;
}

void indent(int i) {
	while (--i)
		putc('\t', out);
}

int traverse(int k) {
	if (k == CMLR_EOF) return 1;
	if (k == CMLR_ERROR) {
		fprintf(stderr, "error: %s at %d:%d", cmlr_error(rd), cmlr_line_num(rd), cmlr_char_pos(rd));
		return 1;
	}
	switch (k) {
	case CMLR_INT:
		fprintf(out, "%lld", cmlr_int(rd));
		break;
	case CMLR_BOOL:
		fprintf(out, "%s", cmlr_bool(rd) ?  "true" : "false");
		break;
	case CMLR_STRING:
		fputc('\"', out);
		{
			const char *src = cmlr_str(rd);
			for (;;) {
				int c = get_utf8(getc_char_ptr, (void*)&src);
				if (!c)
					break;
				if (c == '\\' || c == '\"' || c < ' ')
					fprintf(out, "\\u%04x", c);
				else 
					put_utf8(c, putc_out, 0);
			}
		}
		fputc('\"', out);
		break;
	case CMLR_START_STRUCT:
		fprintf(out, "{\r");
		indent(++tabs);
		fprintf(out, "\"%s\":\"%s\"", typeField, cmlr_type(rd));
		if (*cmlr_id(rd)) {
			fprintf(out, ",\n");
			indent(tabs);
			fprintf(out, "\"%s\":\"%s\"", idField, cmlr_id(rd));
		}
		while ((k = cmlr_next(rd)) != CMLR_END_STRUCT) {
			fprintf(out, ",\n");
			indent(tabs);
			fprintf(out, "\"%s\":", cmlr_field(rd));
			if (traverse(k))
				return 1;
		}
		fprintf(out, "\n");
		indent(--tabs);
		fprintf(out, "}");
		break;
	case CMLR_REF:
		fprintf(out, "{\"%s\":\"%s\"}", refField, cmlr_id(rd));
		break;
	case CMLR_START_ARRAY:
		{
			const char *comma = "";
			fprintf(out, "[\n");
			++tabs;
			while ((k = cmlr_next(rd)) != CMLR_END_ARRAY) {
				fprintf(out, comma);
				comma = ",\n";
				indent(tabs);
				if (traverse(k))
					return 1;
			}
			fprintf(out, "\n");
			indent(--tabs);
			fprintf(out, "]");
		}
		break;
	default:
		fprintf(stderr, "error: unexpected node %d at %d:%d", k, cmlr_line_num(rd), cmlr_char_pos(rd));
		return 1;
	}
	return 0;
}

int main(int argc, const char **args) {
	int r = 0;
	if (argc >= 3) {
		in = fopen(args[1], "r");
		out = fopen(args[2], "w");
		typeField = argc >= 4 ? args[3] : "$";
		idField = argc >= 5 ? args[4] : "#";
		refField = argc >= 6 ? args[5] : "=";

		if (!in || !out) {
			fprintf(stderr, "can't open %s", in ? args[2] : args[1]);
			return -1;
		}
		rd = cmlr_create(getc_in, 0);
		r = traverse(cmlr_next(rd));
		cmlr_dispose(rd);
		fclose(in);
		fclose(out);
	} else
		printf("usage in_cml_file out_json_file [optional_type_field] [optional_id_field] [optional_ref_field]\n");
	return r;
}
