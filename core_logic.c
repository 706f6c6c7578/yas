#include "core_logic.h"

static long force(long x, long max) {
    long result = x % max;
    return result < 0 ? result + max : result;
}

char* normalize_crlf(const char* input) {
    if (!input) return strdup("");
    size_t input_len = strlen(input);
    if (input_len == 0) return strdup("");

    char* output = malloc(input_len * 2 + 1);
    if (!output) return NULL;

    size_t input_pos = 0;
    size_t output_pos = 0;

    while (input_pos < input_len) {
        char c = input[input_pos];

        if (c == '\n') {
            if (input_pos == 0 || input[input_pos - 1] != '\r') {
                output[output_pos++] = '\r';
            }
            output[output_pos++] = '\n';
            input_pos++;
        } else if (c == '\r') {
            output[output_pos++] = '\r';
            input_pos++;
            if (input_pos < input_len && input[input_pos] == '\n') {
                 output[output_pos++] = '\n';
                 input_pos++;
            } else {
                 output[output_pos++] = '\n';
            }
        } else {
            output[output_pos++] = c;
            input_pos++;
        }
    }

    output[output_pos] = '\0';

    char* final_output = realloc(output, output_pos + 1);
    if (!final_output) {
        free(output);
        return NULL;
    }

    return final_output;
}

static char* base64_encode_core(const char* input, size_t input_len, char** error_msg_out) {
    if (!input) return strdup("");
    if (input_len == 0) return strdup("");

    size_t output_len_estimate = (input_len / 3 + 1) * 4 + (input_len / (LINE_WIDTH * 3 / 4) + 1) + 1;
    char* output = malloc(output_len_estimate);
    if (!output) {
        *error_msg_out = strdup("Memory allocation failed during encoding.");
        return NULL;
    }

    const char cset[] = BASE64_ALPHABET;
    unsigned char in[3], out_val[4];
    int bytes_read;
    size_t input_pos = 0;
    size_t output_pos = 0;
    int line_pos = 0;

    while (input_pos < input_len) {
        bytes_read = 0;
        for (int i = 0; i < 3 && input_pos < input_len; ++i) {
            in[i] = input[input_pos++];
            bytes_read++;
        }
        for (int i = bytes_read; i < 3; i++) in[i] = 0;

        out_val[0] = in[0] >> 2;
        out_val[1] = ((in[0] & 0x03) << 4) | (in[1] >> 4);
        out_val[2] = ((in[1] & 0x0F) << 2) | (in[2] >> 6);
        out_val[3] = in[2] & 0x3F;

        char out_chars[4];
         for (int i = 0; i < 4; ++i) {
             out_chars[i] = cset[out_val[i]];
         }

        if (bytes_read < 3) out_chars[3] = '=';
        if (bytes_read < 2) out_chars[2] = '=';

        for (int i = 0; i < 4; i++) {
            output[output_pos++] = out_chars[i];
            if (++line_pos >= LINE_WIDTH) {
                output[output_pos++] = '\n';
                line_pos = 0;
            }
        }
    }
    if (output_pos > 0 && output[output_pos-1] != '\n') {
         output[output_pos++] = '\n';
    }
    output[output_pos] = '\0';

    char* final_output = realloc(output, output_pos + 1);
    if (!final_output) {
        free(output);
        *error_msg_out = strdup("Memory reallocation failed after encoding.");
        return NULL;
    }

    return final_output;
}

