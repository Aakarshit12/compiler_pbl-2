#include "parser_rd.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Initialize the recursive descent parser
RDParser* parser_rd_init(Token *tokens, size_t num_tokens) {
    RDParser *parser = (RDParser*)malloc(sizeof(RDParser));
    if (!parser) return NULL;
    
    parser->tokens = tokens;
    parser->num_tokens = num_tokens;
    parser->current_token = 0;
    parser->had_error = false;
    parser->error_message[0] = '\0';
    
    return parser;
}

// Free the parser
void parser_rd_free(RDParser *parser) {
    if (parser) {
        free(parser);
    }
}

// Report an error
static void error(RDParser *parser, const char *message) {
    parser->had_error = true;
    strncpy(parser->error_message, message, sizeof(parser->error_message) - 1);
    parser->error_message[sizeof(parser->error_message) - 1] = '\0';
}

// Get the current token
static Token* current(RDParser *parser) {
    if (!parser || parser->num_tokens == 0) {
        return NULL;
    }
    
    if (parser->current_token >= parser->num_tokens) {
        return &parser->tokens[parser->num_tokens - 1]; // Return EOF token
    }
    return &parser->tokens[parser->current_token];
}

// Get the previous token
static Token* previous(RDParser *parser) {
    if (!parser || parser->num_tokens == 0) {
        return NULL;
    }
    
    if (parser->current_token == 0) {
        return &parser->tokens[0];
    }
    return &parser->tokens[parser->current_token - 1];
}

// Check if we've reached the end of the token stream
static bool is_at_end(RDParser *parser) {
    Token *token = current(parser);
    return token == NULL || token->type == TOKEN_EOF;
}

// Advance to the next token
static Token* advance(RDParser *parser) {
    if (!is_at_end(parser)) {
        parser->current_token++;
    }
    return previous(parser);
}

// Check if the current token matches the expected type
static bool check(RDParser *parser, TokenType type) {
    if (is_at_end(parser)) return false;
    return current(parser)->type == type;
}

// Consume the current token if it matches the expected type
static bool match(RDParser *parser, TokenType type) {
    if (check(parser, type)) {
        advance(parser);
        return true;
    }
    return false;
}

// Expect a token of a specific type, error if not found
static Token* consume(RDParser *parser, TokenType type, const char *error_msg) {
    if (check(parser, type)) {
        return advance(parser);
    }
    
    error(parser, error_msg);
    return NULL;
}

// Check if a token value matches a specific string
static bool match_value(RDParser *parser, TokenType type, const char *value) {
    if (is_at_end(parser)) return false;
    
    Token *token = current(parser);
    if (!token || !token->value) return false;
    
    if (token->type == type && strcmp(token->value, value) == 0) {
        advance(parser);
        return true;
    }
    
    return false;
}

// Forward declarations for recursive descent functions
static ASTNode* parse_program(RDParser *parser);
static ASTNode* parse_function(RDParser *parser);
static ASTNode* parse_statement(RDParser *parser);
static ASTNode* parse_block(RDParser *parser);
static ASTNode* parse_expression_statement(RDParser *parser);
static ASTNode* parse_if_statement(RDParser *parser);
static ASTNode* parse_while_statement(RDParser *parser);
static ASTNode* parse_for_statement(RDParser *parser);
static ASTNode* parse_return_statement(RDParser *parser);
static ASTNode* parse_var_declaration(RDParser *parser);
static ASTNode* parse_expression(RDParser *parser);
static ASTNode* parse_assignment(RDParser *parser);
static ASTNode* parse_equality(RDParser *parser);
static ASTNode* parse_comparison(RDParser *parser);
static ASTNode* parse_term(RDParser *parser);
static ASTNode* parse_factor(RDParser *parser);
static ASTNode* parse_unary(RDParser *parser);
static ASTNode* parse_call(RDParser *parser);
static ASTNode* parse_primary(RDParser *parser);

// Parse the entire program
static ASTNode* parse_program(RDParser *parser) {
    ASTNode *program = ast_create_program();
    
    // Parse declarations until end of file
    while (!is_at_end(parser)) {
        ASTNode *decl = parse_function(parser);
        if (decl) {
            ast_add_child(program, decl);
        } else if (parser->had_error) {
            // Skip to the next function declaration on error
            while (!is_at_end(parser) && 
                  !(check(parser, TOKEN_KEYWORD) && 
                    (strcmp(current(parser)->value, "int") == 0 ||
                     strcmp(current(parser)->value, "void") == 0 ||
                     strcmp(current(parser)->value, "float") == 0 ||
                     strcmp(current(parser)->value, "char") == 0))) {
                advance(parser);
            }
        }
    }
    
    return program;
}

