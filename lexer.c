#include "lexer.h"
#include <ctype.h>

// Keywords in our C-like language
static const char *keywords[] = {
    "if", "else", "while", "for", "return", "int", "float", "char", "void", "struct",
    "break", "continue", "switch", "case", "default", "do", "const", "static", NULL
};

// Initialize the lexer with source code
Lexer* lexer_init(const char *source) {
    Lexer *lexer = (Lexer*)malloc(sizeof(Lexer));
    if (!lexer) return NULL;
    
    lexer->source = strdup(source);
    lexer->source_len = strlen(source);
    lexer->pos = 0;
    lexer->line = 1;
    lexer->column = 1;
    
    // Initialize token array
    lexer->capacity = 128;  // Initial capacity
    lexer->tokens = (Token*)malloc(sizeof(Token) * lexer->capacity);
    lexer->num_tokens = 0;
    
    if (!lexer->source || !lexer->tokens) {
        lexer_free(lexer);
        return NULL;
    }
    
    return lexer;
}

// Free lexer resources
void lexer_free(Lexer *lexer) {
    if (!lexer) return;
    
    if (lexer->source) {
        free(lexer->source);
    }
    
    if (lexer->tokens) {
        // Free each token's value
        for (size_t i = 0; i < lexer->num_tokens; i++) {
            if (lexer->tokens[i].value) {
                free(lexer->tokens[i].value);
            }
        }
        free(lexer->tokens);
    }
    
    free(lexer);
}

// Check if a string is a keyword
static bool is_keyword(const char *str) {
    for (int i = 0; keywords[i] != NULL; i++) {
        if (strcmp(str, keywords[i]) == 0) {
            return true;
        }
    }
    return false;
}

// Add a token to the lexer's token array
static void add_token(Lexer *lexer, TokenType type, const char *value, int line, int column) {
    // Resize token array if needed
    if (lexer->num_tokens >= lexer->capacity) {
        lexer->capacity *= 2;
        Token *new_tokens = (Token*)realloc(lexer->tokens, sizeof(Token) * lexer->capacity);
        if (!new_tokens) return;
        lexer->tokens = new_tokens;
    }
    
    // Add the new token
    Token *token = &lexer->tokens[lexer->num_tokens++];
    token->type = type;
    token->value = strdup(value);
    token->line = line;
    token->column = column;
}

// Get the current character
static char current_char(Lexer *lexer) {
    if (lexer->pos >= lexer->source_len) return '\0';
    return lexer->source[lexer->pos];
}

// Peek at the next character without advancing
static char peek_char(Lexer *lexer) {
    if (lexer->pos + 1 >= lexer->source_len) return '\0';
    return lexer->source[lexer->pos + 1];
}

