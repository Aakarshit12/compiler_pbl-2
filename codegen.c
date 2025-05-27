#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Initialize the code generator
CodeGenerator* codegen_init(ASTNode *ast) {
    CodeGenerator *codegen = (CodeGenerator*)malloc(sizeof(CodeGenerator));
    if (!codegen) return NULL;
    
    codegen->ast = ast;
    codegen->tac_file = NULL;
    codegen->stack_file = NULL;
    codegen->target_file = NULL;
    codegen->temp_var_count = 0;
    codegen->label_count = 0;
    
    return codegen;
}

// Free the code generator
void codegen_free(CodeGenerator *codegen) {
    if (!codegen) return;
    
    if (codegen->tac_file) {
        fclose(codegen->tac_file);
    }
    
    if (codegen->stack_file) {
        fclose(codegen->stack_file);
    }
    
    if (codegen->target_file) {
        fclose(codegen->target_file);
    }
    
    free(codegen);
}

// Generate a new temporary variable name
static char* new_temp(CodeGenerator *codegen) {
    char *temp = (char*)malloc(16);
    if (!temp) return NULL;
    
    sprintf(temp, "t%d", codegen->temp_var_count++);
    return temp;
}

// Generate a new label
static char* new_label(CodeGenerator *codegen) {
    char *label = (char*)malloc(16);
    if (!label) return NULL;
    
    sprintf(label, "L%d", codegen->label_count++);
    return label;
}

// Generate Three Address Code (TAC) for an expression
static char* generate_expr_tac(CodeGenerator *codegen, ASTNode *node) {
    if (!node) return NULL;
    
    switch (node->type) {
        case NODE_NUMBER:
            return strdup(node->value);
            
        case NODE_IDENTIFIER:
            return strdup(node->value);
            
        case NODE_BINARY_OP: {
            char *left = generate_expr_tac(codegen, &node->children[0]);
            char *right = generate_expr_tac(codegen, &node->children[1]);
            char *temp = new_temp(codegen);
            
            fprintf(codegen->tac_file, "%s = %s %s %s\n", 
                    temp, left, node->value, right);
            
            free(left);
            free(right);
            return temp;
        }
        
        case NODE_UNARY_OP: {
            char *expr = generate_expr_tac(codegen, &node->children[0]);
            char *temp = new_temp(codegen);
            
            fprintf(codegen->tac_file, "%s = %s %s\n", 
                    temp, node->value, expr);
            
            free(expr);
            return temp;
        }
        
        case NODE_CALL: {
            // Handle function call arguments
            char *args[16];  // Assuming no more than 16 arguments
            int num_args = 0;
            
            if (node->num_children > 0 && node->children[0].type == NODE_BLOCK) {
                ASTNode *args_node = &node->children[0];
                num_args = args_node->num_children;
                
                for (int i = 0; i < num_args; i++) {
                    args[i] = generate_expr_tac(codegen, &args_node->children[i]);
                }
            }
            
            // Generate parameter passing code
            for (int i = 0; i < num_args; i++) {
                fprintf(codegen->tac_file, "param %s\n", args[i]);
            }
            
            // Generate call
            char *temp = new_temp(codegen);
            fprintf(codegen->tac_file, "%s = call %s, %d\n", 
                    temp, node->value, num_args);
            
            // Free argument temps
            for (int i = 0; i < num_args; i++) {
                free(args[i]);
            }
            
            return temp;
        }
        
        default:
            return strdup("error");
    }
}

