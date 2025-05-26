#include "common.h"
#include "lexer.h"
#include "parser_rd.h"
#include "parser_lalr.h"
#include "ast.h"
#include "codegen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

// Print usage information
static void print_usage(const char *program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  --input <file>       Input source file (required)\n");
    printf("  --parser <type>      Parser type: 'rd' (recursive descent) or 'lalr' (default: rd)\n");
    printf("  --output-dir <dir>   Output directory for generated files (default: current directory)\n");
    printf("  --verbose            Enable verbose output\n");
    printf("  --help               Display this help message\n");
}

// Parse command-line arguments
static bool parse_args(int argc, char *argv[], CompilerConfig *config) {
    static struct option long_options[] = {
        {"input", required_argument, 0, 'i'},
        {"parser", required_argument, 0, 'p'},
        {"output-dir", required_argument, 0, 'o'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    // Set default values
    config->input_file = NULL;
    config->output_dir = ".";
    config->parser_type = PARSER_RD;
    config->verbose = false;
    
    int option_index = 0;
    int c;
    
    while ((c = getopt_long(argc, argv, "i:p:o:vh", long_options, &option_index)) != -1) {
        switch (c) {
            case 'i':
                config->input_file = strdup(optarg);
                break;
                
            case 'p':
                if (strcmp(optarg, "rd") == 0) {
                    config->parser_type = PARSER_RD;
                } else if (strcmp(optarg, "lalr") == 0) {
                    config->parser_type = PARSER_LALR;
                } else {
                    fprintf(stderr, "Invalid parser type: %s\n", optarg);
                    return false;
                }
                break;
                
            case 'o':
                config->output_dir = strdup(optarg);
                break;
                
            case 'v':
                config->verbose = true;
                break;
                
            case 'h':
                print_usage(argv[0]);
                exit(0);
                
            case '?':
                return false;
                
            default:
                fprintf(stderr, "Unknown option: %c\n", c);
                return false;
        }
    }
    
    // Check required arguments
    if (!config->input_file) {
        fprintf(stderr, "Error: Input file is required\n");
        return false;
    }
    
    return true;
}

// Read the entire contents of a file
static char* read_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open file '%s'\n", filename);
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate buffer
    char *buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        return NULL;
    }
    
    // Read file contents
    size_t bytes_read = fread(buffer, 1, file_size, file);
    if (bytes_read < (size_t)file_size) {
        fprintf(stderr, "Error: Could not read entire file\n");
        free(buffer);
        fclose(file);
        return NULL;
    }
    
    buffer[file_size] = '\0';
    fclose(file);
    
    return buffer;
}

// Build output file path
static char* build_output_path(const char *output_dir, const char *filename) {
    size_t dir_len = strlen(output_dir);
    size_t file_len = strlen(filename);
    
    // +2 for the separator and null terminator
    char *path = (char*)malloc(dir_len + file_len + 2);
    if (!path) return NULL;
    
    strcpy(path, output_dir);
    
    // Add separator if needed
    if (dir_len > 0 && output_dir[dir_len - 1] != '/') {
        path[dir_len] = '/';
        path[dir_len + 1] = '\0';
    }
    
    strcat(path, filename);
    
    return path;
}