// Advance to the next character
static void advance(Lexer *lexer) {
    if (current_char(lexer) == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    lexer->pos++;
}

// Skip whitespace
static void skip_whitespace(Lexer *lexer) {
    while (isspace(current_char(lexer))) {
        advance(lexer);
    }
}

// Skip comments
static void skip_comment(Lexer *lexer) {
    // Single line comment
    if (current_char(lexer) == '/' && peek_char(lexer) == '/') {
        while (current_char(lexer) != '\n' && current_char(lexer) != '\0') {
            advance(lexer);
        }
    }
    // Multi-line comment
    else if (current_char(lexer) == '/' && peek_char(lexer) == '*') {
        advance(lexer); // Skip /
        advance(lexer); // Skip *
        
        while (!(current_char(lexer) == '*' && peek_char(lexer) == '/') && 
               current_char(lexer) != '\0') {
            advance(lexer);
        }
        
        if (current_char(lexer) != '\0') {
            advance(lexer); // Skip *
            advance(lexer); // Skip /
        }
    }
}

// Tokenize an identifier or keyword
static void tokenize_identifier(Lexer *lexer) {
    size_t start_pos = lexer->pos;
    int start_line = lexer->line;
    int start_column = lexer->column;
    
    while (isalnum(current_char(lexer)) || current_char(lexer) == '_') {
        advance(lexer);
    }
    
    size_t length = lexer->pos - start_pos;
    char *value = (char*)malloc(length + 1);
    if (!value) return;
    
    strncpy(value, lexer->source + start_pos, length);
    value[length] = '\0';
    
    TokenType type = is_keyword(value) ? TOKEN_KEYWORD : TOKEN_IDENTIFIER;
    add_token(lexer, type, value, start_line, start_column);
    
    free(value);
}

// Tokenize a number
static void tokenize_number(Lexer *lexer) {
    size_t start_pos = lexer->pos;
    int start_line = lexer->line;
    int start_column = lexer->column;
    
    bool has_decimal = false;
    
    while (isdigit(current_char(lexer)) || 
           (current_char(lexer) == '.' && !has_decimal)) {
        if (current_char(lexer) == '.') {
            has_decimal = true;
        }
        advance(lexer);
    }
    
    size_t length = lexer->pos - start_pos;
    char *value = (char*)malloc(length + 1);
    if (!value) return;
    
    strncpy(value, lexer->source + start_pos, length);
    value[length] = '\0';
    
    add_token(lexer, TOKEN_NUMBER, value, start_line, start_column);
    
    free(value);
}

// Tokenize a string
static void tokenize_string(Lexer *lexer) {
    int start_line = lexer->line;
    int start_column = lexer->column;
    
    advance(lexer); // Skip opening quote
    
    size_t start_pos = lexer->pos;
    while (current_char(lexer) != '"' && current_char(lexer) != '\0') {
        // Handle escape sequences
        if (current_char(lexer) == '\\' && peek_char(lexer) != '\0') {
            advance(lexer);
        }
        advance(lexer);
    }
    
    size_t length = lexer->pos - start_pos;
    char *value = (char*)malloc(length + 1);
    if (!value) return;
    
    strncpy(value, lexer->source + start_pos, length);
    value[length] = '\0';
    
    add_token(lexer, TOKEN_STRING, value, start_line, start_column);
    
    free(value);
    
    if (current_char(lexer) == '"') {
        advance(lexer); // Skip closing quote
    }
}

// Tokenize an operator
static void tokenize_operator(Lexer *lexer) {
    int start_line = lexer->line;
    int start_column = lexer->column;
    
    char op[3] = {0};
    op[0] = current_char(lexer);
    advance(lexer);
    
    // Check for two-character operators
    if ((op[0] == '+' && current_char(lexer) == '+') ||
        (op[0] == '-' && current_char(lexer) == '-') ||
        (op[0] == '=' && current_char(lexer) == '=') ||
        (op[0] == '!' && current_char(lexer) == '=') ||
        (op[0] == '<' && current_char(lexer) == '=') ||
        (op[0] == '>' && current_char(lexer) == '=') ||
        (op[0] == '&' && current_char(lexer) == '&') ||
        (op[0] == '|' && current_char(lexer) == '|') ||
        (op[0] == '+' && current_char(lexer) == '=') ||
        (op[0] == '-' && current_char(lexer) == '=') ||
        (op[0] == '*' && current_char(lexer) == '=') ||
        (op[0] == '/' && current_char(lexer) == '=')) {
        
        op[1] = current_char(lexer);
        advance(lexer);
    }
    
    add_token(lexer, TOKEN_OPERATOR, op, start_line, start_column);
}

// Tokenize punctuation
static void tokenize_punctuation(Lexer *lexer) {
    int start_line = lexer->line;
    int start_column = lexer->column;
    
    char punct[2] = {0};
    punct[0] = current_char(lexer);
    
    add_token(lexer, TOKEN_PUNCTUATION, punct, start_line, start_column);
    
    advance(lexer);
}

// Main tokenization function
bool lexer_tokenize(Lexer *lexer) {
    while (lexer->pos < lexer->source_len) {
        char c = current_char(lexer);
        
        // Skip whitespace
        if (isspace(c)) {
            skip_whitespace(lexer);
            continue;
        }
        
        // Skip comments
        if (c == '/' && (peek_char(lexer) == '/' || peek_char(lexer) == '*')) {
            skip_comment(lexer);
            continue;
        }
        
        // Identifiers and keywords
        if (isalpha(c) || c == '_') {
            tokenize_identifier(lexer);
        }
        // Numbers
        else if (isdigit(c)) {
            tokenize_number(lexer);
        }
        // Strings
        else if (c == '"') {
            tokenize_string(lexer);
        }
        // Operators
        else if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' ||
                 c == '=' || c == '<' || c == '>' || c == '!' ||
                 c == '&' || c == '|' || c == '^' || c == '~') {
            tokenize_operator(lexer);
        }
        // Punctuation
        else if (c == '(' || c == ')' || c == '{' || c == '}' ||
                 c == '[' || c == ']' || c == ';' || c == ',' ||
                 c == '.' || c == ':' || c == '?') {
            tokenize_punctuation(lexer);
        }
        // Unknown
        else {
            char unknown[2] = {c, '\0'};
            add_token(lexer, TOKEN_UNKNOWN, unknown, lexer->line, lexer->column);
            advance(lexer);
        }
    }
    
    // Add EOF token
    add_token(lexer, TOKEN_EOF, "", lexer->line, lexer->column);
    
    return true;
}

