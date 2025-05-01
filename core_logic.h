#ifndef CORE_LOGIC_H
#define CORE_LOGIC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BASE64_ALPHABET "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
#define BASE64_LEN 64
#define LINE_WIDTH 76

char* normalize_crlf(const char* input);
char* encode_and_encrypt_string(const char* input_string, long keybase, long keyinc, char** error_msg_out);
char* decrypt_and_decode_string(const char* input_string, long keybase, long keyinc, char** error_msg_out);

#endif