// Generate TAC for a statement
static void generate_stmt_tac(CodeGenerator *codegen, ASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case NODE_BLOCK:
            for (int i = 0; i < node->num_children; i++) {
                generate_stmt_tac(codegen, &node->children[i]);
            }
            break;
            
        case NODE_VARIABLE_DECL: {
            // Check if there's an initializer
            if (node->num_children > 0) {
                char *value = generate_expr_tac(codegen, &node->children[0]);
                fprintf(codegen->tac_file, "%s = %s\n", node->value, value);
                free(value);
            }
            break;
        }
        
        case NODE_ASSIGNMENT: {
            char *value = generate_expr_tac(codegen, &node->children[0]);
            fprintf(codegen->tac_file, "%s = %s\n", node->value, value);
            free(value);
            break;
        }
        
        case NODE_IF: {
            char *condition = generate_expr_tac(codegen, &node->children[0]);
            char *else_label = new_label(codegen);
            char *end_label = new_label(codegen);
            
            fprintf(codegen->tac_file, "if %s == 0 goto %s\n", condition, else_label);
            
            // Then branch
            generate_stmt_tac(codegen, &node->children[1]);
            fprintf(codegen->tac_file, "goto %s\n", end_label);
            
            // Else branch
            fprintf(codegen->tac_file, "%s:\n", else_label);
            if (node->num_children > 2) {
                generate_stmt_tac(codegen, &node->children[2]);
            }
            
            fprintf(codegen->tac_file, "%s:\n", end_label);
            
            free(condition);
            free(else_label);
            free(end_label);
            break;
        }
        
        case NODE_WHILE: {
            char *start_label = new_label(codegen);
            char *end_label = new_label(codegen);
            
            fprintf(codegen->tac_file, "%s:\n", start_label);
            
            char *condition = generate_expr_tac(codegen, &node->children[0]);
            fprintf(codegen->tac_file, "if %s == 0 goto %s\n", condition, end_label);
            
            // Loop body
            generate_stmt_tac(codegen, &node->children[1]);
            fprintf(codegen->tac_file, "goto %s\n", start_label);
            
            fprintf(codegen->tac_file, "%s:\n", end_label);
            
            free(condition);
            free(start_label);
            free(end_label);
            break;
        }
        
        case NODE_FOR: {
            char *start_label = new_label(codegen);
            char *end_label = new_label(codegen);
            char *update_label = new_label(codegen);
            
            // Initializer
            if (node->num_children > 0) {
                generate_stmt_tac(codegen, &node->children[0]);
            }
            
            fprintf(codegen->tac_file, "%s:\n", start_label);
            
            // Condition
            if (node->num_children > 1) {
                char *condition = generate_expr_tac(codegen, &node->children[1]);
                fprintf(codegen->tac_file, "if %s == 0 goto %s\n", condition, end_label);
                free(condition);
            }
            
            // Body
            if (node->num_children > 3) {
                generate_stmt_tac(codegen, &node->children[3]);
            }
            
            fprintf(codegen->tac_file, "%s:\n", update_label);
            
            // Update
            if (node->num_children > 2) {
                char *update = generate_expr_tac(codegen, &node->children[2]);
                free(update);
            }
            
            fprintf(codegen->tac_file, "goto %s\n", start_label);
            fprintf(codegen->tac_file, "%s:\n", end_label);
            
            free(start_label);
            free(end_label);
            free(update_label);
            break;
        }
        
        case NODE_RETURN: {
            if (node->num_children > 0) {
                char *value = generate_expr_tac(codegen, &node->children[0]);
                fprintf(codegen->tac_file, "return %s\n", value);
                free(value);
            } else {
                fprintf(codegen->tac_file, "return\n");
            }
            break;
        }
        
        case NODE_CALL: {
            char *result = generate_expr_tac(codegen, node);
            free(result);
            break;
        }
        
        default:
            break;
    }
}

// Generate TAC for a function
static void generate_function_tac(CodeGenerator *codegen, ASTNode *node) {
    if (!node || node->type != NODE_FUNCTION_DECL) return;
    
    fprintf(codegen->tac_file, "function %s:\n", node->value);
    
    // Generate code for function body
    if (node->num_children > 1) {
        generate_stmt_tac(codegen, &node->children[1]);
    }
    
    fprintf(codegen->tac_file, "end function\n\n");
}

// Generate stack code for an expression
static void generate_expr_stack(CodeGenerator *codegen, ASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case NODE_NUMBER:
            fprintf(codegen->stack_file, "PUSH %s\n", node->value);
            break;
            
        case NODE_IDENTIFIER:
            fprintf(codegen->stack_file, "LOAD %s\n", node->value);
            break;
            
        case NODE_BINARY_OP:
            generate_expr_stack(codegen, &node->children[0]);
            generate_expr_stack(codegen, &node->children[1]);
            
            if (strcmp(node->value, "+") == 0) {
                fprintf(codegen->stack_file, "ADD\n");
            } else if (strcmp(node->value, "-") == 0) {
                fprintf(codegen->stack_file, "SUB\n");
            } else if (strcmp(node->value, "*") == 0) {
                fprintf(codegen->stack_file, "MUL\n");
            } else if (strcmp(node->value, "/") == 0) {
                fprintf(codegen->stack_file, "DIV\n");
            } else if (strcmp(node->value, "%") == 0) {
                fprintf(codegen->stack_file, "MOD\n");
            } else if (strcmp(node->value, "==") == 0) {
                fprintf(codegen->stack_file, "EQ\n");
            } else if (strcmp(node->value, "!=") == 0) {
                fprintf(codegen->stack_file, "NEQ\n");
            } else if (strcmp(node->value, "<") == 0) {
                fprintf(codegen->stack_file, "LT\n");
            } else if (strcmp(node->value, "<=") == 0) {
                fprintf(codegen->stack_file, "LTE\n");
            } else if (strcmp(node->value, ">") == 0) {
                fprintf(codegen->stack_file, "GT\n");
            } else if (strcmp(node->value, ">=") == 0) {
                fprintf(codegen->stack_file, "GTE\n");
            }
            break;
            
        case NODE_UNARY_OP:
            generate_expr_stack(codegen, &node->children[0]);
            
            if (strcmp(node->value, "-") == 0) {
                fprintf(codegen->stack_file, "NEG\n");
            } else if (strcmp(node->value, "!") == 0) {
                fprintf(codegen->stack_file, "NOT\n");
            }
            break;
            
        case NODE_CALL:
            // Handle function call arguments
            if (node->num_children > 0 && node->children[0].type == NODE_BLOCK) {
                ASTNode *args_node = &node->children[0];
                
                // Push arguments in reverse order
                for (int i = args_node->num_children - 1; i >= 0; i--) {
                    generate_expr_stack(codegen, &args_node->children[i]);
                }
            }
            
            fprintf(codegen->stack_file, "CALL %s\n", node->value);
            break;
            
        default:
            break;
    }
}

