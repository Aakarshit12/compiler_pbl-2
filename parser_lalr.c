#include "parser_lalr.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Grammar symbols
typedef enum {
    SYM_ERROR = -1,
    SYM_EOF = 0,
    
    // Terminals
    SYM_IDENTIFIER,
    SYM_NUMBER,
    SYM_STRING,
    SYM_KEYWORD_INT,
    SYM_KEYWORD_FLOAT,
    SYM_KEYWORD_CHAR,
    SYM_KEYWORD_VOID,
    SYM_KEYWORD_IF,
    SYM_KEYWORD_ELSE,
    SYM_KEYWORD_WHILE,
    SYM_KEYWORD_FOR,
    SYM_KEYWORD_RETURN,
    SYM_OPERATOR_PLUS,
    SYM_OPERATOR_MINUS,
    SYM_OPERATOR_STAR,
    SYM_OPERATOR_SLASH,
    SYM_OPERATOR_PERCENT,
    SYM_OPERATOR_ASSIGN,
    SYM_OPERATOR_EQ,
    SYM_OPERATOR_NE,
    SYM_OPERATOR_LT,
    SYM_OPERATOR_LE,
    SYM_OPERATOR_GT,
    SYM_OPERATOR_GE,
    SYM_PUNCTUATION_LPAREN,
    SYM_PUNCTUATION_RPAREN,
    SYM_PUNCTUATION_LBRACE,
    SYM_PUNCTUATION_RBRACE,
    SYM_PUNCTUATION_SEMICOLON,
    SYM_PUNCTUATION_COMMA,
    
    // Non-terminals
    SYM_PROGRAM,
    SYM_FUNCTION_DECL,
    SYM_PARAM_LIST,
    SYM_PARAM,
    SYM_BLOCK,
    SYM_STATEMENT,
    SYM_EXPR_STMT,
    SYM_IF_STMT,
    SYM_WHILE_STMT,
    SYM_FOR_STMT,
    SYM_RETURN_STMT,
    SYM_VAR_DECL,
    SYM_EXPR,
    SYM_ASSIGNMENT,
    SYM_EQUALITY,
    SYM_COMPARISON,
    SYM_TERM,
    SYM_FACTOR,
    SYM_UNARY,
    SYM_CALL,
    SYM_PRIMARY,
    SYM_ARG_LIST
} Symbol;

// Action types
typedef enum {
    ACTION_SHIFT,
    ACTION_REDUCE,
    ACTION_ACCEPT,
    ACTION_ERROR
} ActionType;

// Action structure
typedef struct {
    ActionType type;
    int value;  // State for shift, rule for reduce
} Action;

// Production rule
typedef struct {
    Symbol lhs;
    Symbol *rhs;
    int rhs_length;
} Rule;

// Initialize the LALR parser
LALRParser* parser_lalr_init(Token *tokens, size_t num_tokens) {
    LALRParser *parser = (LALRParser*)malloc(sizeof(LALRParser));
    if (!parser) return NULL;
    
    parser->tokens = tokens;
    parser->num_tokens = num_tokens;
    parser->current_token = 0;
    parser->had_error = false;
    parser->error_message[0] = '\0';
    
    // Initialize state stack
    parser->state_stack_capacity = 128;
    parser->state_stack = (int*)malloc(sizeof(int) * parser->state_stack_capacity);
    parser->state_stack_size = 1;
    parser->state_stack[0] = 0;  // Start state
    
    // Initialize symbol stack
    parser->symbol_stack_capacity = 128;
    parser->symbol_stack = (void**)malloc(sizeof(void*) * parser->symbol_stack_capacity);
    parser->symbol_stack_size = 0;
    
    if (!parser->state_stack || !parser->symbol_stack) {
        parser_lalr_free(parser);
        return NULL;
    }
    
    return parser;
}

// Free the parser
void parser_lalr_free(LALRParser *parser) {
    if (!parser) return;
    
    if (parser->state_stack) {
        free(parser->state_stack);
    }
    
    if (parser->symbol_stack) {
        // Free any AST nodes on the stack
        for (int i = 0; i < parser->symbol_stack_size; i++) {
            if (parser->symbol_stack[i]) {
                // Check if it's an AST node (this is a simplification)
                // In a real implementation, you'd need a way to distinguish between
                // different types of values on the stack
                ast_free_node((ASTNode*)parser->symbol_stack[i]);
            }
        }
        free(parser->symbol_stack);
    }
    
    free(parser);
}

// Report an error
static void error(LALRParser *parser, const char *message) {
    parser->had_error = true;
    strncpy(parser->error_message, message, sizeof(parser->error_message) - 1);
    parser->error_message[sizeof(parser->error_message) - 1] = '\0';
}

