#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "cul_preprocessor.h"

// ==========================================================================
// UTILITY FUNCTIONS
// ==========================================================================

// Converts a hex character ('0'-'9', 'a'-'f') to its 4-bit binary string representation.
static const char* hex_char_to_bin(char c) {
    switch(c) {
        case '0': return "0000";
        case '1': return "0001";
        case '2': return "0010";
        case '3': return "0011";
        case '4': return "0100";
        case '5': return "0101";
        case '6': return "0110";
        case '7': return "0111";
        case '8': return "1000";
        case '9': return "1001";
        case 'a': case 'A': return "1010";
        case 'b': case 'B': return "1011";
        case 'c': case 'C': return "1100";
        case 'd': case 'D': return "1101";
        case 'e': case 'E': return "1110";
        case 'f': case 'F': return "1111";
    }
    return NULL;
}

// Converts an 8-character binary string (e.g., "10101011") to a byte.
static uint8_t bin_str_to_byte(const char* bin_str) {
    return (uint8_t)strtol(bin_str, NULL, 2);
}

// Converts a raw hex string from the CUL into a bit string.
// Caller must free the returned string.
static char* raw_hex_to_bit_string(const char* hex) {
    size_t hex_len = strlen(hex);
    // Each hex char is 4 bits, +1 for null terminator
    char* bit_string = malloc(hex_len * 4 + 1);
    if (!bit_string) {
        perror("malloc failed");
        return NULL;
    }
    bit_string[0] = '\0';

    for (size_t i = 0; i < hex_len; ++i) {
        const char* bin = hex_char_to_bin(hex[i]);
        if (bin) {
            strcat(bit_string, bin);
        } else {
            // Invalid hex character
            free(bit_string);
            return NULL;
        }
    }
    return bit_string;
}

// ==========================================================================
// OREGON PROTOCOL DECODERS
// ==========================================================================

// Decodes an Oregon V2 Manchester-encoded bit stream.
static char* decode_oregon_v2(const char* bit_data) {
    const char* preamble = "10011001";
    char* preamble_pos = strstr(bit_data, preamble);

    if (!preamble_pos) {
        return NULL; // Not a valid OSV2 message
    }

    // Allocate buffers
    char decoded_bits[256] = {0};
    char final_hex[128] = {0};

    int bit_idx = 0;
    // Start after the preamble and step 16 bits (8 Manchester-encoded pairs) at a time
    for (const char* p = preamble_pos; (p - bit_data + 16) <= strlen(bit_data); p += 16) {
        char reversed_byte[9] = {0};
        
        // This loop extracts every second bit and prepends it, effectively
        // doing the Manchester decode and reversing the bit order simultaneously.
        for (int i = 1; i < 16; i += 2) {
            memmove(reversed_byte + 1, reversed_byte, 7);
            reversed_byte[0] = p[i];
        }
        
        strcat(decoded_bits, reversed_byte);
    }
    
    // Convert the decoded bits back to a hex string
    for (size_t i = 0; i < strlen(decoded_bits); i += 8) {
        char current_byte_str[9] = {0};
        strncpy(current_byte_str, decoded_bits + i, 8);
        uint8_t byte = bin_str_to_byte(current_byte_str);
        sprintf(final_hex + strlen(final_hex), "%02X", byte);
    }

    if (strlen(final_hex) == 0) {
        return NULL;
    }

    // Prepend the total bit length as a two-char hex string
    size_t total_bits = strlen(final_hex) * 4;
    char* result = malloc(strlen(final_hex) + 3);
    sprintf(result, "%02lX%s", total_bits, final_hex);

    return result;
}

// Decodes an Oregon V3 bit stream (bit-reversed bytes).
static char* decode_oregon_v3(const char* bit_data) {
    const char* preamble = "11110101";
    if (!strstr(bit_data, preamble)) {
        return NULL;
    }
    
    // Find the start of the actual data
    char* start_pos = strstr(bit_data, "0101");
    if (!start_pos) {
        return NULL;
    }

    char final_hex[128] = {0};

    // Step 8 bits at a time
    for (const char* p = start_pos; (p - bit_data + 8) <= strlen(bit_data); p += 8) {
        char byte_str[9] = {0};
        char reversed_byte_str[9] = {0};
        strncpy(byte_str, p, 8);

        // Reverse the bits in the byte
        for (int i = 0; i < 8; ++i) {
            reversed_byte_str[i] = byte_str[7 - i];
        }
        
        uint8_t byte = bin_str_to_byte(reversed_byte_str);
        sprintf(final_hex + strlen(final_hex), "%02X", byte);
    }

    if (strlen(final_hex) == 0) {
        return NULL;
    }
    
    // Prepend bit length
    size_t total_bits = strlen(final_hex) * 4;
    char* result = malloc(strlen(final_hex) + 3);
    sprintf(result, "%02lX%s", total_bits, final_hex);

    return result;
}


// ==========================================================================
// MAIN PRE-PROCESSOR FUNCTION
// ==========================================================================

char* preprocess_cul_message(const char* cul_msg) {
    // Check for "om" prefix and minimum length
    if (strncmp(cul_msg, "om", 2) != 0 || strlen(cul_msg) < 4) {
        fprintf(stderr, "Error: Invalid CUL message format. Must start with 'om'.\n");
        return NULL;
    }
    
    // The actual raw hex data starts after "om"
    const char* raw_hex = cul_msg + 2;
    
    char* bit_string = raw_hex_to_bit_string(raw_hex);
    if (!bit_string) {
        fprintf(stderr, "Error: Could not convert raw hex to bit string.\n");
        return NULL;
    }
    
    char* result = NULL;

    // Try decoding as Oregon V2
    result = decode_oregon_v2(bit_string);
    if (result) {
        free(bit_string);
        return result;
    }

    // If V2 fails, try decoding as Oregon V3
    result = decode_oregon_v3(bit_string);
    if (result) {
        free(bit_string);
        return result;
    }

    // If both fail
    free(bit_string);
    return NULL;
}