#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Create a new AST node
ASTNode* ast_create_node(NodeType type, const char *value) {
    ASTNode *node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = type;
    node->value = value ? strdup(value) : NULL;
    node->children = NULL;
    node->num_children = 0;
    node->capacity = 0;
    
    return node;
}

// Add a child node to a parent node
void ast_add_child(ASTNode *parent, ASTNode *child) {
    if (!parent || !child) return;
    
    // Initialize or expand children array if needed
    if (!parent->children) {
        parent->capacity = 4;  // Initial capacity
        parent->children = (ASTNode*)malloc(sizeof(ASTNode) * parent->capacity);
        if (!parent->children) return;
    } else if (parent->num_children >= parent->capacity) {
        parent->capacity *= 2;
        ASTNode *new_children = (ASTNode*)realloc(parent->children, 
                                                 sizeof(ASTNode) * parent->capacity);
        if (!new_children) return;
        parent->children = new_children;
    }
    
    // Add the child
    parent->children[parent->num_children++] = *child;
    
    // We've copied the child, so free the original pointer
    // but not its contents (which are now owned by the parent)
    free(child);
}

// Free an AST node and all its children
void ast_free_node(ASTNode *node) {
    if (!node) return;
    
    // Free value
    if (node->value) {
        free(node->value);
    }
    
    // Free children
    if (node->children) {
        for (int i = 0; i < node->num_children; i++) {
            ast_free_node(&node->children[i]);
        }
        free(node->children);
    }
    
    // Free the node itself
    free(node);
}

// Get string representation of node type
static const char* get_node_type_str(NodeType type) {
    switch (type) {
        case NODE_PROGRAM: return "PROGRAM";
        case NODE_FUNCTION_DECL: return "FUNCTION_DECL";
        case NODE_BLOCK: return "BLOCK";
        case NODE_VARIABLE_DECL: return "VARIABLE_DECL";
        case NODE_ASSIGNMENT: return "ASSIGNMENT";
        case NODE_BINARY_OP: return "BINARY_OP";
        case NODE_UNARY_OP: return "UNARY_OP";
        case NODE_IF: return "IF";
        case NODE_WHILE: return "WHILE";
        case NODE_FOR: return "FOR";
        case NODE_RETURN: return "RETURN";
        case NODE_CALL: return "CALL";
        case NODE_IDENTIFIER: return "IDENTIFIER";
        case NODE_NUMBER: return "NUMBER";
        case NODE_STRING: return "STRING";
        default: return "UNKNOWN";
    }
}

// Forward declaration of helper function
static void print_ast(FILE *file, ASTNode *node, int depth);

// Save AST to a text file
bool ast_save_to_file(ASTNode *root, const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) return false;
    
    print_ast(file, root, 0);
    
    fclose(file);
    return true;
}

// Helper function to print the AST recursively
static void print_ast(FILE *file, ASTNode *node, int depth) {
    if (!node) return;
    
    // Print indentation
    for (int i = 0; i < depth; i++) {
        fprintf(file, "  ");
    }
    
    // Print node info
    fprintf(file, "%s", get_node_type_str(node->type));
    if (node->value) {
        fprintf(file, " (%s)", node->value);
    }
    fprintf(file, "\n");
    
    // Print children
    for (int i = 0; i < node->num_children; i++) {
        print_ast(file, &node->children[i], depth + 1);
    }
}

// Forward declaration of helper function
static void generate_dot_nodes(FILE *file, ASTNode *node, int parent_id, int my_id, int *node_counter);

// Save AST to DOT format for Graphviz
bool ast_save_to_dot(ASTNode *root, const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) return false;
    
    // Write DOT header
    fprintf(file, "digraph AST {\n");
    fprintf(file, "  node [shape=box, fontname=\"Arial\"];\n");
    
    // Node counter for unique IDs
    int node_counter = 0;
    
    // Start generating from root
    generate_dot_nodes(file, root, -1, node_counter, &node_counter);
    
    // Write DOT footer
    fprintf(file, "}\n");
    
    fclose(file);
    return true;
}

// Helper function to generate DOT nodes recursively
static void generate_dot_nodes(FILE *file, ASTNode *node, int parent_id, int my_id, int *node_counter) {
    if (!node) return;
    
    // Create node label
    const char *type_str = get_node_type_str(node->type);
    fprintf(file, "  node%d [label=\"%s", my_id, type_str);
    if (node->value) {
        fprintf(file, "\\n%s", node->value);
    }
    fprintf(file, "\"];\n");
    
    // Create edge from parent to this node
    if (parent_id >= 0) {
        fprintf(file, "  node%d -> node%d;\n", parent_id, my_id);
    }
    
    // Process children
    for (int i = 0; i < node->num_children; i++) {
        generate_dot_nodes(file, &node->children[i], my_id, ++(*node_counter), node_counter);
    }
}

// Forward declaration of helper function
static void write_node_json(FILE *file, ASTNode *node, int depth, bool is_last);

// Save AST to JSON format
bool ast_save_to_json(ASTNode *root, const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) return false;
    
    // Write the JSON file
    fprintf(file, "{\n");
    fprintf(file, "  \"ast\": ");
    write_node_json(file, root, 1, true);
    fprintf(file, "}\n");
    
    fclose(file);
    return true;
}