static char* base64_decode_core(const char* input, char** error_msg_out) {
    if (!input) return strdup("");
    if (strlen(input) == 0) return strdup("");

    size_t input_len = strlen(input);
    size_t output_len_estimate = input_len * 3 / 4 + 1;
    char* output = malloc(output_len_estimate);
    if (!output) {
        *error_msg_out = strdup("Memory allocation failed during decoding.");
        return NULL;
    }

    const char cset[] = BASE64_ALPHABET;
    unsigned char in_val[4], out_byte[3];
    size_t input_pos = 0;
    size_t output_pos = 0;
    int pad_count = 0;
    int current_block_pos = 0;

    while (input_pos < input_len) {
        char c = input[input_pos++];

        if (isspace((unsigned char)c)) continue;

        if (c == '=') {
             if (current_block_pos < 2) {
                 free(output);
                 const char* error_char_pos = input + input_pos - 1;
                 size_t error_offset = error_char_pos - input;
                 char msg[100];
                 sprintf(msg, "Invalid padding position at offset %zu.", error_offset);
                 *error_msg_out = strdup(msg);
                 return NULL;
             }
             pad_count++;
             in_val[current_block_pos++] = 0;
        } else if (pad_count > 0) {
             free(output);
             const char* error_char_pos = input + input_pos - 1;
             size_t error_offset = error_char_pos - input;
             char msg[100];
             sprintf(msg, "Invalid character '%c' after padding at offset %zu.", c, error_offset);
             *error_msg_out = strdup(msg);
             return NULL;
        } else {
            const char* p = strchr(cset, c);
            if (!p) {
                free(output);
                const char* error_char_pos = input + input_pos - 1;
                size_t error_offset = error_char_pos - input;
                char msg[100];
                sprintf(msg, "Invalid Base64 character '%c' at offset %zu.", c, error_offset);
                *error_msg_out = strdup(msg);
                return NULL;
            }
            in_val[current_block_pos++] = p - cset;
        }

        if (current_block_pos == 4) {
             out_byte[0] = (in_val[0] << 2) | (in_val[1] >> 4);
             out_byte[1] = (in_val[1] << 4) | (in_val[2] >> 2);
             out_byte[2] = (in_val[2] << 6) | in_val[3];

             int write_bytes = 3 - pad_count;
             for(int i = 0; i < write_bytes; ++i) {
                 output[output_pos++] = out_byte[i];
             }

             current_block_pos = 0;
             pad_count = 0;
        }
    }

    if (current_block_pos != 0 && current_block_pos != 4) {
         free(output);
         *error_msg_out = strdup("Invalid Base64 input: Data length not a multiple of 4 (after removing whitespace/valid padding).");
         return NULL;
    }

     if (current_block_pos > 0 && current_block_pos < 4) {
          free(output);
          *error_msg_out = strdup("Invalid Base64 input: Partial block at the end.");
          return NULL;
     }

    output[output_pos] = '\0';

    char* final_output = realloc(output, output_pos + 1);
     if (!final_output) {
         free(output);
         *error_msg_out = strdup("Memory reallocation failed after decoding.");
         return NULL;
     }

    return final_output;
}

char* encode_and_encrypt_string(const char* input_string, long keybase, long keyinc, char** error_msg_out) {
    *error_msg_out = NULL;

    if (keybase < 0 || keybase >= BASE64_LEN || keyinc < 0 || keyinc >= BASE64_LEN) {
        *error_msg_out = strdup("Error: Keys must be 0-63.");
        return NULL;
    }
    if (!input_string) input_string = "";

    char* base64_encoded = base64_encode_core(input_string, strlen(input_string), error_msg_out);
    if (!base64_encoded) {
        return NULL;
    }

    size_t encoded_len = strlen(base64_encoded);
    char* encrypted_output = malloc(encoded_len + 1);
     if (!encrypted_output) {
         free(base64_encoded);
         *error_msg_out = strdup("Memory allocation failed during encryption.");
         return NULL;
     }

    const char cset[] = BASE64_ALPHABET;
    long current_key = keybase;
    size_t encrypted_pos = 0;

    for (size_t i = 0; i < encoded_len; ++i) {
        char c = base64_encoded[i];
        if (c == '\n' || c == '\r') {
            encrypted_output[encrypted_pos++] = c;
            continue;
        }
         if (c == '=') {
            encrypted_output[encrypted_pos++] = '=';
            continue;
        }

        const char* p = strchr(cset, c);
        if (!p) {
             free(base64_encoded);
             free(encrypted_output);
             *error_msg_out = strdup("Internal error: Character not found in alphabet during encryption.");
             return NULL;
        }
        int val = p - cset;

        current_key = force(current_key, BASE64_LEN);
        int encrypted_val = force(val + current_key, BASE64_LEN);
        encrypted_output[encrypted_pos++] = cset[encrypted_val];
        current_key += keyinc;
    }
     encrypted_output[encrypted_pos] = '\0';

    free(base64_encoded);

    char header[64];
    sprintf(header, "yas %ld %ld\r\n\r\n", keybase, keyinc);
    size_t header_len = strlen(header);
    size_t encrypted_len_final = strlen(encrypted_output);
    size_t final_len = header_len + encrypted_len_final;
    char* final_output = malloc(final_len + 1);
     if (!final_output) {
         free(encrypted_output);
         *error_msg_out = strdup("Memory allocation failed for final output.");
         return NULL;
     }

    strcpy(final_output, header);
    strcat(final_output, encrypted_output);

    free(encrypted_output);

    return final_output;
}