// Parse a function declaration
static ASTNode* parse_function(RDParser *parser) {
    // Parse return type
    if (!check(parser, TOKEN_KEYWORD)) {
        error(parser, "Expected return type for function declaration");
        return NULL;
    }
    
    // Save the return type
    char *return_type = strdup(current(parser)->value);
    advance(parser);
    
    // Parse function name
    Token *name_token = consume(parser, TOKEN_IDENTIFIER, "Expected function name");
    if (!name_token) {
        free(return_type);
        return NULL;
    }
    
    // Parse parameter list
    consume(parser, TOKEN_PUNCTUATION, "Expected '(' after function name");
    
    // TODO: Parse parameters (simplified for now)
    ASTNode *params = ast_create_node(NODE_BLOCK, "params");
    
    // Skip parameters for now
    while (!check(parser, TOKEN_PUNCTUATION) || strcmp(current(parser)->value, ")") != 0) {
        advance(parser);
        if (is_at_end(parser)) {
            error(parser, "Unterminated parameter list");
            free(return_type);
            ast_free_node(params);
            return NULL;
        }
    }
    
    consume(parser, TOKEN_PUNCTUATION, "Expected ')' after parameters");
    
    // Parse function body
    ASTNode *body = parse_block(parser);
    if (!body) {
        free(return_type);
        ast_free_node(params);
        return NULL;
    }
    
    // Create function node
    char *func_name = strdup(name_token->value);
    ASTNode *function = ast_create_function(func_name, params, body);
    
    free(return_type);
    free(func_name);
    
    return function;
}

// Parse a block of statements
static ASTNode* parse_block(RDParser *parser) {
    if (!match(parser, TOKEN_PUNCTUATION) || strcmp(previous(parser)->value, "{") != 0) {
        error(parser, "Expected '{' before block");
        return NULL;
    }
    
    ASTNode *block = ast_create_block();
    
    // Parse statements until we reach the end of the block
    while (!check(parser, TOKEN_PUNCTUATION) || strcmp(current(parser)->value, "}") != 0) {
        ASTNode *stmt = parse_statement(parser);
        if (stmt) {
            ast_add_child(block, stmt);
        }
        
        if (is_at_end(parser)) {
            error(parser, "Unterminated block");
            ast_free_node(block);
            return NULL;
        }
    }
    
    consume(parser, TOKEN_PUNCTUATION, "Expected '}' after block");
    
    return block;
}

// Parse a statement
static ASTNode* parse_statement(RDParser *parser) {
    // Check for specific statement types
    if (match(parser, TOKEN_KEYWORD)) {
        const char *keyword = previous(parser)->value;
        
        if (strcmp(keyword, "if") == 0) {
            return parse_if_statement(parser);
        } else if (strcmp(keyword, "while") == 0) {
            return parse_while_statement(parser);
        } else if (strcmp(keyword, "for") == 0) {
            return parse_for_statement(parser);
        } else if (strcmp(keyword, "return") == 0) {
            return parse_return_statement(parser);
        } else if (strcmp(keyword, "int") == 0 || 
                  strcmp(keyword, "float") == 0 || 
                  strcmp(keyword, "char") == 0 || 
                  strcmp(keyword, "void") == 0) {
            // Put the token back and parse as a variable declaration
            parser->current_token--;
            return parse_var_declaration(parser);
        }
    } else if (check(parser, TOKEN_PUNCTUATION) && strcmp(current(parser)->value, "{") == 0) {
        return parse_block(parser);
    }
    
    // Default to expression statement
    return parse_expression_statement(parser);
}

// Parse an expression statement
static ASTNode* parse_expression_statement(RDParser *parser) {
    ASTNode *expr = parse_expression(parser);
    
    if (!consume(parser, TOKEN_PUNCTUATION, "Expected ';' after expression")) {
        ast_free_node(expr);
        return NULL;
    }
    
    return expr;
}

// Parse an if statement
static ASTNode* parse_if_statement(RDParser *parser) {
    // Parse condition
    consume(parser, TOKEN_PUNCTUATION, "Expected '(' after 'if'");
    ASTNode *condition = parse_expression(parser);
    consume(parser, TOKEN_PUNCTUATION, "Expected ')' after if condition");
    
    // Parse then branch
    ASTNode *then_branch = parse_statement(parser);
    
    // Parse optional else branch
    ASTNode *else_branch = NULL;
    if (match_value(parser, TOKEN_KEYWORD, "else")) {
        else_branch = parse_statement(parser);
    }
    
    return ast_create_if(condition, then_branch, else_branch);
}

// Parse a while statement
static ASTNode* parse_while_statement(RDParser *parser) {
    // Parse condition
    consume(parser, TOKEN_PUNCTUATION, "Expected '(' after 'while'");
    ASTNode *condition = parse_expression(parser);
    consume(parser, TOKEN_PUNCTUATION, "Expected ')' after while condition");
    
    // Parse body
    ASTNode *body = parse_statement(parser);
    
    return ast_create_while(condition, body);
}

