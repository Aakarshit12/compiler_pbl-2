# Compiler Visualizer

A C-style compiler pipeline visualization tool for educational purposes. This tool demonstrates the various stages of compilation from lexical analysis to code generation, with an interactive GUI to explore each phase.

## Features

- **C Backend**: Complete compiler implementation in C with modular stages
  - Custom lexer for tokenization
  - Two parser implementations: Recursive Descent and LALR
  - AST generation and visualization
  - Three Address Code (TAC) generation
  - Stack-based code generation
  - Target machine code generation

- **Python GUI Frontend**: Interactive visualization of compilation phases
  - Code editor with syntax highlighting
  - Token viewer
  - AST visualization (using Graphviz)
  - Three Address Code viewer
  - Stack code viewer
  - Target code viewer

## Project Structure

```
compiler_visualizer/
├── backend/                  # C compiler implementation
│   ├── src/                  # Source files
│   │   ├── lexer.c/h         # Lexer implementation
│   │   ├── parser_rd.c/h     # Recursive Descent Parser
│   │   ├── parser_lalr.c/h   # LALR Parser
│   │   ├── ast.c/h           # AST generation and utilities
│   │   ├── codegen.c/h       # Code generation (TAC, stack, target)
│   │   ├── common.h          # Common definitions
│   │   └── main.c            # Main compiler driver
│   └── Makefile              # Build system
├── frontend/                 # Python GUI
│   ├── gui.py                # Main GUI application
│   ├── ast_visualizer.py     # AST visualization
│   └── requirements.txt      # Python dependencies
├── build/                    # Build output
└── README.md                 # This file
```

## Prerequisites

### For the C Backend:
- GCC or compatible C compiler
- Make

### For the Python Frontend:
- Python 3.6+
- Tkinter
- Pillow (PIL)
- ttkbootstrap (optional, for enhanced UI)
- Graphviz (for AST visualization)

## Building and Running

### Step 1: Build the C Compiler Backend

```bash
cd compiler_visualizer/backend
make
```

This will create the `my_compiler` executable in the `build` directory.

### Step 2: Install Python Dependencies

```bash
cd compiler_visualizer/frontend
pip install -r requirements.txt
```

### Step 3: Install Graphviz

#### macOS:
```bash
brew install graphviz
```

#### Linux:
```bash
sudo apt-get install graphviz
```

#### Windows:
Download and install from the [Graphviz website](https://graphviz.org/download/).

### Step 4: Run the GUI

```bash
cd compiler_visualizer/frontend
python gui.py
```

## Using the Compiler Visualizer

1. Enter C code in the editor or use the "Load Example" button for a sample
2. Select the parser type (Recursive Descent or LALR)
3. Click "Compile" to process the code
4. Navigate through the tabs to see different compilation phases:
   - Tokens: View the lexical tokens
   - AST: Visualize the Abstract Syntax Tree
   - Three Address Code: View the intermediate representation
   - Stack Code: See the stack-based code
   - Target Code: Examine the final target code

## Command Line Usage

The compiler can also be used directly from the command line:

```bash
./build/my_compiler --input input.c --parser=rd --output-dir ./output
```

Options:
- `--input <file>`: Input source file (required)
- `--parser <type>`: Parser type: 'rd' (recursive descent) or 'lalr' (default: rd)
- `--output-dir <dir>`: Output directory for generated files (default: current directory)
- `--verbose`: Enable verbose output
- `--help`: Display help message

## License

This project is provided for educational purposes.

## Acknowledgments

- This tool was designed for educational use to help understand compiler construction
- The implementation is simplified for clarity and does not include all optimizations or features of a production compiler
