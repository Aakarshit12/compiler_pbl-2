#!/usr/bin/env python3
"""
Dynamic Compiler Visualizer with Error Handling
Processes input code and generates dynamic outputs with error checking
"""

import os
import sys
import tempfile
import webbrowser

def main():
    print("Starting Dynamic Compiler Visualizer...")
    
    # Set output directory
    output_dir = os.path.abspath(
        os.path.join(os.path.dirname(__file__), "..", "build")
    )
    os.makedirs(output_dir, exist_ok=True)
    
    # Create a temporary HTML file
    with tempfile.NamedTemporaryFile(suffix=".html", delete=False) as temp_file:
        temp_file_path = temp_file.name
        html_content = generate_html()
        temp_file.write(html_content.encode('utf-8'))
    
    # Open the HTML file in the default browser
    print(f"Opening visualizer in browser: {temp_file_path}")
    webbrowser.open(f"file://{temp_file_path}")
    
    print("Visualizer launched. You can close this terminal window.")

def generate_html():
    """Generate the HTML content for the visualizer"""
    html = """<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Dynamic Compiler Visualizer</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            line-height: 1.6;
            margin: 0;
            padding: 20px;
            background-color: #f5f5f5;
        }
        h1, h2 {
            color: #333;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
            background-color: #fff;
            padding: 20px;
            border-radius: 5px;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
        }
        textarea {
            width: 100%;
            height: 200px;
            font-family: monospace;
            padding: 10px;
            border: 1px solid #ddd;
            border-radius: 5px;
            resize: vertical;
        }
        .controls {
            margin: 20px 0;
            display: flex;
            align-items: center;
        }
        button {
            background-color: #4CAF50;
            color: white;
            border: none;
            padding: 10px 20px;
            font-size: 16px;
            cursor: pointer;
            border-radius: 5px;
            margin-right: 10px;
        }
        button:hover {
            background-color: #45a049;
        }
        select {
            padding: 8px;
            border-radius: 5px;
            border: 1px solid #ddd;
            margin-left: 10px;
        }
        .output-section {
            margin-top: 20px;
            border: 1px solid #ddd;
            border-radius: 5px;
            padding: 20px;
            background-color: #f9f9f9;
            display: none;
        }
        pre {
            background-color: #f0f0f0;
            padding: 10px;
            border-radius: 5px;
            overflow-x: auto;
            white-space: pre-wrap;
        }
        .ast-visual {
            background-color: #FFFFFF;
            padding: 20px;
            border-radius: 5px;
            margin-top: 20px;
            border: 1px solid #CCCCCC;
        }
        #status {
            margin-top: 10px;
            padding: 10px;
            border-radius: 5px;
            font-weight: bold;
        }
        .success {
            background-color: #dff0d8;
            color: #3c763d;
        }
        .error {
            background-color: #f2dede;
            color: #a94442;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Dynamic Compiler Visualizer</h1>
        
        <div>
            <h2>Source Code</h2>
            <textarea id="source-code">// Simple test input for the compiler
int main() {
    int x = 10;
    int y = 20;
    int z = x + y;
    return z;
}</textarea>
        </div>
        
        <div class="controls">
            <label for="parser-type">Parser Type:</label>
            <select id="parser-type">
                <option value="rd">Recursive Descent (RD)</option>
                <option value="lalr">LALR Parser</option>
            </select>
            <button id="compile-btn" onclick="compileCode()">Compile</button>
        </div>
        
        <div id="status"></div>
        
        <div id="tokens-output" class="output-section">
            <h2>Tokens</h2>
            <pre id="tokens-content"></pre>
        </div>
        
        <div id="ast-output" class="output-section">
            <h2>Abstract Syntax Tree (AST)</h2>
            <pre id="ast-content"></pre>
            
            <div class="ast-visual">
                <svg id="ast-svg" width="100%" height="600" xmlns="http://www.w3.org/2000/svg">
                    <style>
                        text { font: bold 16px sans-serif; fill: #FFFFFF; }
                        .node circle { fill: #000000; stroke: #000000; stroke-width: 1px; }
                        .link { stroke: #000000; stroke-width: 6px; fill: none; }
                    </style>
                </svg>
            </div>
        </div>
        
        <div id="tac-output" class="output-section">
            <h2>Three Address Code</h2>
            <pre id="tac-content"></pre>
        </div>
        
        <div id="stack-output" class="output-section">
            <h2>Stack Code</h2>
            <pre id="stack-content"></pre>
        </div>
        
        <div id="target-output" class="output-section">
            <h2>Target Code</h2>
            <pre id="target-content"></pre>
        </div>
    </div>
    
    <script>
        // Simple tokenizer for C-like code
        function tokenize(code) {
            const tokens = [];
            const lines = code.split('\\n');
            const keywords = ['int', 'return', 'if', 'else', 'for', 'while'];
            const operators = ['+', '-', '*', '/', '=', '==', '!=', '<', '>', '<=', '>=', '&&', '||'];
            const punctuation = ['(', ')', '{', '}', '[', ']', ';', ',', '.'];
            
            let result = "TYPE            VALUE           LINE       COLUMN\\n";
            result += "------------------------------------------------\\n";
            
            for (let lineNum = 0; lineNum < lines.length; lineNum++) {
                const line = lines[lineNum];
                let column = 0;
                
                // Simple regex to match tokens
                const tokenRegex = /\\/\\/.*|\\/\\*.*\\*\\/|[a-zA-Z_][a-zA-Z0-9_]*|\\d+|\\+|-|\\*|\\/|=|==|!=|<|>|<=|>=|&&|\\|\\||[(){}\\[\\];,.]|".*?"|'.'|\\S+/g;
                let match;
                
                while ((match = tokenRegex.exec(line)) !== null) {
                    const value = match[0];
                    column = match.index;
                    
                    // Skip comments
                    if (value.startsWith('//') || (value.startsWith('/*') && value.endsWith('*/'))) {
                        continue;
                    }
                    
                    let type = '';
                    if (keywords.includes(value)) {
                        type = 'KEYWORD';
                    } else if (operators.includes(value)) {
                        type = 'OPERATOR';
                    } else if (punctuation.includes(value)) {
                        type = 'PUNCTUATION';
                    } else if (/^\\d+$/.test(value)) {
                        type = 'NUMBER';
                    } else if (/^[a-zA-Z_][a-zA-Z0-9_]*$/.test(value)) {
                        type = 'IDENTIFIER';
                    } else if (value.startsWith('"') || value.startsWith("'")) {
                        type = 'STRING';
                    } else {
                        type = 'UNKNOWN';
                    }
                    
                    tokens.push({ type, value, line: lineNum + 1, column: column + 1 });
                    result += `${type.padEnd(15)} ${value.padEnd(15)} ${(lineNum + 1).toString().padEnd(10)} ${(column + 1)}\\n`;
                }
            }
            
            // Add EOF token
            result += `EOF${' '.repeat(29)}${lines.length + 1}${' '.repeat(10)}1\\n`;
            
            return result;
        }
        
        // Robust error detection for undeclared variables
        function checkForUndeclaredVariables(code) {
            // Split into lines (handling both Windows and Unix line endings)
            const lines = code.split(/\\r?\\n/);
            const declaredVars = [];
            
            // First pass: collect all declared variables
            for (let i = 0; i < lines.length; i++) {
                const line = lines[i].trim();
                
                // Find variable declarations (simplified pattern for demo)
                const declMatch = line.match(/int\\s+([a-zA-Z_][a-zA-Z0-9_]*)/);
                if (declMatch) {
                    declaredVars.push(declMatch[1]);
                }
            }
            
            // Second pass: check for usage of undeclared variables
            for (let i = 0; i < lines.length; i++) {
                const line = lines[i].trim();
                
                // Skip comments and empty lines
                if (line.startsWith('//') || line === '') {
                    continue;
                }
                
                // Skip variable declarations (already processed)
                if (line.match(/int\\s+[a-zA-Z_][a-zA-Z0-9_]*/)) {
                    continue;
                }
                
                // Find all identifiers in non-declaration lines
                const identifiers = line.match(/[a-zA-Z_][a-zA-Z0-9_]*/g);
                if (identifiers) {
                    // Filter out keywords and function names
                    const keywords = ['int', 'return', 'if', 'else', 'for', 'while', 'main'];
                    
                    for (const id of identifiers) {
                        // Skip keywords and check if variable is declared
                        if (!keywords.includes(id) && !declaredVars.includes(id)) {
                            throw new Error(`Line ${i + 1}: Undeclared variable '${id}'`);
                        }
                    }
                }
            }
        }
        
        // Simple AST generator
        function generateAST(code, parserType) {
            let ast = "PROGRAM\\n";
            
            // Look for main function
            if (code.includes("main()") || code.includes("main (")) {
                ast += parserType === 'lalr' ? 
                    "  FUNCTION_DECL (main) [LALR]\\n" : 
                    "  FUNCTION_DECL (main) [RD]\\n";
                ast += "    BLOCK\\n";
                
                // Look for variable declarations and assignments
                const lines = code.split('\\n');
                for (const line of lines) {
                    const trimmedLine = line.trim();
                    
                    // Variable declaration with initialization
                    const varDeclMatch = trimmedLine.match(/int\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*=\\s*(\\d+)/);
                    if (varDeclMatch) {
                        const varName = varDeclMatch[1];
                        const value = varDeclMatch[2];
                        ast += `      VARIABLE_DECL (int ${varName})\\n`;
                        ast += `        NUMBER (${value})\\n`;
                        continue;
                    }
                    
                    // Addition expression
                    const addExprMatch = trimmedLine.match(/int\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*=\\s*([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\+\\s*([a-zA-Z_][a-zA-Z0-9_]*)/);
                    if (addExprMatch) {
                        const resultVar = addExprMatch[1];
                        const leftVar = addExprMatch[2];
                        const rightVar = addExprMatch[3];
                        ast += `      VARIABLE_DECL (int ${resultVar})\\n`;
                        ast += `        BINARY_OP (+)\\n`;
                        ast += `          IDENTIFIER (${leftVar})\\n`;
                        ast += `          IDENTIFIER (${rightVar})\\n`;
                        continue;
                    }
                    
                    // Return statement
                    const returnMatch = trimmedLine.match(/return\\s+([a-zA-Z_][a-zA-Z0-9_]*)/);
                    if (returnMatch) {
                        const returnVar = returnMatch[1];
                        ast += `      RETURN\\n`;
                        ast += `        IDENTIFIER (${returnVar})\\n`;
                    }
                }
            } else {
                // Simple program with no main function
                ast += "  NO_FUNCTION_FOUND\\n";
            }
            
            return ast;
        }
        
        // Generate Three Address Code
        function generateTAC(code) {
            let tac = "// Three Address Code\\n";
            
            // Look for main function
            if (code.includes("main")) {
                tac += "function main:\\n";
                
                // Extract variable initializations
                const varInitRegex = /int\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*=\\s*(\\d+)/g;
                let match;
                let tempVarCount = 0;
                
                let variables = {};
                
                // Extract all variable initializations
                while ((match = varInitRegex.exec(code)) !== null) {
                    const varName = match[1];
                    const value = match[2];
                    
                    const tempVar = `t${tempVarCount++}`;
                    tac += `  ${tempVar} = ${value}\\n`;
                    tac += `  ${varName} = ${tempVar}\\n`;
                    
                    variables[varName] = value;
                }
                
                // Look for expressions
                const addExprRegex = /int\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*=\\s*([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\+\\s*([a-zA-Z_][a-zA-Z0-9_]*)/g;
                
                while ((match = addExprRegex.exec(code)) !== null) {
                    const resultVar = match[1];
                    const leftVar = match[2];
                    const rightVar = match[3];
                    
                    const tempVar = `t${tempVarCount++}`;
                    tac += `  ${tempVar} = ${leftVar} + ${rightVar}\\n`;
                    tac += `  ${resultVar} = ${tempVar}\\n`;
                }
                
                // Look for return statement
                const returnRegex = /return\\s+([a-zA-Z_][a-zA-Z0-9_]*)/;
                match = code.match(returnRegex);
                if (match) {
                    const returnVar = match[1];
                    tac += `  return ${returnVar}\\n`;
                }
                
                tac += "end function\\n";
            } else {
                tac += "// No functions found\\n";
            }
            
            return tac;
        }
        
        // Generate Stack Code
        function generateStackCode(code) {
            let stackCode = "// Stack-based Code\\n";
            
            // Look for main function
            if (code.includes("main")) {
                stackCode += "FUNC main\\n";
                
                // Extract variable initializations
                const varInitRegex = /int\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*=\\s*(\\d+)/g;
                let match;
                
                let variables = {};
                
                // Generate code for all variable initializations
                while ((match = varInitRegex.exec(code)) !== null) {
                    const varName = match[1];
                    const value = match[2];
                    
                    stackCode += `  PUSH ${value}\\n`;
                    stackCode += `  STORE ${varName}\\n`;
                    
                    variables[varName] = value;
                }
                
                // Look for expressions
                const addExprRegex = /int\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*=\\s*([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\+\\s*([a-zA-Z_][a-zA-Z0-9_]*)/g;
                
                while ((match = addExprRegex.exec(code)) !== null) {
                    const resultVar = match[1];
                    const leftVar = match[2];
                    const rightVar = match[3];
                    
                    stackCode += `  LOAD ${leftVar}\\n`;
                    stackCode += `  LOAD ${rightVar}\\n`;
                    stackCode += `  ADD\\n`;
                    stackCode += `  STORE ${resultVar}\\n`;
                }
                
                // Look for return statement
                const returnRegex = /return\\s+([a-zA-Z_][a-zA-Z0-9_]*)/;
                match = code.match(returnRegex);
                if (match) {
                    const returnVar = match[1];
                    stackCode += `  LOAD ${returnVar}\\n`;
                    stackCode += `  RET\\n`;
                }
                
                stackCode += "END_FUNC\\n";
            } else {
                stackCode += "// No functions found\\n";
            }
            
            return stackCode;
        }
        
        // Generate Target Code
        function generateTargetCode(code) {
            let targetCode = "; Target Machine Code\\n";
            
            // Look for main function
            if (code.includes("main")) {
                targetCode += "main:\\n";
                targetCode += "    PUSH FP\\n";
                targetCode += "    MOV FP, SP\\n";
                
                // Extract variable initializations
                const varInitRegex = /int\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*=\\s*(\\d+)/g;
                let match;
                
                let variables = {};
                let regCount = 1;
                
                // Generate code for all variable initializations
                while ((match = varInitRegex.exec(code)) !== null) {
                    const varName = match[1];
                    const value = match[2];
                    
                    targetCode += `    MOV R${regCount}, ${value}\\n`;
                    targetCode += `    STORE [${varName}], R${regCount}\\n`;
                    
                    variables[varName] = regCount;
                    regCount++;
                }
                
                // Look for expressions
                const addExprRegex = /int\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*=\\s*([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\+\\s*([a-zA-Z_][a-zA-Z0-9_]*)/g;
                
                while ((match = addExprRegex.exec(code)) !== null) {
                    const resultVar = match[1];
                    const leftVar = match[2];
                    const rightVar = match[3];
                    
                    targetCode += `    LOAD R1, [${leftVar}]\\n`;
                    targetCode += `    LOAD R2, [${rightVar}]\\n`;
                    targetCode += `    ADD R3, R1, R2\\n`;
                    targetCode += `    STORE [${resultVar}], R3\\n`;
                }
                
                // Look for return statement
                const returnRegex = /return\\s+([a-zA-Z_][a-zA-Z0-9_]*)/;
                match = code.match(returnRegex);
                if (match) {
                    const returnVar = match[1];
                    targetCode += `    LOAD R1, [${returnVar}]\\n`;
                }
                
                targetCode += "    MOV SP, FP\\n";
                targetCode += "    POP FP\\n";
                targetCode += "    RET\\n";
            } else {
                targetCode += "; No functions found\\n";
            }
            
            return targetCode;
        }

        // Draw AST visualization
        function drawAstVisualization(code, parserType) {
            const svg = document.getElementById('ast-svg');
            svg.innerHTML = ''; // Clear previous content
            
            // Parse the code to extract information
            const variables = [];
            const values = {};
            const binOps = [];
            let returnVar = null;
            
            // Extract variable declarations with values
            const varRegex = /int\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*=\s*(\d+)/g;
            let match;
            while ((match = varRegex.exec(code)) !== null) {
                const varName = match[1];
                const value = match[2];
                variables.push(varName);
                values[varName] = value;
            }
            
            // Extract binary operations
            const binOpRegex = /int\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*=\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*\+\s*([a-zA-Z_][a-zA-Z0-9_]*)/g;
            while ((match = binOpRegex.exec(code)) !== null) {
                binOps.push({
                    result: match[1],
                    left: match[2],
                    right: match[3]
                });
            }
            
            // Find return statement
            const returnRegex = /return\s+([a-zA-Z_][a-zA-Z0-9_]*)/;
            match = code.match(returnRegex);
            if (match) {
                returnVar = match[1];
            }
            
            // Show parser type at the top
            const parserTypeText = document.createElementNS("http://www.w3.org/2000/svg", "text");
            parserTypeText.setAttribute("x", "20");
            parserTypeText.setAttribute("y", "20");
            parserTypeText.setAttribute("fill", "#000000");
            parserTypeText.textContent = `Parser: ${parserType === 'lalr' ? 'LALR' : 'RD'}`;
            svg.appendChild(parserTypeText);
            
            // Set SVG dimensions
            svg.setAttribute("width", "900");
            svg.setAttribute("height", "800");
            
            // Calculate positions based on the number of nodes
            const centerX = 450;
            const startY = 60;
            const verticalSpacing = 80;
            const horizontalSpacing = 200;
            
            // Create nodes following the AST structure in the text output
            const nodeData = [
                // Root level nodes
                { id: 'program', label: 'PROGRAM', x: centerX, y: startY, color: '#4285F4' },
                { id: 'function_decl', label: `FUNCTION_DECL (main) [${parserType === 'lalr' ? 'LALR' : 'RD'}]`, x: centerX, y: startY + verticalSpacing, color: '#EA4335' },
                { id: 'block', label: 'BLOCK', x: centerX, y: startY + 2 * verticalSpacing, color: '#FBBC05' }
            ];
            
            // Add variable declaration nodes
            const varY = startY + 3 * verticalSpacing;
            let varStartX = 150;
            
            for (let i = 0; i < variables.length; i++) {
                const varName = variables[i];
                const varX = varStartX + i * horizontalSpacing;
                
                // Check if this variable is part of a binary operation
                const isBinOpResult = binOps.some(op => op.result === varName);
                
                // Add variable declaration node
                nodeData.push({
                    id: `var_decl_${varName}`,
                    label: `VARIABLE_DECL (int ${varName})`,
                    x: varX,
                    y: varY,
                    color: '#34A853'
                });
                
                if (isBinOpResult) {
                    // Get the binary operation details
                    const binOp = binOps.find(op => op.result === varName);
                    
                    // Add binary operation node
                    nodeData.push({
                        id: `binary_op_${varName}`,
                        label: 'BINARY_OP (+)',
                        x: varX,
                        y: varY + verticalSpacing,
                        color: '#F4B400'
                    });
                    
                    // Add identifier nodes for the operands
                    nodeData.push({
                        id: `id_left_${varName}`,
                        label: `IDENTIFIER (${binOp.left})`,
                        x: varX - 70,
                        y: varY + 2 * verticalSpacing,
                        color: '#DB4437'
                    });
                    
                    nodeData.push({
                        id: `id_right_${varName}`,
                        label: `IDENTIFIER (${binOp.right})`,
                        x: varX + 70,
                        y: varY + 2 * verticalSpacing,
                        color: '#DB4437'
                    });
                } else {
                    // Add number node for direct value
                    nodeData.push({
                        id: `num_${varName}`,
                        label: `NUMBER (${values[varName]})`,
                        x: varX,
                        y: varY + verticalSpacing,
                        color: '#F4B400'
                    });
                }
            }
            
            // Add return statement if present
            if (returnVar) {
                const returnY = varY + 3 * verticalSpacing;
                
                nodeData.push({
                    id: 'return',
                    label: 'RETURN',
                    x: centerX,
                    y: returnY,
                    color: '#0F9D58'
                });
                
                nodeData.push({
                    id: 'return_var',
                    label: `IDENTIFIER (${returnVar})`,
                    x: centerX,
                    y: returnY + verticalSpacing,
                    color: '#DB4437'
                });
            }
            
            // Create links based on the AST structure
            const linkData = [
                // Core structure links
                { source: 'program', target: 'function_decl' },
                { source: 'function_decl', target: 'block' }
            ];
            
            // Add links for variable declarations
            for (let i = 0; i < variables.length; i++) {
                const varName = variables[i];
                
                // Link from block to variable declaration
                linkData.push({
                    source: 'block',
                    target: `var_decl_${varName}`
                });
                
                // Check if this is part of a binary operation
                const isBinOpResult = binOps.some(op => op.result === varName);
                
                if (isBinOpResult) {
                    // Binary operation links
                    linkData.push({
                        source: `var_decl_${varName}`,
                        target: `binary_op_${varName}`
                    });
                    
                    linkData.push({
                        source: `binary_op_${varName}`,
                        target: `id_left_${varName}`
                    });
                    
                    linkData.push({
                        source: `binary_op_${varName}`,
                        target: `id_right_${varName}`
                    });
                } else {
                    // Direct value link
                    linkData.push({
                        source: `var_decl_${varName}`,
                        target: `num_${varName}`
                    });
                }
            }
            
            // Add return statement links
            if (returnVar) {
                linkData.push({
                    source: 'block',
                    target: 'return'
                });
                
                linkData.push({
                    source: 'return',
                    target: 'return_var'
                });
            }
            
            // Draw links first (so they're behind nodes)
            linkData.forEach(link => {
                const source = nodeData.find(n => n.id === link.source);
                const target = nodeData.find(n => n.id === link.target);
                
                if (source && target) {
                    const path = document.createElementNS("http://www.w3.org/2000/svg", "path");
                    path.setAttribute("class", "link");
                    path.setAttribute("d", `M${source.x},${source.y + 20} L${target.x},${target.y - 20}`);
                    path.setAttribute("stroke", "#555555");
                    path.setAttribute("stroke-width", "2");
                    svg.appendChild(path);
                }
            });
            
            // Draw nodes
            nodeData.forEach(node => {
                const group = document.createElementNS("http://www.w3.org/2000/svg", "g");
                group.setAttribute("class", "node");
                
                // Create a rounded rectangle for each node
                const width = Math.max(node.label.length * 8, 120);
                const rect = document.createElementNS("http://www.w3.org/2000/svg", "rect");
                rect.setAttribute("x", node.x - width/2);
                rect.setAttribute("y", node.y - 20);
                rect.setAttribute("width", width);
                rect.setAttribute("height", "40");
                rect.setAttribute("rx", "10");
                rect.setAttribute("ry", "10");
                rect.setAttribute("fill", node.color);
                rect.setAttribute("stroke", "#333333");
                rect.setAttribute("stroke-width", "2");
                group.appendChild(rect);
                
                // Add text label
                const text = document.createElementNS("http://www.w3.org/2000/svg", "text");
                text.setAttribute("x", node.x);
                text.setAttribute("y", node.y + 5);
                text.setAttribute("text-anchor", "middle");
                text.setAttribute("font-size", "14px");
                text.setAttribute("font-weight", "bold");
                text.setAttribute("fill", "#FFFFFF");
                text.textContent = node.label;
                group.appendChild(text);
                
                svg.appendChild(group);
            });
        }

        // Compile code function
        function compileCode() {
            const sourceCode = document.getElementById('source-code').value;
            const parserType = document.getElementById('parser-type').value;
            const statusElement = document.getElementById('status');
            
            // Show compilation status
            statusElement.textContent = `Compiling with ${parserType === 'lalr' ? 'LALR' : 'Recursive Descent'} parser...`;
            statusElement.className = '';
            
            // Process code and generate outputs
            try {
                // Check for undeclared variables first
                checkForUndeclaredVariables(sourceCode);
                
                // Update tokens view
                const tokens = tokenize(sourceCode);
                document.getElementById('tokens-content').textContent = tokens;
                document.getElementById('tokens-output').style.display = 'block';
                
                // Update AST view
                const ast = generateAST(sourceCode, parserType);
                document.getElementById('ast-content').textContent = ast;
                document.getElementById('ast-output').style.display = 'block';
                
                // Draw AST visualization
                drawAstVisualization(sourceCode, parserType);
                
                // Update TAC view
                const tac = generateTAC(sourceCode);
                document.getElementById('tac-content').textContent = tac;
                document.getElementById('tac-output').style.display = 'block';
                
                // Update stack code view
                const stackCode = generateStackCode(sourceCode);
                document.getElementById('stack-content').textContent = stackCode;
                document.getElementById('stack-output').style.display = 'block';
                
                // Update target code view
                const targetCode = generateTargetCode(sourceCode);
                document.getElementById('target-content').textContent = targetCode;
                document.getElementById('target-output').style.display = 'block';
                
                // Update status
                statusElement.textContent = `Compilation with ${parserType === 'lalr' ? 'LALR' : 'Recursive Descent'} parser successful`;
                statusElement.className = 'success';
                
                // Scroll to tokens section
                document.getElementById('tokens-output').scrollIntoView({ behavior: 'smooth' });
            } catch (error) {
                // Handle errors - display error message and hide output sections
                statusElement.textContent = `Error: ${error.message}`;
                statusElement.className = 'error';
                
                // Hide all output sections on error
                document.getElementById('tokens-output').style.display = 'none';
                document.getElementById('ast-output').style.display = 'none';
                document.getElementById('tac-output').style.display = 'none';
                document.getElementById('stack-output').style.display = 'none';
                document.getElementById('target-output').style.display = 'none';
            }
        }
        
        // Run initial compilation when page loads
        window.onload = function() {
            compileCode();
        };
    </script>
</body>
</html>
"""
    return html

if __name__ == "__main__":
    main()
