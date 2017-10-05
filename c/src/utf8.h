#ifndef _UTF8_H_
#define _UTF8_H_

//
// Decodes a single unicode symbol from the sequence of bytes containing UTF-8 character representation.
// Also decodes utf16 surrogate pairs if they are transcoded by utf16->utf8 converter.
// Skips all ill-formed sequences.
//
// get_fn - a function returning the next byte of sequence.
//		If it returns 0 or negative number, decoding stops and get_utf8 returns its value.
// get_fn_context - context to be passed to get_fn.
// Usage example: int a = get_utf8(fgetc, fopen("text", "r"));
// For more examples see utf8_tests.
//
int get_utf8(int (*get_fn)(void *context), void *get_fn_context);

//
// Encodes a single unicode character as a sequence of bytes in UTF-8 encoding.
// character - in range 0..0x10ffff
// put_fn - function to be called to store bytes.
//    If it returs <= 0, encoding is terminated
// put_fn_context - cntext data to be passed to put_fn.
// Returns the result of last put_fn call, or 0 if the character out of allowed range.
// For examples see utf8_tests.
//
int put_utf8(int character, int (*put_fn)(void *context, char byte), void *put_fn_context);

#endif
