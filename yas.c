#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BASE64_ALPHABET "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
#define BASE64_LEN 64
#define LINE_WIDTH 76

#define E 1
#define D -1

static long force(long x, long max) {
    return (x % max + max) % max;
}

void base64_encode(FILE *input, FILE *output, long keybase, long keyinc) {
    const char cset[] = BASE64_ALPHABET;
    unsigned char in[3], out[4];
    int bytes_read, i, pos = 0;
    long key = keybase;

    while ((bytes_read = fread(in, 1, 3, input)) > 0) {
        // Pad with zeros if needed
        for (i = bytes_read; i < 3; i++) in[i] = 0;

        // Base64 encoding
        out[0] = in[0] >> 2;
        out[1] = ((in[0] & 0x03) << 4) | (in[1] >> 4);
        out[2] = ((in[1] & 0x0F) << 2) | (in[2] >> 6);
        out[3] = in[2] & 0x3F;

        // Encryption
        for (i = 0; i < 4; i++) {
            key = force(key, BASE64_LEN);
            out[i] = cset[force(out[i] + key, BASE64_LEN)];
            key += keyinc;
        }

        // Add padding
        if (bytes_read < 3) {
            out[3] = '=';
            if (bytes_read < 2) out[2] = '=';
        }

        // Write output
        for (i = 0; i < 4; i++) {
            fputc(out[i], output);
            if (++pos >= LINE_WIDTH) { fputc('\n', output); pos = 0; }
        }
    }
    if (pos > 0) fputc('\n', output);
}

void base64_decode(FILE *input, FILE *output, long keybase, long keyinc) {
    const char cset[] = BASE64_ALPHABET;
    unsigned char in[4], out[3];
    int i, c, bytes_read, pad_count = 0;
    long key = keybase;

    while (1) {
        // Read 4 valid characters
        bytes_read = 0;
        for (i = 0; i < 4; ) {
            c = fgetc(input);
            if (c == EOF) break;
            if (isspace(c)) continue;
            if (c == '=') pad_count++;
            in[i++] = c;
            bytes_read++;
        }

        if (bytes_read == 0) break;
        if (bytes_read != 4) {
            fprintf(stderr, "Error: Invalid Base64 input (block size != 4)\n");
            exit(1);
        }

        // Decrypt and convert to 6-bit values
        for (i = 0; i < 4; i++) {
            key = force(key, BASE64_LEN);
            char *p = strchr(cset, in[i]);
            int val = (p ? p - cset : (in[i] == '=' ? 0 : -1));
            if (val == -1) {
                fprintf(stderr, "Error: Invalid Base64 character '%c'\n", in[i]);
                exit(1);
            }
            in[i] = force(val - key, BASE64_LEN);
            key += keyinc;
        }

        // Convert 4x6-bit to 3x8-bit
        out[0] = (in[0] << 2) | (in[1] >> 4);
        out[1] = (in[1] << 4) | (in[2] >> 2);
        out[2] = (in[2] << 6) | in[3];

        // Determine output length
        int write_bytes = 3 - pad_count;
        if (write_bytes > 0) fwrite(out, 1, write_bytes, output);
        
        pad_count = 0; // Reset for next block
    }
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s (e|d) keybase keyinc\n", argv[0]);
        return 1;
    }

    int mode = (argv[1][0] == 'e') ? E : D;
    long keybase = strtol(argv[2], NULL, 10);
    long keyinc = strtol(argv[3], NULL, 10);

    if (keybase < 0 || keybase >= BASE64_LEN || keyinc < 0 || keyinc >= BASE64_LEN) {
        fprintf(stderr, "Error: Keys must be 0-%d\n", BASE64_LEN-1);
        return 1;
    }

    if (mode == E) base64_encode(stdin, stdout, keybase, keyinc);
    else base64_decode(stdin, stdout, keybase, keyinc);

    return 0;
}