#!/bin/bash
# MANIFEST CRISIS FIX: Automated Doxygen Documentation Generator
# Architect: swarm-3-zen-architect
# Task: 20250806-1225.yaml

set -e

echo "🔧 EMERGENCY MANIFEST FIX: Adding missing doxygen documentation"

# Function to add doxygen to a specific function
fix_function_doxygen() {
    local file="$1"
    local func_name="$2"
    local description="$3"
    local signature="$4"
    local return_type="$5"
    
    # Skip if function already has doxygen
    if grep -A5 -B5 "$func_name" "$file" | grep -q "@brief"; then
        echo "  ✅ $func_name already has doxygen"
        return
    fi
    
    echo "  📝 Adding doxygen for $func_name"
    
    # Find the function definition line
    local line_num=$(grep -n "^$signature" "$file" | head -1 | cut -d: -f1)
    if [ -z "$line_num" ]; then
        # Try alternative signature patterns
        line_num=$(grep -n "$func_name(" "$file" | head -1 | cut -d: -f1)
    fi
    
    if [ -z "$line_num" ]; then
        echo "  ⚠️  Could not find function $func_name in $file"
        return
    fi
    
    # Generate doxygen block
    local doxygen_block="/**\n * @brief $description"
    
    # Add parameter documentation by parsing signature
    if echo "$signature" | grep -q "void)"; then
        # No parameters
        :
    else
        # Parse parameters from signature
        local params=$(echo "$signature" | sed 's/.*(//' | sed 's/).*//' | tr ',' '\n')
        while IFS= read -r param; do
            param=$(echo "$param" | xargs)  # trim whitespace
            if [ -n "$param" ] && [ "$param" != "void" ]; then
                local param_name=$(echo "$param" | awk '{print $NF}' | sed 's/*//g')
                local param_desc=""
                
                # Infer parameter description based on common patterns
                case "$param_name" in
                    "value"|"*value") param_desc="The input value to process" ;;
                    "args"|"**args") param_desc="Array of argument values" ;;
                    "argc") param_desc="Number of arguments" ;;
                    "str"|"*str") param_desc="Input string to process" ;;
                    "filename"|"filepath") param_desc="Path to the file" ;;
                    "enable") param_desc="Whether to enable the feature" ;;
                    "lexer"|"*lexer") param_desc="The lexer instance" ;;
                    "parser"|"*parser") param_desc="The parser instance" ;;
                    "visitor"|"*visitor") param_desc="The visitor instance" ;;
                    "node"|"*node") param_desc="The AST node to process" ;;
                    "scope"|"*scope") param_desc="The scope context" ;;
                    "token"|"*token") param_desc="The token to process" ;;
                    "ast"|"*ast") param_desc="The AST node" ;;
                    "error"|"*error") param_desc="The error value" ;;
                    "size"|"count"|"length") param_desc="The size or count" ;;
                    "*") param_desc="Parameter description" ;;
                esac
                
                doxygen_block+="\n * @param $param_name $param_desc"
            fi
        done <<< "$params"
    fi
    
    # Add return documentation if function returns something
    if [ -n "$return_type" ] && [ "$return_type" != "void" ]; then
        local return_desc=""
        case "$return_type" in
            "Value*") return_desc="The result value" ;;
            "bool") return_desc="True on success, false on failure" ;;
            "char*") return_desc="The resulting string" ;;
            "size_t") return_desc="The size or count" ;;
            "int") return_desc="The result code or value" ;;
            "*") return_desc="The result" ;;
        esac
        doxygen_block+="\n * @return $return_desc"
    fi
    
    doxygen_block+="\n */"
    
    # Insert doxygen block before function definition
    sed -i "${line_num}i\\$(echo -e "$doxygen_block")" "$file"
    
    echo "  ✅ Added doxygen for $func_name"
}

# Main functions to fix
echo "🎯 Fixing critical doxygen violations..."

# Fix init_scope in scope.c
if [ -f "src/core/scope.c" ]; then
    echo "📁 Processing src/core/scope.c"
    fix_function_doxygen "src/core/scope.c" "init_scope" "Create new scope instance" "scope_T* init_scope()" "scope_T*"
fi

# Fix init_visitor in visitor.c  
if [ -f "src/core/visitor.c" ]; then
    echo "📁 Processing src/core/visitor.c"
    fix_function_doxygen "src/core/visitor.c" "init_visitor" "Create new visitor instance" "visitor_T* init_visitor()" "visitor_T*"
fi

# Fix print_help in main.c
if [ -f "src/main.c" ]; then
    echo "📁 Processing src/main.c"
    fix_function_doxygen "src/main.c" "print_help" "Print help information" "void print_help()" "void"
fi

# Fix zen_math_random in math.c
if [ -f "src/stdlib/math.c" ]; then
    echo "📁 Processing src/stdlib/math.c"
    fix_function_doxygen "src/stdlib/math.c" "zen_math_random" "Random number generator" "Value* zen_math_random()" "Value*"
fi

echo ""
echo "✅ Doxygen fixes applied!"
echo "🔍 Run 'make enforce' to check remaining violations"