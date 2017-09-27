#ifndef _CML_STAX_WRITER_H_
#define _CML_STAX_WRITER_H_

typedef struct cml_stax_writer_tag cml_stax_writer;

cml_stax_writer *cmlw_create(
	int (*putc)(char c, void *context),
	void *putc_context);

void cmlw_dispose(cml_stax_writer *);

enum cmlw_error_codes {
	CMLW_PUTC_ERROR = -1,
	CMLW_UNEXPECTED_FIELD = -2,
};

int cmlw_int(cml_stax_writer *writer, const char *field, long long value);
int cmlw_str(cml_stax_writer *writer, const char *field, const char *value);
int cmlw_array(cml_stax_writer *writer, const char *field);
int cmlw_end_array(cml_stax_writer *writer, int prev_state);
int cmlw_struct(cml_stax_writer *writer, const char *field, const char *type, const char *id);
int cmlw_end_struct(cml_stax_writer *writer, int prev_state);
int cmlw_ref(cml_stax_writer *writer, const char *field, const char *id);

#endif