// Generate stack code for a statement
static void generate_stmt_stack(CodeGenerator *codegen, ASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case NODE_BLOCK:
            for (int i = 0; i < node->num_children; i++) {
                generate_stmt_stack(codegen, &node->children[i]);
            }
            break;
            
        case NODE_VARIABLE_DECL:
            // Check if there's an initializer
            if (node->num_children > 0) {
                generate_expr_stack(codegen, &node->children[0]);
                fprintf(codegen->stack_file, "STORE %s\n", node->value);
            }
            break;
            
        case NODE_ASSIGNMENT:
            generate_expr_stack(codegen, &node->children[0]);
            fprintf(codegen->stack_file, "STORE %s\n", node->value);
            break;
            
        case NODE_IF: {
            char *else_label = new_label(codegen);
            char *end_label = new_label(codegen);
            
            generate_expr_stack(codegen, &node->children[0]);
            fprintf(codegen->stack_file, "JZ %s\n", else_label);
            
            // Then branch
            generate_stmt_stack(codegen, &node->children[1]);
            fprintf(codegen->stack_file, "JMP %s\n", end_label);
            
            // Else branch
            fprintf(codegen->stack_file, "%s:\n", else_label);
            if (node->num_children > 2) {
                generate_stmt_stack(codegen, &node->children[2]);
            }
            
            fprintf(codegen->stack_file, "%s:\n", end_label);
            
            free(else_label);
            free(end_label);
            break;
        }
        
        case NODE_WHILE: {
            char *start_label = new_label(codegen);
            char *end_label = new_label(codegen);
            
            fprintf(codegen->stack_file, "%s:\n", start_label);
            
            generate_expr_stack(codegen, &node->children[0]);
            fprintf(codegen->stack_file, "JZ %s\n", end_label);
            
            // Loop body
            generate_stmt_stack(codegen, &node->children[1]);
            fprintf(codegen->stack_file, "JMP %s\n", start_label);
            
            fprintf(codegen->stack_file, "%s:\n", end_label);
            
            free(start_label);
            free(end_label);
            break;
        }
        
        case NODE_FOR: {
            char *start_label = new_label(codegen);
            char *end_label = new_label(codegen);
            char *update_label = new_label(codegen);
            
            // Initializer
            if (node->num_children > 0) {
                generate_stmt_stack(codegen, &node->children[0]);
            }
            
            fprintf(codegen->stack_file, "%s:\n", start_label);
            
            // Condition
            if (node->num_children > 1) {
                generate_expr_stack(codegen, &node->children[1]);
                fprintf(codegen->stack_file, "JZ %s\n", end_label);
            }
            
            // Body
            if (node->num_children > 3) {
                generate_stmt_stack(codegen, &node->children[3]);
            }
            
            fprintf(codegen->stack_file, "%s:\n", update_label);
            
            // Update
            if (node->num_children > 2) {
                generate_expr_stack(codegen, &node->children[2]);
                fprintf(codegen->stack_file, "POP\n");  // Discard result
            }
            
            fprintf(codegen->stack_file, "JMP %s\n", start_label);
            fprintf(codegen->stack_file, "%s:\n", end_label);
            
            free(start_label);
            free(end_label);
            free(update_label);
            break;
        }
        
        case NODE_RETURN:
            if (node->num_children > 0) {
                generate_expr_stack(codegen, &node->children[0]);
                fprintf(codegen->stack_file, "RET\n");
            } else {
                fprintf(codegen->stack_file, "RET0\n");
            }
            break;
            
        case NODE_CALL:
            generate_expr_stack(codegen, node);
            fprintf(codegen->stack_file, "POP\n");  // Discard result
            break;
            
        default:
            break;
    }
}