// Get the tokens
Token* lexer_get_tokens(Lexer *lexer, size_t *num_tokens) {
    if (num_tokens) {
        *num_tokens = lexer->num_tokens;
    }
    return lexer->tokens;
}

// Save tokens to a text file
bool lexer_save_tokens(Lexer *lexer, const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) return false;
    
    // Write header
    fprintf(file, "%-15s %-15s %-10s %-10s\n", "TYPE", "VALUE", "LINE", "COLUMN");
    fprintf(file, "------------------------------------------------\n");
    
    // Write each token
    for (size_t i = 0; i < lexer->num_tokens; i++) {
        Token *token = &lexer->tokens[i];
        
        const char *type_str = "UNKNOWN";
        switch (token->type) {
            case TOKEN_IDENTIFIER: type_str = "IDENTIFIER"; break;
            case TOKEN_NUMBER: type_str = "NUMBER"; break;
            case TOKEN_STRING: type_str = "STRING"; break;
            case TOKEN_KEYWORD: type_str = "KEYWORD"; break;
            case TOKEN_OPERATOR: type_str = "OPERATOR"; break;
            case TOKEN_PUNCTUATION: type_str = "PUNCTUATION"; break;
            case TOKEN_COMMENT: type_str = "COMMENT"; break;
            case TOKEN_WHITESPACE: type_str = "WHITESPACE"; break;
            case TOKEN_EOF: type_str = "EOF"; break;
            default: type_str = "UNKNOWN"; break;
        }
        
        fprintf(file, "%-15s %-15s %-10d %-10d\n", 
                type_str, token->value, token->line, token->column);
    }
    
    fclose(file);
    return true;
}

// Save tokens to a JSON file
bool lexer_save_tokens_json(Lexer *lexer, const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) return false;
    
    fprintf(file, "{\n");
    fprintf(file, "  \"tokens\": [\n");
    
    // Write each token as a JSON object
    for (size_t i = 0; i < lexer->num_tokens; i++) {
        Token *token = &lexer->tokens[i];
        
        const char *type_str = "UNKNOWN";
        switch (token->type) {
            case TOKEN_IDENTIFIER: type_str = "IDENTIFIER"; break;
            case TOKEN_NUMBER: type_str = "NUMBER"; break;
            case TOKEN_STRING: type_str = "STRING"; break;
            case TOKEN_KEYWORD: type_str = "KEYWORD"; break;
            case TOKEN_OPERATOR: type_str = "OPERATOR"; break;
            case TOKEN_PUNCTUATION: type_str = "PUNCTUATION"; break;
            case TOKEN_COMMENT: type_str = "COMMENT"; break;
            case TOKEN_WHITESPACE: type_str = "WHITESPACE"; break;
            case TOKEN_EOF: type_str = "EOF"; break;
            default: type_str = "UNKNOWN"; break;
        }
        
        fprintf(file, "    {\n");
        fprintf(file, "      \"type\": \"%s\",\n", type_str);
        fprintf(file, "      \"value\": \"%s\",\n", token->value);
        fprintf(file, "      \"line\": %d,\n", token->line);
        fprintf(file, "      \"column\": %d\n", token->column);
        
        if (i < lexer->num_tokens - 1) {
            fprintf(file, "    },\n");
        } else {
            fprintf(file, "    }\n");
        }
    }
    
    fprintf(file, "  ]\n");
    fprintf(file, "}\n");
    
    fclose(file);
    return true;
}
