#ifndef CODEGEN_H
#define CODEGEN_H

#include "common.h"
#include "ast.h"

// Code generator structure
typedef struct {
    ASTNode *ast;
    FILE *tac_file;
    FILE *stack_file;
    FILE *target_file;
    int temp_var_count;
    int label_count;
} CodeGenerator;

// Code generator functions
CodeGenerator* codegen_init(ASTNode *ast);
void codegen_free(CodeGenerator *codegen);
bool codegen_generate(CodeGenerator *codegen);
bool codegen_save_tac(CodeGenerator *codegen, const char *filename);
bool codegen_save_stack_code(CodeGenerator *codegen, const char *filename);
bool codegen_save_target_code(CodeGenerator *codegen, const char *filename);

#endif // CODEGEN_H
