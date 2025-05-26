#ifndef LEXER_H
#define LEXER_H

#include "common.h"

// Lexer structure
typedef struct {
    char *source;
    size_t source_len;
    size_t pos;
    size_t line;
    size_t column;
    Token *tokens;
    size_t num_tokens;
    size_t capacity;
} Lexer;

// Lexer functions
Lexer* lexer_init(const char *source);
void lexer_free(Lexer *lexer);
Token* lexer_get_tokens(Lexer *lexer, size_t *num_tokens);
bool lexer_tokenize(Lexer *lexer);
bool lexer_save_tokens(Lexer *lexer, const char *filename);
bool lexer_save_tokens_json(Lexer *lexer, const char *filename);

#endif // LEXER_H
