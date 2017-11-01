#include "cml_config.h"
#include "utf8.h"

int get_utf8(int (*get_fn)(void *context), void *get_fn_context)
{
	int r, n;
restart_and_reload:
	r = get_fn(get_fn_context);
restart:
	n = 2;
	if (r <= 0)
		return r;
	if ((r & 0x80) == 0)
		return r;
	if ((r & 0xe0) == 0xc0) r &= 0x1f;
	else if ((r & 0xf0) == 0xe0) n = 3, r &= 0xf;
	else if ((r & 0xf8) == 0xf0) n = 4, r &= 7;
	else
		goto restart_and_reload;
	while (--n) {
		int c = get_fn(get_fn_context);
		if ((c & 0xc0) != 0x80) {
			if (c <= 0)
				return c;
			r = c;
			goto restart;
		}
		r = r << 6 | (c & 0x3f);
	}
	while (r >= 0xD800 && r <= 0xDBFF) { // it's ill-formed surrogate from utf16->utf8
		int low_part = get_utf8(get_fn_context, get_fn);
		if (low_part < 0xDC00 || low_part > 0xDFFF)
			r = low_part; // bad utf16 sequence
		else {
			return ((r & 0x3ff) << 10 | (low_part & 0x3ff)) | 0x10000;
		}
	}
	return r;
}

int put_utf8(int v, int (*put_fn)(void *context, char ch), void *put_fn_context)
{
	if (v <= 0x7f)
		return put_fn(put_fn_context, v);
	else {
		int r;
		if (v <= 0x7ff)
			r = put_fn(put_fn_context, v >> 6 | 0xc0);
		else {
			if (v <= 0xffff)
				r = put_fn(put_fn_context, v >> (6 + 6) | 0xe0);
			else {
				if (v <= 0x10ffff)
					r = put_fn(put_fn_context, v >> (6 + 6 + 6) | 0xf0);
				else
					return 0;
				if (r > 0)
					r = put_fn(put_fn_context, ((v >> (6 + 6)) & 0x3f) | 0x80);
			}
			if (r > 0)
				r = put_fn(put_fn_context, ((v >> 6) & 0x3f) | 0x80);
		}
		if (r > 0)
			r = put_fn(put_fn_context, (v & 0x3f) | 0x80);
		return r;
	}
}
