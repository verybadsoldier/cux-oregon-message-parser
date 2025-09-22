#ifndef CUL_PREPROCESSOR_H
#define CUL_PREPROCESSOR_H

// Pre-processes a raw CUL message (e.g., "omAAAA...") into a clean 
// Oregon Scientific hex string (e.g., "581a89...").
// The caller is responsible for freeing the returned string.
// Returns NULL if the message cannot be decoded.
char* preprocess_cul_message(const char* cul_msg);

#endif // CUL_PREPROCESSOR_H