#ifndef PARSER_LALR_H
#define PARSER_LALR_H

#include "common.h"
#include "lexer.h"

// LALR Parser structure
typedef struct {
    Token *tokens;
    size_t num_tokens;
    size_t current_token;
    bool had_error;
    char error_message[256];
    
    // LALR specific fields
    int *state_stack;
    int state_stack_size;
    int state_stack_capacity;
    
    // Symbol stack (for values)
    void **symbol_stack;
    int symbol_stack_size;
    int symbol_stack_capacity;
} LALRParser;

// Parser functions
LALRParser* parser_lalr_init(Token *tokens, size_t num_tokens);
void parser_lalr_free(LALRParser *parser);
ASTNode* parser_lalr_parse(LALRParser *parser);
bool parser_lalr_had_error(LALRParser *parser);
const char* parser_lalr_get_error(LALRParser *parser);

#endif // PARSER_LALR_H
