#ifndef _CML_DOM_READER_TEST_H_
#define _CML_DOM_READER_TEST_H_

#include "../src/tests.h"
#include "../src/cml_dom_reader.h"
#include "dom_test.h"

void cml_dom_reader_test() {
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
		"		url \"logo.gif\\\\\\\"\"\n"
		"\n"
		"		TextBox.title\n"
		"		content:\n"
		"			Span\n"
		"			text \"\\u000d\\u000a\\u0411\\uffffTitle\"\n"
		"			style TextStyle\n"
		"				parent TextStyle.main_style\n"
		"					family \"Arial\"\n"
		"					weight 400\n"
		"					size 12.15\n"
		"					color 0\n"
		"					italic -\n"
		"				size 240e-2\n"
		"				color 16436877\n"
		"\n"
		"	TextBox.mainText\n"
		"	content:\n"
		"		Span\n"
		"		text \"Hello \"\n"
		"		style=main_style\n"
		"\n"
		"		Span\n"
		"		text \"world!\\\\\\\"\"\n"
		"		style TextStyle.bold\n"
		"			weight 600\n"
		"			italic +\n"
		"			parent=main_style\n";
	d_dom *d = cml_read(getc_asciiz, &t, 0, 0);
	string_builder sb;
	sb_init(&sb);
	to_string(&sb, d_root(d), 0);
	ASSERT(strcmp(sb_get_str(&sb),
		"Page{items:[Page:header{size:20align:1items:[Image:logo{url:\"logo.gif\\\"\"size:20align:2},"
		"TextBox:title{content:[Span{style:TextStyle{color:16436877size:2.4parent:TextStyle:main_style{"
		"italic:falsecolor:0size:12.15weight:400family:\"Arial\"}}text:\"\x0d\x0a\xd0\x91\xEF\xBF\xBFTitle\"}]}]},"
		"TextBox:mainText{content:[Span{style:=main_styletext:\"Hello \"},Span{style:TextStyle:bold{"
		"italic:trueweight:600parent:=main_style}text:\"world!\\\"\"}]}]}") == 0);
	d_dispose_dom(d);
	sb_dispose(&sb);
}

#endif