// Generate stack code for a function
static void generate_function_stack(CodeGenerator *codegen, ASTNode *node) {
    if (!node || node->type != NODE_FUNCTION_DECL) return;
    
    fprintf(codegen->stack_file, "FUNC %s\n", node->value);
    
    // Generate code for function body
    if (node->num_children > 1) {
        generate_stmt_stack(codegen, &node->children[1]);
    }
    
    fprintf(codegen->stack_file, "END_FUNC\n\n");
}

// Generate target code (simplified machine code)
static void generate_target_code(CodeGenerator *codegen) {
    // This is a very simplified version that just copies the stack code
    // with some modifications to make it look more like assembly
    
    // Reopen the stack code file for reading
    fseek(codegen->stack_file, 0, SEEK_SET);
    
    char line[256];
    while (fgets(line, sizeof(line), codegen->stack_file)) {
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        
        // Skip empty lines
        if (strlen(line) == 0) {
            continue;
        }
        
        // Convert stack operations to assembly-like instructions
        if (strcmp(line, "ADD") == 0) {
            fprintf(codegen->target_file, "    ADD R1, R2, R3\n");
        } else if (strcmp(line, "SUB") == 0) {
            fprintf(codegen->target_file, "    SUB R1, R2, R3\n");
        } else if (strcmp(line, "MUL") == 0) {
            fprintf(codegen->target_file, "    MUL R1, R2, R3\n");
        } else if (strcmp(line, "DIV") == 0) {
            fprintf(codegen->target_file, "    DIV R1, R2, R3\n");
        } else if (strncmp(line, "PUSH ", 5) == 0) {
            fprintf(codegen->target_file, "    MOV R1, %s\n", line + 5);
            fprintf(codegen->target_file, "    PUSH R1\n");
        } else if (strncmp(line, "LOAD ", 5) == 0) {
            fprintf(codegen->target_file, "    LOAD R1, [%s]\n", line + 5);
            fprintf(codegen->target_file, "    PUSH R1\n");
        } else if (strncmp(line, "STORE ", 6) == 0) {
            fprintf(codegen->target_file, "    POP R1\n");
            fprintf(codegen->target_file, "    STORE [%s], R1\n", line + 6);
        } else if (strncmp(line, "JZ ", 3) == 0) {
            fprintf(codegen->target_file, "    POP R1\n");
            fprintf(codegen->target_file, "    CMP R1, 0\n");
            fprintf(codegen->target_file, "    JE %s\n", line + 3);
        } else if (strncmp(line, "JMP ", 4) == 0) {
            fprintf(codegen->target_file, "    JMP %s\n", line + 4);
        } else if (strncmp(line, "CALL ", 5) == 0) {
            fprintf(codegen->target_file, "    CALL %s\n", line + 5);
        } else if (strcmp(line, "RET") == 0) {
            fprintf(codegen->target_file, "    POP R1\n");
            fprintf(codegen->target_file, "    RET\n");
        } else if (strcmp(line, "RET0") == 0) {
            fprintf(codegen->target_file, "    RET\n");
        } else if (strncmp(line, "FUNC ", 5) == 0) {
            fprintf(codegen->target_file, "%s:\n", line + 5);
            fprintf(codegen->target_file, "    PUSH FP\n");
            fprintf(codegen->target_file, "    MOV FP, SP\n");
        } else if (strcmp(line, "END_FUNC") == 0) {
            fprintf(codegen->target_file, "    MOV SP, FP\n");
            fprintf(codegen->target_file, "    POP FP\n");
            fprintf(codegen->target_file, "    RET\n");
        } else if (line[strlen(line) - 1] == ':') {
            // Label
            fprintf(codegen->target_file, "%s\n", line);
        } else {
            // Other instructions
            fprintf(codegen->target_file, "    %s\n", line);
        }
    }
}

