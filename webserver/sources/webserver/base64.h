#ifndef BASE64_H
#define BASE64_H

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdint.h>

char *base64_encode(const unsigned char *data, size_t input_length, size_t *output_length);
unsigned char *base64_decode(const unsigned char *data, size_t input_length, size_t *output_length);
void build_decoding_table();
void base64_cleanup();

#endif // BASE64_H