char* decrypt_and_decode_string(const char* input_string, long keybase, long keyinc, char** error_msg_out) {
    *error_msg_out = NULL;

     if (keybase < 0 || keybase >= BASE64_LEN || keyinc < 0 || keyinc >= BASE64_LEN) {
        *error_msg_out = strdup("Error: Keys must be 0-63.");
        return NULL;
    }
     if (!input_string) {
         *error_msg_out = strdup("Error: No input string provided.");
         return NULL;
     }
     if (strlen(input_string) == 0) return strdup("");

    const char* data_start = input_string;
    long header_keybase_ignored = -1, header_keyinc_ignored = -1;
    int parsed_items = 0;

    const char* first_non_space = input_string;
    while(*first_non_space != '\0' && isspace((unsigned char)*first_non_space)) {
        first_non_space++;
    }

    if (strncmp(first_non_space, "yas ", 4) == 0) {
        parsed_items = sscanf(first_non_space + 4, "%ld %ld", &header_keybase_ignored, &header_keyinc_ignored);

        const char* newline1 = strchr(first_non_space, '\n');
        if (newline1) {
             if (newline1 > first_non_space && *(newline1 - 1) == '\r') {
                 const char* newline2 = strchr(newline1 + 1, '\n');
                 if (newline2 && newline2 > newline1 && *(newline2 - 1) == '\r') {
                      data_start = newline2 + 1;
                 } else {
                      *error_msg_out = strdup("Error: Malformed header format. Expected 'yas <int> <int>\\r\\n\\r\\n'. Second line break missing or not CRLF.");
                      return NULL;
                 }
             } else {
                 const char* newline2 = strchr(newline1 + 1, '\n');
                 if (newline2 && newline2 == newline1 + 1) {
                      data_start = newline2 + 1;
                 } else {
                       *error_msg_out = strdup("Error: Malformed header format. Expected 'yas <int> <int>\\n\\n' or '...\\r\\n\\r\\n'. Second line break incorrect.");
                      return NULL;
                 }
             }
        } else {
             *error_msg_out = strdup("Error: Malformed header format. Expected 'yas <int> <int>\\n\\n' or '...\\r\\n\\r\\n'. First newline is missing.");
             return NULL;
        }

        if (parsed_items != 2) {
             *error_msg_out = strdup("Error: Malformed header format. Expected 'yas <int> <int>\\n\\n' or '...\\r\\n\\r\\n'. Keys not parsed correctly.");
             return NULL;
        }
    }

    size_t data_len = strlen(data_start);
    if (data_len == 0 && data_start != input_string) {
         return strdup("");
    }
    if (data_len == 0 && data_start == input_string) {
         return strdup("");
    }

    char* decrypted_values_str = malloc(data_len + 1);
     if (!decrypted_values_str) {
         *error_msg_out = strdup("Memory allocation failed during decryption.");
         return NULL;
     }

    const char cset[] = BASE64_ALPHABET;
    long current_key = keybase;
    size_t decrypted_pos = 0;

    for (size_t i = 0; i < data_len; ++i) {
        char c = data_start[i];

        if (isspace((unsigned char)c)) {
            continue;
        }
        if (c == '=') {
             decrypted_values_str[decrypted_pos++] = '=';
             continue;
        }

        const char* p = strchr(cset, c);
        if (!p) {
             free(decrypted_values_str);
             const char* error_char_pos = data_start + i;
             size_t error_offset = (error_char_pos - input_string);
             char msg[100];
             sprintf(msg, "Error: Invalid Base64 character '%c' at offset %zu during decryption.", c, error_offset);
             *error_msg_out = strdup(msg);
             return NULL;
        }
        int val = p - cset;

        current_key = force(current_key, BASE64_LEN);
        int decrypted_val = force(val - current_key, BASE64_LEN);
        decrypted_values_str[decrypted_pos++] = cset[decrypted_val];
        current_key += keyinc;
    }
     decrypted_values_str[decrypted_pos] = '\0';

    char* final_output = base64_decode_core(decrypted_values_str, error_msg_out);

    free(decrypted_values_str);

    if (!final_output && !*error_msg_out) {
         *error_msg_out = strdup("Unknown error during Base64 decoding.");
    }

    return final_output;
}