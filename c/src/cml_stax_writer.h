#ifndef _CML_STAX_WRITER_H_
#define _CML_STAX_WRITER_H_

typedef struct cml_stax_writer_tag cml_stax_writer;

cml_stax_writer *cmlw_create(
	int (*putc)(void *context, char c),
	void *putc_context);

void cmlw_dispose(cml_stax_writer *);

enum cmlw_error_codes {
	CMLW_PUTC_ERROR = -1,
	CMLW_UNEXPECTED_FIELD = -2,
	CMLW_WRITE_AFTER_ERROR = -3,
	CMLW_UNPAIRED_END = -4,
};

int cmlw_int(cml_stax_writer *writer, const char *field, long long value);
int cmlw_bool(cml_stax_writer *writer, const char *field, int value);
int cmlw_str(cml_stax_writer *writer, const char *field, const char *value);
int cmlw_array(cml_stax_writer *writer, const char *field, int size);
int cmlw_end_array(cml_stax_writer *writer, int prev_state);
int cmlw_struct(cml_stax_writer *writer, const char *field, const char *type, const char *id);
int cmlw_end_struct(cml_stax_writer *writer, int prev_state);
int cmlw_ref(cml_stax_writer *writer, const char *field, const char *id);

#ifdef CONFIG_LIBC_FLOATINGPOINT
int cmlw_double(cml_stax_writer *writer, const char *field, double value);
#endif

#endif