// Push a state onto the state stack
static void push_state(LALRParser *parser, int state) {
    if (parser->state_stack_size >= parser->state_stack_capacity) {
        parser->state_stack_capacity *= 2;
        int *new_stack = (int*)realloc(parser->state_stack, 
                                       sizeof(int) * parser->state_stack_capacity);
        if (!new_stack) {
            error(parser, "Out of memory");
            return;
        }
        parser->state_stack = new_stack;
    }
    
    parser->state_stack[parser->state_stack_size++] = state;
}

// Pop states from the state stack
static void pop_states(LALRParser *parser, int count) {
    parser->state_stack_size -= count;
    if (parser->state_stack_size < 0) {
        parser->state_stack_size = 0;
    }
}

// Push a symbol onto the symbol stack
static void push_symbol(LALRParser *parser, void *value) {
    if (parser->symbol_stack_size >= parser->symbol_stack_capacity) {
        parser->symbol_stack_capacity *= 2;
        void **new_stack = (void**)realloc(parser->symbol_stack, 
                                          sizeof(void*) * parser->symbol_stack_capacity);
        if (!new_stack) {
            error(parser, "Out of memory");
            return;
        }
        parser->symbol_stack = new_stack;
    }
    
    parser->symbol_stack[parser->symbol_stack_size++] = value;
}

// Pop symbols from the symbol stack
static void* pop_symbol(LALRParser *parser) {
    if (parser->symbol_stack_size <= 0) {
        return NULL;
    }
    
    return parser->symbol_stack[--parser->symbol_stack_size];
}

// Get the current token
static Token* current_token(LALRParser *parser) {
    if (parser->current_token >= parser->num_tokens) {
        return &parser->tokens[parser->num_tokens - 1]; // Return EOF token
    }
    return &parser->tokens[parser->current_token];
}

// Convert token to symbol
static Symbol token_to_symbol(Token *token) {
    switch (token->type) {
        case TOKEN_IDENTIFIER:
            return SYM_IDENTIFIER;
        case TOKEN_NUMBER:
            return SYM_NUMBER;
        case TOKEN_STRING:
            return SYM_STRING;
        case TOKEN_KEYWORD:
            if (strcmp(token->value, "int") == 0) return SYM_KEYWORD_INT;
            if (strcmp(token->value, "float") == 0) return SYM_KEYWORD_FLOAT;
            if (strcmp(token->value, "char") == 0) return SYM_KEYWORD_CHAR;
            if (strcmp(token->value, "void") == 0) return SYM_KEYWORD_VOID;
            if (strcmp(token->value, "if") == 0) return SYM_KEYWORD_IF;
            if (strcmp(token->value, "else") == 0) return SYM_KEYWORD_ELSE;
            if (strcmp(token->value, "while") == 0) return SYM_KEYWORD_WHILE;
            if (strcmp(token->value, "for") == 0) return SYM_KEYWORD_FOR;
            if (strcmp(token->value, "return") == 0) return SYM_KEYWORD_RETURN;
            return SYM_ERROR;
        case TOKEN_OPERATOR:
            if (strcmp(token->value, "+") == 0) return SYM_OPERATOR_PLUS;
            if (strcmp(token->value, "-") == 0) return SYM_OPERATOR_MINUS;
            if (strcmp(token->value, "*") == 0) return SYM_OPERATOR_STAR;
            if (strcmp(token->value, "/") == 0) return SYM_OPERATOR_SLASH;
            if (strcmp(token->value, "%") == 0) return SYM_OPERATOR_PERCENT;
            if (strcmp(token->value, "=") == 0) return SYM_OPERATOR_ASSIGN;
            if (strcmp(token->value, "==") == 0) return SYM_OPERATOR_EQ;
            if (strcmp(token->value, "!=") == 0) return SYM_OPERATOR_NE;
            if (strcmp(token->value, "<") == 0) return SYM_OPERATOR_LT;
            if (strcmp(token->value, "<=") == 0) return SYM_OPERATOR_LE;
            if (strcmp(token->value, ">") == 0) return SYM_OPERATOR_GT;
            if (strcmp(token->value, ">=") == 0) return SYM_OPERATOR_GE;
            return SYM_ERROR;
        case TOKEN_PUNCTUATION:
            if (strcmp(token->value, "(") == 0) return SYM_PUNCTUATION_LPAREN;
            if (strcmp(token->value, ")") == 0) return SYM_PUNCTUATION_RPAREN;
            if (strcmp(token->value, "{") == 0) return SYM_PUNCTUATION_LBRACE;
            if (strcmp(token->value, "}") == 0) return SYM_PUNCTUATION_RBRACE;
            if (strcmp(token->value, ";") == 0) return SYM_PUNCTUATION_SEMICOLON;
            if (strcmp(token->value, ",") == 0) return SYM_PUNCTUATION_COMMA;
            return SYM_ERROR;
        case TOKEN_EOF:
            return SYM_EOF;
        default:
            return SYM_ERROR;
    }
}

