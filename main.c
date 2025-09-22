#include <stdio.h>
#include <stdlib.h> // For free()
#include "cul_preprocessor.h" // Our new pre-processor
#include "oregon_parser.h"    // Our original parser

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <raw_cul_message>\n", argv[0]);
        fprintf(stderr, "Example: %s omAAAAAAAB32D4CB3554D54CAB5554B53554B54D4D555414\n", argv[0]);
        return 1;
    }

    printf("--- Stage 1: Pre-processing CUL Message ---\n");
    printf("Raw Input: %s\n", argv[1]);

    // Step 1: Call the pre-processor to decode the Manchester stream
    char* oregon_hex_string = preprocess_cul_message(argv[1]);

    if (!oregon_hex_string) {
        fprintf(stderr, "Failed to decode CUL message. It's not a recognized Oregon V2 or V3 protocol.\n");
        return 1;
    }

    printf("Preprocessor Output: %s\n\n", oregon_hex_string);

    printf("--- Stage 2: Parsing Oregon Data ---\n");
    
    // Step 2: Pass the clean hex string to our original parser
    parse_oregon_message(oregon_hex_string);

    // IMPORTANT: Free the memory allocated by the pre-processor
    free(oregon_hex_string);

    return 0;
}