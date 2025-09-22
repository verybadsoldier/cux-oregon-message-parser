#ifndef OREGON_PARSER_H
#define OREGON_PARSER_H

#include <stdint.h>
#include <stdbool.h>

// Structure to hold a single decoded sensor reading
typedef struct {
    char device[50];
    char type[25];
    double current;
    double average;
    char string_val[25];
    char units[15];
    char forecast[15];
    char risk[15];
} OregonReading;

// Main entry point for parsing a message
// Takes a hex string (e.g., "fa28...") and prints the decoded data.
void parse_oregon_message(const char* hex_msg);

#endif // OREGON_PARSER_H