#include <stdio.h>
#include "cml_stax_reader.h"

FILE *in, *out;
cml_stax_reader *rd;
int tabs = 1;

int getc_stdin(void *unused) {
	return getc(in);
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
	case CMLR_STRING:
		fprintf(out, "\"%s\"", cmlr_str(rd));
		break;
	case CMLR_START_STRUCT:
		fprintf(out, "{\r");
		indent(++tabs);
		fprintf(out, "\"$\":\"%s\"", cmlr_type(rd));
		if (*cmlr_id(rd)) {
			fprintf(out, ",\n");
			indent(tabs);
			fprintf(out, "\"#\":\"%s\"", cmlr_id(rd));
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
		fprintf(out, "{\"=\":\"%s\"}", cmlr_id(rd));
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
	if (argc == 3) {
		in = fopen(args[1], "rt");
		out = fopen(args[2], "wt");
		if (!in || !out) {
			fprintf(stderr, "can't open %s", in ? args[2] : args[1]);
			return -1;
		}
		rd = cmlr_create(getc_stdin, 0);
		r = traverse(cmlr_next(rd));
		cmlr_dispose(rd);
		fclose(in);
		fclose(out);
	} else
		printf("usage in_cml_file out_json_file\n");
	return r;
}
