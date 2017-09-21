#ifndef _CML_STAX_WRITER_H_
#define _CML_STAX_WRITER_H_

typedef struct cml_stax_writer_tag cml_stax_writer;

cml_stax_writer *cml_create_writer(
	int (*putc)(char c, void *context),
	void *putc_context);

void cml_dispose_writer(cml_stax_writer *);

int cml_write_int(cml_stax_writer *writer, const char *field, long long value);
int cml_write_str(cml_stax_writer *writer, const char *field, const char *value);
int cml_write_array(cml_stax_writer *writer, const char *field);
int cml_write_end_array(cml_stax_writer *writer, int prev_state);
int cml_write_struct(cml_stax_writer *writer, const char *field, const char *type, const char *id);
int cml_write_end_struct(cml_stax_writer *writer, int prev_state);
int cml_write_ref(cml_stax_writer *writer, const char *field, const char *id);

#endif