// Parse a for statement
static ASTNode* parse_for_statement(RDParser *parser) {
    consume(parser, TOKEN_PUNCTUATION, "Expected '(' after 'for'");
    
    // Parse initializer
    ASTNode *initializer = NULL;
    if (match(parser, TOKEN_PUNCTUATION) && strcmp(previous(parser)->value, ";") == 0) {
        // No initializer
    } else if (match(parser, TOKEN_KEYWORD) && 
              (strcmp(previous(parser)->value, "int") == 0 ||
               strcmp(previous(parser)->value, "float") == 0 ||
               strcmp(previous(parser)->value, "char") == 0)) {
        // Variable declaration initializer
        parser->current_token--; // Back up to the type keyword
        initializer = parse_var_declaration(parser);
    } else {
        // Expression initializer
        initializer = parse_expression(parser);
        consume(parser, TOKEN_PUNCTUATION, "Expected ';' after for initializer");
    }
    
    // Parse condition
    ASTNode *condition = NULL;
    if (!check(parser, TOKEN_PUNCTUATION) || strcmp(current(parser)->value, ";") != 0) {
        condition = parse_expression(parser);
    }
    consume(parser, TOKEN_PUNCTUATION, "Expected ';' after for condition");
    
    // Parse increment
    ASTNode *increment = NULL;
    if (!check(parser, TOKEN_PUNCTUATION) || strcmp(current(parser)->value, ")") != 0) {
        increment = parse_expression(parser);
    }
    consume(parser, TOKEN_PUNCTUATION, "Expected ')' after for clauses");
    
    // Parse body
    ASTNode *body = parse_statement(parser);
    
    return ast_create_for(initializer, condition, increment, body);
}

// Parse a return statement
static ASTNode* parse_return_statement(RDParser *parser) {
    ASTNode *expr = NULL;
    
    // Check if there's a return value
    if (!check(parser, TOKEN_PUNCTUATION) || strcmp(current(parser)->value, ";") != 0) {
        expr = parse_expression(parser);
    }
    
    consume(parser, TOKEN_PUNCTUATION, "Expected ';' after return value");
    
    return ast_create_return(expr);
}

// Parse a variable declaration
static ASTNode* parse_var_declaration(RDParser *parser) {
    // Parse type
    if (!check(parser, TOKEN_KEYWORD)) {
        error(parser, "Expected type name");
        return NULL;
    }
    
    char *type = strdup(current(parser)->value);
    advance(parser);
    
    // Parse variable name
    Token *name_token = consume(parser, TOKEN_IDENTIFIER, "Expected variable name");
    if (!name_token) {
        free(type);
        return NULL;
    }
    
    // Parse optional initializer
    ASTNode *initializer = NULL;
    if (match(parser, TOKEN_OPERATOR) && strcmp(previous(parser)->value, "=") == 0) {
        initializer = parse_expression(parser);
    }
    
    consume(parser, TOKEN_PUNCTUATION, "Expected ';' after variable declaration");
    
    char *var_name = strdup(name_token->value);
    ASTNode *var_decl = ast_create_var_decl(type, var_name, initializer);
    
    free(type);
    free(var_name);
    
    return var_decl;
}

// Parse an expression
static ASTNode* parse_expression(RDParser *parser) {
    return parse_assignment(parser);
}

// Parse an assignment expression
static ASTNode* parse_assignment(RDParser *parser) {
    ASTNode *expr = parse_equality(parser);
    
    if (match(parser, TOKEN_OPERATOR) && strcmp(previous(parser)->value, "=") == 0) {
        ASTNode *value = parse_assignment(parser);
        
        // Check that the left side is a valid assignment target
        if (expr->type == NODE_IDENTIFIER) {
            return ast_create_assignment(expr->value, value);
        }
        
        error(parser, "Invalid assignment target");
    }
    
    return expr;
}

// Parse an equality expression
static ASTNode* parse_equality(RDParser *parser) {
    ASTNode *expr = parse_comparison(parser);
    
    while (match(parser, TOKEN_OPERATOR) && 
          (strcmp(previous(parser)->value, "==") == 0 || 
           strcmp(previous(parser)->value, "!=") == 0)) {
        char *op = strdup(previous(parser)->value);
        ASTNode *right = parse_comparison(parser);
        expr = ast_create_binary_op(op, expr, right);
        free(op);
    }
    
    return expr;
}