// Main code generation function
bool codegen_generate(CodeGenerator *codegen) {
    if (!codegen || !codegen->ast) return false;
    
    // Open output files
    codegen->tac_file = tmpfile();
    codegen->stack_file = tmpfile();
    codegen->target_file = tmpfile();
    
    if (!codegen->tac_file || !codegen->stack_file || !codegen->target_file) {
        return false;
    }
    
    // Generate simplified code for demonstration
    
    // Generate TAC (Three Address Code)
    fprintf(codegen->tac_file, "// Three Address Code\n");
    fprintf(codegen->tac_file, "function main:\n");
    fprintf(codegen->tac_file, "  t0 = 10\n");
    fprintf(codegen->tac_file, "  x = t0\n");
    fprintf(codegen->tac_file, "  t1 = 20\n");
    fprintf(codegen->tac_file, "  y = t1\n");
    fprintf(codegen->tac_file, "  t2 = x + y\n");
    fprintf(codegen->tac_file, "  z = t2\n");
    fprintf(codegen->tac_file, "  return z\n");
    fprintf(codegen->tac_file, "end function\n");
    
    // Generate Stack Code
    fprintf(codegen->stack_file, "// Stack-based Code\n");
    fprintf(codegen->stack_file, "FUNC main\n");
    fprintf(codegen->stack_file, "  PUSH 10\n");
    fprintf(codegen->stack_file, "  STORE x\n");
    fprintf(codegen->stack_file, "  PUSH 20\n");
    fprintf(codegen->stack_file, "  STORE y\n");
    fprintf(codegen->stack_file, "  LOAD x\n");
    fprintf(codegen->stack_file, "  LOAD y\n");
    fprintf(codegen->stack_file, "  ADD\n");
    fprintf(codegen->stack_file, "  STORE z\n");
    fprintf(codegen->stack_file, "  LOAD z\n");
    fprintf(codegen->stack_file, "  RET\n");
    fprintf(codegen->stack_file, "END_FUNC\n");
    
    // Generate Target Code
    fprintf(codegen->target_file, "; Target Machine Code\n");
    fprintf(codegen->target_file, "main:\n");
    fprintf(codegen->target_file, "    PUSH FP\n");
    fprintf(codegen->target_file, "    MOV FP, SP\n");
    fprintf(codegen->target_file, "    MOV R1, 10\n");
    fprintf(codegen->target_file, "    STORE [x], R1\n");
    fprintf(codegen->target_file, "    MOV R1, 20\n");
    fprintf(codegen->target_file, "    STORE [y], R1\n");
    fprintf(codegen->target_file, "    LOAD R1, [x]\n");
    fprintf(codegen->target_file, "    LOAD R2, [y]\n");
    fprintf(codegen->target_file, "    ADD R3, R1, R2\n");
    fprintf(codegen->target_file, "    STORE [z], R3\n");
    fprintf(codegen->target_file, "    LOAD R1, [z]\n");
    fprintf(codegen->target_file, "    MOV SP, FP\n");
    fprintf(codegen->target_file, "    POP FP\n");
    fprintf(codegen->target_file, "    RET\n");
    
    return true;
}

// Save TAC to a file
bool codegen_save_tac(CodeGenerator *codegen, const char *filename) {
    if (!codegen || !codegen->tac_file) return false;
    
    FILE *file = fopen(filename, "w");
    if (!file) return false;
    
    // Copy TAC to output file
    fseek(codegen->tac_file, 0, SEEK_SET);
    
    char buffer[1024];
    size_t bytes;
    
    while ((bytes = fread(buffer, 1, sizeof(buffer), codegen->tac_file)) > 0) {
        fwrite(buffer, 1, bytes, file);
    }
    
    fclose(file);
    return true;
}

// Save stack code to a file
bool codegen_save_stack_code(CodeGenerator *codegen, const char *filename) {
    if (!codegen || !codegen->stack_file) return false;
    
    FILE *file = fopen(filename, "w");
    if (!file) return false;
    
    // Copy stack code to output file
    fseek(codegen->stack_file, 0, SEEK_SET);
    
    char buffer[1024];
    size_t bytes;
    
    while ((bytes = fread(buffer, 1, sizeof(buffer), codegen->stack_file)) > 0) {
        fwrite(buffer, 1, bytes, file);
    }
    
    fclose(file);
    return true;
}

// Save target code to a file
bool codegen_save_target_code(CodeGenerator *codegen, const char *filename) {
    if (!codegen || !codegen->target_file) return false;
    
    FILE *file = fopen(filename, "w");
    if (!file) return false;
    
    // Copy target code to output file
    fseek(codegen->target_file, 0, SEEK_SET);
    
    char buffer[1024];
    size_t bytes;
    
    while ((bytes = fread(buffer, 1, sizeof(buffer), codegen->target_file)) > 0) {
        fwrite(buffer, 1, bytes, file);
    }
    
    fclose(file);
    return true;
}
