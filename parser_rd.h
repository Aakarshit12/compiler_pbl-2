#ifndef PARSER_RD_H
#define PARSER_RD_H

#include "common.h"
#include "lexer.h"

// Recursive Descent Parser structure
typedef struct {
    Token *tokens;
    size_t num_tokens;
    size_t current_token;
    bool had_error;
    char error_message[256];
} RDParser;

// Parser functions
RDParser* parser_rd_init(Token *tokens, size_t num_tokens);
void parser_rd_free(RDParser *parser);
ASTNode* parser_rd_parse(RDParser *parser);
bool parser_rd_had_error(RDParser *parser);
const char* parser_rd_get_error(RDParser *parser);

#endif // PARSER_RD_H
