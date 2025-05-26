#ifndef AST_H
#define AST_H

#include "common.h"

// AST node functions
ASTNode* ast_create_node(NodeType type, const char *value);
void ast_add_child(ASTNode *parent, ASTNode *child);
void ast_free_node(ASTNode *node);
bool ast_save_to_file(ASTNode *root, const char *filename);
bool ast_save_to_dot(ASTNode *root, const char *filename);
bool ast_save_to_json(ASTNode *root, const char *filename);

// Helper functions for node creation
ASTNode* ast_create_program();
ASTNode* ast_create_function(const char *name, ASTNode *params, ASTNode *body);
ASTNode* ast_create_block();
ASTNode* ast_create_var_decl(const char *type, const char *name, ASTNode *init_expr);
ASTNode* ast_create_assignment(const char *name, ASTNode *expr);
ASTNode* ast_create_binary_op(const char *op, ASTNode *left, ASTNode *right);
ASTNode* ast_create_unary_op(const char *op, ASTNode *expr);
ASTNode* ast_create_if(ASTNode *condition, ASTNode *then_branch, ASTNode *else_branch);
ASTNode* ast_create_while(ASTNode *condition, ASTNode *body);
ASTNode* ast_create_for(ASTNode *init, ASTNode *condition, ASTNode *update, ASTNode *body);
ASTNode* ast_create_return(ASTNode *expr);
ASTNode* ast_create_call(const char *name, ASTNode *args);
ASTNode* ast_create_identifier(const char *name);
ASTNode* ast_create_number(const char *value);
ASTNode* ast_create_string(const char *value);

#endif // AST_H