// Define grammar rules
static Rule rules[] = {
    // Rule 0: Program -> Function+
    {SYM_PROGRAM, (Symbol[]){SYM_FUNCTION_DECL}, 1},
    
    // Rule 1: Function -> Type Identifier ( Params ) Block
    {SYM_FUNCTION_DECL, (Symbol[]){SYM_KEYWORD_INT, SYM_IDENTIFIER, SYM_PUNCTUATION_LPAREN, 
                                 SYM_PARAM_LIST, SYM_PUNCTUATION_RPAREN, SYM_BLOCK}, 6},
    
    // More rules would be defined here...
    
    // This is a simplified set of rules for demonstration
};

// LALR parsing table (action and goto tables combined)
// This is a simplified version for demonstration
static Action get_action(int state, Symbol symbol) {
    Action action = {ACTION_ERROR, 0};
    
    // This would be a large table in a real implementation
    // Here's a simplified example:
    if (state == 0) {
        if (symbol == SYM_KEYWORD_INT || symbol == SYM_KEYWORD_FLOAT || 
            symbol == SYM_KEYWORD_CHAR || symbol == SYM_KEYWORD_VOID) {
            action.type = ACTION_SHIFT;
            action.value = 1;  // Shift to state 1
        }
    }
    // Many more states and actions would be defined here...
    
    return action;
}

// Get the goto state
static int get_goto(int state, Symbol symbol) {
    // This would be part of the LALR parsing table
    // Simplified example:
    if (state == 0 && symbol == SYM_PROGRAM) return 100;  // Accept state
    
    // Many more goto entries would be defined here...
    
    return -1;  // Error
}

// Perform a reduction using the specified rule
static void do_reduction(LALRParser *parser, int rule_index) {
    Rule *rule = &rules[rule_index];
    
    // Pop the RHS symbols and states
    void *values[16];  // Assuming no rule has more than 16 symbols on RHS
    for (int i = rule->rhs_length - 1; i >= 0; i--) {
        values[i] = pop_symbol(parser);
    }
    pop_states(parser, rule->rhs_length);
    
    // Create a new AST node based on the rule
    ASTNode *node = NULL;
    
    switch (rule_index) {
        case 0:  // Program -> Function+
            node = ast_create_program();
            ast_add_child(node, (ASTNode*)values[0]);
            break;
        case 1:  // Function -> Type Identifier ( Params ) Block
            {
                char *name = strdup(((Token*)values[1])->value);
                node = ast_create_function(name, (ASTNode*)values[3], (ASTNode*)values[5]);
                free(name);
            }
            break;
        // More reduction actions would be defined here...
    }
    
    // Push the new node onto the symbol stack
    push_symbol(parser, node);
    
    // Look up the goto state and push it
    int current_state = parser->state_stack[parser->state_stack_size - 1];
    int goto_state = get_goto(current_state, rule->lhs);
    
    if (goto_state < 0) {
        error(parser, "Invalid state transition");
        return;
    }
    
    push_state(parser, goto_state);
}

// Main parsing function
ASTNode* parser_lalr_parse(LALRParser *parser) {
    // This is a simplified LALR parser implementation
    // A real implementation would have a complete parsing table
    
    while (true) {
        Token *token = current_token(parser);
        Symbol symbol = token_to_symbol(token);
        int current_state = parser->state_stack[parser->state_stack_size - 1];
        
        Action action = get_action(current_state, symbol);
        
        switch (action.type) {
            case ACTION_SHIFT:
                push_symbol(parser, token);
                push_state(parser, action.value);
                parser->current_token++;
                break;
                
            case ACTION_REDUCE:
                do_reduction(parser, action.value);
                break;
                
            case ACTION_ACCEPT:
                // The parse was successful, return the root node
                if (parser->symbol_stack_size > 0) {
                    return (ASTNode*)parser->symbol_stack[0];
                }
                return NULL;
                
            case ACTION_ERROR:
            default:
                error(parser, "Syntax error");
                return NULL;
        }
        
        if (parser->had_error) {
            return NULL;
        }
    }
    
    return NULL;
}

// Check if the parser encountered an error
bool parser_lalr_had_error(LALRParser *parser) {
    return parser->had_error;
}

// Get the error message
const char* parser_lalr_get_error(LALRParser *parser) {
    return parser->error_message;
}
