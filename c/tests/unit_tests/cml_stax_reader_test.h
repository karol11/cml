#ifndef _CML_STAX_READER_TEST_H_
#define _CML_STAX_READER_TEST_H_

#include "../../src/tests.h"
#include "../../src/cml_stax_reader.h"
#include "../../src/string_builder.h"

static int getc_asciiz(void *context) {
	unsigned char **c = (unsigned char**) context;
	return *(*c)++;
}

static void dump(cml_stax_reader *r, string_builder *s) {
	char itoa_buf[32];
	for (;;) {
		int t = cmlr_next(r);
		if (*cmlr_field(r)) {
			sb_puts(s, cmlr_field(r));
			sb_append(s, ':');
		}
		switch(t) {
		case CMLR_INT: sb_puts(s, _ltoa((long)cmlr_int(r), itoa_buf, 10)); break;
		case CMLR_BOOL: sb_puts(s, cmlr_bool(r) ? "true" : "false"); break;
		case CMLR_DOUBLE:
			sprintf(itoa_buf, "%lg", cmlr_double(r));
			sb_puts(s, itoa_buf);
			break;
		case CMLR_STRING:
			sb_append(s, '\'');
			sb_puts(s, cmlr_str(r));
			sb_append(s, '\'');
			break;
		case CMLR_BINARY:
			{
				int i;
				int size = cmlr_size(r);
				char *buf = (char*)malloc(size);
				cmlr_binary(r, buf);
				for (i = 0; i < size; i++) {
					sprintf(itoa_buf, "%02x", buf[i] & 0xff);
					sb_puts(s, itoa_buf);
				}
				free(buf);
			}
			break;
		case CMLR_START_STRUCT:
			if (*cmlr_id(r)){
				sb_puts(s, cmlr_id(r));
				sb_append(s, '=');
			}
			sb_puts(s, cmlr_type(r));
			sb_append(s, '{');
			break;
		case CMLR_END_STRUCT: sb_append(s, '}'); break;
		case CMLR_REF:
			sb_append(s, '=');
			sb_puts(s, cmlr_id(r));
			break;
		case CMLR_START_ARRAY:  sb_append(s, '['); break;
		case CMLR_END_ARRAY:  sb_append(s, ']'); break;
		case CMLR_EOF: return;
		case CMLR_ERROR:
			sb_append(s, '?');
			sb_puts(s, cmlr_error(r));
			sb_puts(s, _itoa(cmlr_line_num(r), itoa_buf, 10)); 
			sb_append(s, ':');
			sb_puts(s, _itoa(cmlr_char_pos(r), itoa_buf, 10)); 
			return;
		}
	}
}

static void test(char *cml, char *expected_dump) {
	cml_stax_reader *r = cmlr_create(getc_asciiz, &cml);
	string_builder sb;
	sb_init(&sb);
	dump(r, &sb);
	cmlr_dispose(r);
	ASSERT(strcmp(sb_get_str(&sb), expected_dump) == 0);
	sb_dispose(&sb);
}

void cml_stax_reader_test() {
	test(
		"Page\n"
		"items:\n"
		"	Page.header ;comment\n"
		"	align 1 ; comment\n"
		"	size 20\n"
		"	items: ;comment\n"
		"		Image.logo\n"
		"		align 2\n"
		"		size 20\n"
		"		url \"logo.gif\"\n"
		"\n"
		"		TextBox.title\n"
		"		content:\n"
		"			Span\n"
		"			text \"Title\"\n"
		"			style TextStyle\n"
		"				parent TextStyle.main_style\n"
		"					family \"Arial\"\n"
		"					weight 400\n"
		"					size 12\n"
		"					color 0\n"
		"				size 24\n"
		"				color 16436877\n"
		"\n"
		"	TextBox.mainText\n"
		"	content:\n"
		"		Span\n"
		"		text \"Hello \"\n"
		"		style=main_style\n"
		"\n"
		"		Span\n"
		"		text \"world!\"\n"
		"		style TextStyle.bold\n"
		"			weight 600\n"
		"			parent=main_style\n",
		
		"Page{items:[header=Page{align:1size:20items:[logo=Image{align:2size:20url:'logo.gif'}"
		"title=TextBox{content:[Span{text:'Title'style:TextStyle{parent:main_style=TextStyle{"
		"family:'Arial'weight:400size:12color:0}size:24color:16436877}}]}]}mainText=TextBox{"
		"content:[Span{text:'Hello 'style:=main_style}Span{text:'world!'style:bold=TextStyle{"
		"weight:600parent:=main_style}}]}]}");

	test(
		":\n"
		"   +\n"
		"   3.14\n"
		"   22\n"
		"   \"\"\n"
		"   point.p\n"
		"   x 1\n"
		"   y 2\n"
		"\n"
		"   +\n"
		"   =p\n"
		"   :\n"
		"     +\n"
		"     -\n",
		"[true3.1422''p=point{x:1y:2}true=p[truefalse]]");

	test(
		"#4\n"
		" /8AMq\n"
		" g==",
		"ffc00caa");
}

#endif