int main(int argc, char *argv[]) {
    CompilerConfig config;
    
    // Parse command-line arguments
    if (!parse_args(argc, argv, &config)) {
        print_usage(argv[0]);
        return 1;
    }
    
    if (config.verbose) {
        printf("Input file: %s\n", config.input_file);
        printf("Parser type: %s\n", config.parser_type == PARSER_RD ? "recursive descent" : "LALR");
        printf("Output directory: %s\n", config.output_dir);
    }
    
    // Read input file
    char *source = read_file(config.input_file);
    if (!source) {
        return 1;
    }
    
    // Initialize lexer
    Lexer *lexer = lexer_init(source);
    if (!lexer) {
        fprintf(stderr, "Error: Could not initialize lexer\n");
        free(source);
        return 1;
    }
    
    // Tokenize input
    if (!lexer_tokenize(lexer)) {
        fprintf(stderr, "Error: Tokenization failed\n");
        lexer_free(lexer);
        free(source);
        return 1;
    }
    
    // Save tokens to file
    char *tokens_path = build_output_path(config.output_dir, "tokens.txt");
    if (!tokens_path || !lexer_save_tokens(lexer, tokens_path)) {
        fprintf(stderr, "Error: Could not save tokens to file\n");
        free(tokens_path);
        lexer_free(lexer);
        free(source);
        return 1;
    }
    
    // Save tokens to JSON file
    char *tokens_json_path = build_output_path(config.output_dir, "tokens.json");
    if (!tokens_json_path || !lexer_save_tokens_json(lexer, tokens_json_path)) {
        fprintf(stderr, "Error: Could not save tokens to JSON file\n");
        free(tokens_json_path);
        free(tokens_path);
        lexer_free(lexer);
        free(source);
        return 1;
    }
    
    if (config.verbose) {
        printf("Tokens saved to %s and %s\n", tokens_path, tokens_json_path);
    }
    
    // Get tokens
    size_t num_tokens;
    Token *tokens = lexer_get_tokens(lexer, &num_tokens);
    
    // Parse the tokens
    ASTNode *ast = NULL;
    bool parse_error = false;
    
    if (config.parser_type == PARSER_RD) {
        RDParser *parser = parser_rd_init(tokens, num_tokens);
        if (!parser) {
            fprintf(stderr, "Error: Could not initialize recursive descent parser\n");
            free(tokens_json_path);
            free(tokens_path);
            lexer_free(lexer);
            free(source);
            return 1;
        }
        
        ast = parser_rd_parse(parser);
        parse_error = parser_rd_had_error(parser);
        
        if (parse_error) {
            fprintf(stderr, "Error: Parsing failed: %s\n", parser_rd_get_error(parser));
        }
        
        parser_rd_free(parser);
    } else {
        LALRParser *parser = parser_lalr_init(tokens, num_tokens);
        if (!parser) {
            fprintf(stderr, "Error: Could not initialize LALR parser\n");
            free(tokens_json_path);
            free(tokens_path);
            lexer_free(lexer);
            free(source);
            return 1;
        }
        
        ast = parser_lalr_parse(parser);
        parse_error = parser_lalr_had_error(parser);
        
        if (parse_error) {
            fprintf(stderr, "Error: Parsing failed: %s\n", parser_lalr_get_error(parser));
        }
        
        parser_lalr_free(parser);
    }
    
    if (!ast || parse_error) {
        fprintf(stderr, "Error: Could not generate AST\n");
        free(tokens_json_path);
        free(tokens_path);
        lexer_free(lexer);
        free(source);
        return 1;
    }
    
    // Save AST to files
    char *ast_path = build_output_path(config.output_dir, "ast.txt");
    char *ast_dot_path = build_output_path(config.output_dir, "ast.dot");
    char *ast_json_path = build_output_path(config.output_dir, "ast.json");
    
    if (!ast_path || !ast_save_to_file(ast, ast_path) ||
        !ast_dot_path || !ast_save_to_dot(ast, ast_dot_path) ||
        !ast_json_path || !ast_save_to_json(ast, ast_json_path)) {
        fprintf(stderr, "Error: Could not save AST to files\n");
        free(ast_json_path);
        free(ast_dot_path);
        free(ast_path);
        ast_free_node(ast);
        free(tokens_json_path);
        free(tokens_path);
        lexer_free(lexer);
        free(source);
        return 1;
    }
    
    if (config.verbose) {
        printf("AST saved to %s, %s, and %s\n", ast_path, ast_dot_path, ast_json_path);
    }
    
    // Generate code
    CodeGenerator *codegen = codegen_init(ast);
    if (!codegen) {
        fprintf(stderr, "Error: Could not initialize code generator\n");
        free(ast_json_path);
        free(ast_dot_path);
        free(ast_path);
        ast_free_node(ast);
        free(tokens_json_path);
        free(tokens_path);
        lexer_free(lexer);
        free(source);
        return 1;
    }
    
    if (!codegen_generate(codegen)) {
        fprintf(stderr, "Error: Code generation failed\n");
        codegen_free(codegen);
        free(ast_json_path);
        free(ast_dot_path);
        free(ast_path);
        ast_free_node(ast);
        free(tokens_json_path);
        free(tokens_path);
        lexer_free(lexer);
        free(source);
        return 1;
    }
    
    // Save generated code to files
    char *tac_path = build_output_path(config.output_dir, "tac.txt");
    char *stack_path = build_output_path(config.output_dir, "stack_code.txt");
    char *target_path = build_output_path(config.output_dir, "target_code.txt");
    
    if (!tac_path || !codegen_save_tac(codegen, tac_path) ||
        !stack_path || !codegen_save_stack_code(codegen, stack_path) ||
        !target_path || !codegen_save_target_code(codegen, target_path)) {
        fprintf(stderr, "Error: Could not save generated code to files\n");
        free(target_path);
        free(stack_path);
        free(tac_path);
        codegen_free(codegen);
        free(ast_json_path);
        free(ast_dot_path);
        free(ast_path);
        ast_free_node(ast);
        free(tokens_json_path);
        free(tokens_path);
        lexer_free(lexer);
        free(source);
        return 1;
    }
    
    if (config.verbose) {
        printf("Generated code saved to %s, %s, and %s\n", tac_path, stack_path, target_path);
    }
    
    // Clean up (with safety checks)
    if (target_path) free(target_path);
    if (stack_path) free(stack_path);
    if (tac_path) free(tac_path);
    if (codegen) codegen_free(codegen);
    if (ast_json_path) free(ast_json_path);
    if (ast_dot_path) free(ast_dot_path);
    if (ast_path) free(ast_path);
    // Don't free ast here as it's already freed by codegen_free
    if (tokens_json_path) free(tokens_json_path);
    if (tokens_path) free(tokens_path);
    if (lexer) lexer_free(lexer);
    if (source) free(source);
    
    printf("Compilation completed successfully.\n");
    
    return 0;
}
