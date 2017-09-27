#ifndef _CML_STAX_READER_TEST_H_
#define _CML_STAX_READER_TEST_H_

#include "../src/tests.h"
#include "../src/cml_stax_reader.h"
#include "../src/string_builder.h"

int getc_asciiz(void *context) {
	char **c = (char**) context;
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
		case CMLR_STRING:
			sb_append(s, '\'');
			sb_puts(s, cmlr_str(r));
			sb_append(s, '\'');
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
void cml_stax_reader_test() {
	char *t =
		"Page\n"
		"items:\n"
		"	Page.header\n"
		"	align 1\n"
		"	size 20\n"
		"	items:\n"
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
		"			parent=main_style\n";
	cml_stax_reader *r = cmlr_create(getc_asciiz, &t);
	string_builder sb;
	sb_init(&sb);
	dump(r, &sb);
	ASSERT(strcmp(sb_get_str(&sb),
		"Page{items:[header=Page{align:1size:20items:[logo=Image{align:2size:20url:'logo.gif'}"
		"title=TextBox{content:[Span{text:'Title'style:TextStyle{parent:main_style=TextStyle{"
		"family:'Arial'weight:400size:12color:0}size:24color:16436877}}]}]}mainText=TextBox{"
		"content:[Span{text:'Hello 'style:=main_style}Span{text:'world!'style:bold=TextStyle{"
		"weight:600parent:=main_style}}]}]}") == 0);
	cmlr_dispose(r);
	sb_dispose(&sb);
}

#endif