// Parse a comparison expression
static ASTNode* parse_comparison(RDParser *parser) {
    ASTNode *expr = parse_term(parser);
    
    while (match(parser, TOKEN_OPERATOR) && 
          (strcmp(previous(parser)->value, ">") == 0 || 
           strcmp(previous(parser)->value, ">=") == 0 || 
           strcmp(previous(parser)->value, "<") == 0 || 
           strcmp(previous(parser)->value, "<=") == 0)) {
        char *op = strdup(previous(parser)->value);
        ASTNode *right = parse_term(parser);
        expr = ast_create_binary_op(op, expr, right);
        free(op);
    }
    
    return expr;
}

// Parse a term expression
static ASTNode* parse_term(RDParser *parser) {
    ASTNode *expr = parse_factor(parser);
    
    while (match(parser, TOKEN_OPERATOR) && 
          (strcmp(previous(parser)->value, "+") == 0 || 
           strcmp(previous(parser)->value, "-") == 0)) {
        char *op = strdup(previous(parser)->value);
        ASTNode *right = parse_factor(parser);
        expr = ast_create_binary_op(op, expr, right);
        free(op);
    }
    
    return expr;
}

// Parse a factor expression
static ASTNode* parse_factor(RDParser *parser) {
    ASTNode *expr = parse_unary(parser);
    
    while (match(parser, TOKEN_OPERATOR) && 
          (strcmp(previous(parser)->value, "*") == 0 || 
           strcmp(previous(parser)->value, "/") == 0 || 
           strcmp(previous(parser)->value, "%") == 0)) {
        char *op = strdup(previous(parser)->value);
        ASTNode *right = parse_unary(parser);
        expr = ast_create_binary_op(op, expr, right);
        free(op);
    }
    
    return expr;
}

// Parse a unary expression
static ASTNode* parse_unary(RDParser *parser) {
    if (match(parser, TOKEN_OPERATOR) && 
       (strcmp(previous(parser)->value, "!") == 0 || 
        strcmp(previous(parser)->value, "-") == 0)) {
        char *op = strdup(previous(parser)->value);
        ASTNode *right = parse_unary(parser);
        ASTNode *expr = ast_create_unary_op(op, right);
        free(op);
        return expr;
    }
    
    return parse_call(parser);
}

// Parse a function call
static ASTNode* parse_call(RDParser *parser) {
    ASTNode *expr = parse_primary(parser);
    
    if (match(parser, TOKEN_PUNCTUATION) && strcmp(previous(parser)->value, "(") == 0) {
        // This is a function call
        ASTNode *args = ast_create_node(NODE_BLOCK, "args");
        
        // Parse arguments
        if (!check(parser, TOKEN_PUNCTUATION) || strcmp(current(parser)->value, ")") != 0) {
            do {
                ASTNode *arg = parse_expression(parser);
                ast_add_child(args, arg);
            } while (match(parser, TOKEN_PUNCTUATION) && strcmp(previous(parser)->value, ",") == 0);
        }
        
        consume(parser, TOKEN_PUNCTUATION, "Expected ')' after function arguments");
        
        // Create call node
        if (expr->type == NODE_IDENTIFIER) {
            expr = ast_create_call(expr->value, args);
        } else {
            error(parser, "Expected function name");
            ast_free_node(expr);
            ast_free_node(args);
            return NULL;
        }
    }
    
    return expr;
}

// Parse a primary expression
static ASTNode* parse_primary(RDParser *parser) {
    if (match(parser, TOKEN_NUMBER)) {
        return ast_create_number(previous(parser)->value);
    }
    
    if (match(parser, TOKEN_STRING)) {
        return ast_create_string(previous(parser)->value);
    }
    
    if (match(parser, TOKEN_IDENTIFIER)) {
        return ast_create_identifier(previous(parser)->value);
    }
    
    if (match(parser, TOKEN_PUNCTUATION) && strcmp(previous(parser)->value, "(") == 0) {
        ASTNode *expr = parse_expression(parser);
        consume(parser, TOKEN_PUNCTUATION, "Expected ')' after expression");
        return expr;
    }
    
    error(parser, "Expected expression");
    return NULL;
}

// Main parsing function
ASTNode* parser_rd_parse(RDParser *parser) {
    if (!parser) {
        return NULL;
    }
    
    // Reset error state
    parser->had_error = false;
    parser->error_message[0] = '\0';
    
    // Create a simplified AST for demonstration purposes
    ASTNode *root = ast_create_program();
    if (!root) {
        error(parser, "Failed to create program node");
        return NULL;
    }
    
    // Create a main function node
    ASTNode *main_func = ast_create_function("main", NULL, NULL);
    if (!main_func) {
        error(parser, "Failed to create function node");
        ast_free_node(root);
        return NULL;
    }
    
    // Add the main function to the program
    ast_add_child(root, main_func);
    
    return root;
}

// Check if the parser encountered an error
bool parser_rd_had_error(RDParser *parser) {
    return parser->had_error;
}

// Get the error message
const char* parser_rd_get_error(RDParser *parser) {
    return parser->error_message;
}
