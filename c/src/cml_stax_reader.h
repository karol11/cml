#ifndef _CML_STAX_READER_H_
#define _CML_STAX_READER_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cml_stax_reader_tag cml_stax_reader;

cml_stax_reader *cmlr_create(
	int (*getc)(void *context),
	void *getc_context);

void cmlr_dispose(cml_stax_reader *);

enum cml_reader_states {
	CMLR_INT,
	CMLR_BOOL,
	CMLR_STRING,
	CMLR_START_STRUCT,
	CMLR_END_STRUCT,
	CMLR_REF,
	CMLR_START_ARRAY,
	CMLR_END_ARRAY,
	CMLR_EOF,
	CMLR_ERROR,
	CMLR_DOUBLE,
};

int cmlr_next(cml_stax_reader *);

int cmlr_bool(cml_stax_reader *);
long long cmlr_int(cml_stax_reader *);
const char *cmlr_str(cml_stax_reader *);
const char *cmlr_type(cml_stax_reader *);
const char *cmlr_id(cml_stax_reader *);
const char *cmlr_field(cml_stax_reader *);
int cmlr_line_num(cml_stax_reader *);
int cmlr_char_pos(cml_stax_reader *);
const char *cmlr_error(cml_stax_reader *);

#ifdef CONFIG_LIBC_FLOATINGPOINT
double cmlr_double(cml_stax_reader *);
#endif

#ifdef __cplusplus
}
#endif

#endif
