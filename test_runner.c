#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cul_preprocessor.h"
#include "oregon_parser.h"

// Define the filename for test data and maximum line length
#define TEST_DATA_FILE "test_data.txt"
#define MAX_LINE_LENGTH 256

/**
 * @brief Reads CUL messages from a file and runs them through the full decoding pipeline.
 */
void run_tests() {
    FILE *file = fopen(TEST_DATA_FILE, "r");
    if (!file) {
        perror("Error: Could not open test_data.txt");
        return;
    }

    char line[MAX_LINE_LENGTH];
    int test_num = 1;

    printf("Starting Oregon message test suite...\n\n");

    // Read the test data file line by line
    while (fgets(line, sizeof(line), file)) {
        // Remove trailing newline character, if any
        line[strcspn(line, "\r\n")] = 0;

        if (strlen(line) == 0) {
            continue; // Skip empty lines
        }

        printf("==================== TEST CASE %d ====================\n", test_num);
        printf("Raw Input: %s\n\n", line);

        // --- Stage 1: Pre-processing ---
        printf("--- Stage 1: Pre-processing CUL Message ---\n");
        char* oregon_hex_string = preprocess_cul_message(line);

        if (!oregon_hex_string) {
            printf("Result: PREPROCESS FAILED. Not a recognized Oregon V2/V3 protocol.\n");
        } else {
            printf("Preprocessor Output: %s\n", oregon_hex_string);
            printf("Result: PREPROCESS SUCCEEDED.\n\n");
            
            // --- Stage 2: Parsing ---
            printf("--- Stage 2: Parsing Oregon Data ---\n");
            parse_oregon_message(oregon_hex_string);
            
            // Free the memory allocated by the pre-processor
            free(oregon_hex_string);
        }
        
        printf("================== END TEST CASE %d ==================\n\n", test_num);
        test_num++;
    }

    fclose(file);
    printf("Test suite finished.\n");
}

int main(int argc, char* argv[]) {
    run_tests();
    return 0;
}