// Helper function to write a node as JSON
static void write_node_json(FILE *file, ASTNode *node, int depth, bool is_last) {
    if (!node) return;
    
    // Indentation
    for (int i = 0; i < depth; i++) {
        fprintf(file, "  ");
    }
    
    // Start node object
    fprintf(file, "{\n");
    
    // Node type
    for (int i = 0; i < depth + 1; i++) {
        fprintf(file, "  ");
    }
    fprintf(file, "\"type\": \"%s\",\n", get_node_type_str(node->type));
    
    // Node value (if any)
    for (int i = 0; i < depth + 1; i++) {
        fprintf(file, "  ");
    }
    if (node->value) {
        fprintf(file, "\"value\": \"%s\",\n", node->value);
    } else {
        fprintf(file, "\"value\": null,\n");
    }
    
    // Children array
    for (int i = 0; i < depth + 1; i++) {
        fprintf(file, "  ");
    }
    fprintf(file, "\"children\": [\n");
    
    // Write each child
    for (int i = 0; i < node->num_children; i++) {
        write_node_json(file, &node->children[i], depth + 2, i == node->num_children - 1);
    }
    
    // Close children array
    for (int i = 0; i < depth + 1; i++) {
        fprintf(file, "  ");
    }
    fprintf(file, "]\n");
    
    // Close node object
    for (int i = 0; i < depth; i++) {
        fprintf(file, "  ");
    }
    if (is_last) {
        fprintf(file, "}\n");
    } else {
        fprintf(file, "},\n");
    }
}

// Helper functions for creating specific node types

ASTNode* ast_create_program() {
    return ast_create_node(NODE_PROGRAM, NULL);
}

ASTNode* ast_create_function(const char *name, ASTNode *params, ASTNode *body) {
    ASTNode *node = ast_create_node(NODE_FUNCTION_DECL, name);
    if (params) ast_add_child(node, params);
    if (body) ast_add_child(node, body);
    return node;
}

ASTNode* ast_create_block() {
    return ast_create_node(NODE_BLOCK, NULL);
}

ASTNode* ast_create_var_decl(const char *type, const char *name, ASTNode *init_expr) {
    // Combine type and name for the value
    char *value = (char*)malloc(strlen(type) + strlen(name) + 2);
    if (!value) return NULL;
    sprintf(value, "%s %s", type, name);
    
    ASTNode *node = ast_create_node(NODE_VARIABLE_DECL, value);
    free(value);
    
    if (init_expr) ast_add_child(node, init_expr);
    return node;
}

ASTNode* ast_create_assignment(const char *name, ASTNode *expr) {
    ASTNode *node = ast_create_node(NODE_ASSIGNMENT, name);
    if (expr) ast_add_child(node, expr);
    return node;
}

ASTNode* ast_create_binary_op(const char *op, ASTNode *left, ASTNode *right) {
    ASTNode *node = ast_create_node(NODE_BINARY_OP, op);
    if (left) ast_add_child(node, left);
    if (right) ast_add_child(node, right);
    return node;
}

ASTNode* ast_create_unary_op(const char *op, ASTNode *expr) {
    ASTNode *node = ast_create_node(NODE_UNARY_OP, op);
    if (expr) ast_add_child(node, expr);
    return node;
}

ASTNode* ast_create_if(ASTNode *condition, ASTNode *then_branch, ASTNode *else_branch) {
    ASTNode *node = ast_create_node(NODE_IF, NULL);
    if (condition) ast_add_child(node, condition);
    if (then_branch) ast_add_child(node, then_branch);
    if (else_branch) ast_add_child(node, else_branch);
    return node;
}

ASTNode* ast_create_while(ASTNode *condition, ASTNode *body) {
    ASTNode *node = ast_create_node(NODE_WHILE, NULL);
    if (condition) ast_add_child(node, condition);
    if (body) ast_add_child(node, body);
    return node;
}

ASTNode* ast_create_for(ASTNode *init, ASTNode *condition, ASTNode *update, ASTNode *body) {
    ASTNode *node = ast_create_node(NODE_FOR, NULL);
    if (init) ast_add_child(node, init);
    if (condition) ast_add_child(node, condition);
    if (update) ast_add_child(node, update);
    if (body) ast_add_child(node, body);
    return node;
}

ASTNode* ast_create_return(ASTNode *expr) {
    ASTNode *node = ast_create_node(NODE_RETURN, NULL);
    if (expr) ast_add_child(node, expr);
    return node;
}

ASTNode* ast_create_call(const char *name, ASTNode *args) {
    ASTNode *node = ast_create_node(NODE_CALL, name);
    if (args) ast_add_child(node, args);
    return node;
}

ASTNode* ast_create_identifier(const char *name) {
    return ast_create_node(NODE_IDENTIFIER, name);
}

ASTNode* ast_create_number(const char *value) {
    return ast_create_node(NODE_NUMBER, value);
}

ASTNode* ast_create_string(const char *value) {
    return ast_create_node(NODE_STRING, value);
}
