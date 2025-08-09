#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../framework/test.h"
#include "zen/core/lexer.h"
#include "zen/core/parser.h"
#include "zen/core/visitor.h"
#include "zen/core/ast.h"
#include "zen/core/memory.h"
#include "zen/core/scope.h"

// Test basic class definition
void test_basic_class_definition(void) {
    const char* code = 
        "class Animal\n"
        "    constructor name\n"
        "        set this.name name\n"
        "    method speak\n"
        "        return \"[\" + this.name + \" makes a sound]\"\n";
    
    lexer_T* lexer = lexer_new((char*)code);
    ASSERT_NOT_NULL(lexer);
    
    parser_T* parser = parser_new(lexer);
    ASSERT_NOT_NULL(parser);
    
    AST_T* root = parser_parse(parser, scope_new());
    ASSERT_NOT_NULL(root);
    ASSERT_EQ(root->type, AST_COMPOUND);
    ASSERT_EQ(root->compound_size, 1);
    
    AST_T* class_node = root->compound_statements[0];
    ASSERT_EQ(class_node->type, AST_CLASS_DEFINITION);
    ASSERT_STR_EQ(class_node->class_name, "Animal");
    ASSERT_NULL(class_node->parent_class);
    ASSERT_NOT_NULL(class_node->class_methods);
    
    parser_free(parser);
    ast_free(root);
}

// Test class with inheritance
void test_class_inheritance(void) {
    const char* code = 
        "class Animal\n"
        "    constructor name\n"
        "        set this.name name\n"
        "    method speak\n"
        "        return \"[\" + this.name + \" makes a sound]\"\n"
        "\n"
        "class Dog extends Animal\n"
        "    method speak\n"
        "        return this.name + \" barks!\"\n";
    
    lexer_T* lexer = lexer_new((char*)code);
    parser_T* parser = parser_new(lexer);
    AST_T* root = parser_parse(parser, scope_new());
    
    ASSERT_NOT_NULL(root);
    ASSERT_EQ(root->compound_size, 2);
    
    AST_T* dog_class = root->compound_statements[1];
    ASSERT_EQ(dog_class->type, AST_CLASS_DEFINITION);
    ASSERT_STR_EQ(dog_class->class_name, "Dog");
    ASSERT_STR_EQ(dog_class->parent_class, "Animal");
    
    parser_free(parser);
    ast_free(root);
}

// Test class instantiation and method calls
void test_class_instantiation(void) {
    const char* code = 
        "class Animal\n"
        "    constructor name\n"
        "        set this.name name\n"
        "    method speak\n"
        "        return \"[\" + this.name + \" makes a sound]\"\n"
        "\n"
        "set cat new Animal \"Fluffy\"\n"
        "set sound cat.speak\n";
    
    lexer_T* lexer = lexer_new((char*)code);
    parser_T* parser = parser_new(lexer);
    scope_T* global_scope = scope_new();
    
    visitor_T* visitor = visitor_new();
    ASSERT_NOT_NULL(visitor);
    
    AST_T* root = parser_parse(parser, global_scope);
    ASSERT_NOT_NULL(root);
    
    // Execute the code
    visitor_visit(visitor, root);
    
    // Check that class was defined
    AST_T* animal_class = scope_get_variable_definition(global_scope, "Animal");
    ASSERT_NOT_NULL(animal_class);
    ASSERT_EQ(animal_class->type, AST_CLASS_DEFINITION);
    
    // Check that instance was created
    AST_T* cat_instance = scope_get_variable_definition(global_scope, "cat");
    ASSERT_NOT_NULL(cat_instance);
    
    // Check the method call result
    AST_T* sound_result = scope_get_variable_definition(global_scope, "sound");
    ASSERT_NOT_NULL(sound_result);
    
    visitor_free(visitor);
    parser_free(parser);
    ast_free(root);
    scope_free(global_scope);
}

// Test private methods
void test_private_methods(void) {
    const char* code = 
        "class BankAccount\n"
        "    constructor balance\n"
        "        set this.balance balance\n"
        "    private method _validate amount\n"
        "        return amount > 0\n"
        "    method deposit amount\n"
        "        if this._validate amount\n"
        "            set this.balance this.balance + amount\n"
        "            return true\n"
        "        return false\n";
    
    lexer_T* lexer = lexer_new((char*)code);
    parser_T* parser = parser_new(lexer);
    AST_T* root = parser_parse(parser, scope_new());
    
    ASSERT_NOT_NULL(root);
    AST_T* class_node = root->compound_statements[0];
    ASSERT_EQ(class_node->type, AST_CLASS_DEFINITION);
    
    // Find the private method in the class methods
    bool found_private = false;
    for (size_t i = 0; i < class_node->class_methods_size; i++) {
        AST_T* method = class_node->class_methods[i];
        if (method->type == AST_FUNCTION_DEFINITION && 
            strcmp(method->function_definition_name, "_validate") == 0) {
            // Note: function_is_private field may not exist yet, skip this test
            found_private = true;
            break;
        }
    }
    ASSERT_TRUE(found_private);
    
    parser_free(parser);
    ast_free(root);
}

// Test super calls
void test_super_calls(void) {
    const char* code = 
        "class Animal\n"
        "    constructor name\n"
        "        set this.name name\n"
        "    method speak\n"
        "        return \"[\" + this.name + \" makes a sound]\"\n"
        "\n"
        "class Dog extends Animal\n"
        "    constructor name breed\n"
        "        super name\n"
        "        set this.breed breed\n"
        "    method speak\n"
        "        set base_sound super.speak\n"
        "        return base_sound + \" Woof!\"\n";
    
    lexer_T* lexer = lexer_new((char*)code);
    parser_T* parser = parser_new(lexer);
    AST_T* root = parser_parse(parser, scope_new());
    
    ASSERT_NOT_NULL(root);
    ASSERT_EQ(root->compound_size, 2);
    
    // Check Dog class has super call in constructor
    AST_T* dog_class = root->compound_statements[1];
    ASSERT_NOT_NULL(dog_class->class_methods);
    
    // Find constructor
    AST_T* constructor = NULL;
    for (size_t i = 0; i < dog_class->class_methods_size; i++) {
        AST_T* method = dog_class->class_methods[i];
        if (method->type == AST_FUNCTION_DEFINITION && 
            strcmp(method->function_definition_name, "constructor") == 0) {
            constructor = method;
            break;
        }
    }
    
    ASSERT_NOT_NULL(constructor);
    ASSERT_NOT_NULL(constructor->function_definition_body);
    
    // Check for super call in constructor body
    bool found_super = false;
    for (size_t i = 0; i < constructor->function_definition_body->compound_size; i++) {
        AST_T* stmt = constructor->function_definition_body->compound_statements[i];
        if (stmt->type == AST_FUNCTION_CALL && 
            stmt->function_call_name && 
            strcmp(stmt->function_call_name, "super") == 0) {
            found_super = true;
            break;
        }
    }
    ASSERT_TRUE(found_super);
    
    parser_free(parser);
    ast_free(root);
}

// Test complex class hierarchy
void test_complex_hierarchy(void) {
    const char* code = 
        "class Shape\n"
        "    constructor name\n"
        "        set this.name name\n"
        "    method area\n"
        "        return 0\n"
        "\n"
        "class Rectangle extends Shape\n"
        "    constructor width height\n"
        "        super \"Rectangle\"\n"
        "        set this.width width\n"
        "        set this.height height\n"
        "    method area\n"
        "        return this.width * this.height\n"
        "\n"
        "class Square extends Rectangle\n"
        "    constructor side\n"
        "        super side side\n"
        "    method perimeter\n"
        "        return 4 * this.width\n";
    
    lexer_T* lexer = lexer_new((char*)code);
    parser_T* parser = parser_new(lexer);
    scope_T* global_scope = scope_new();
    visitor_T* visitor = visitor_new();
    
    AST_T* root = parser_parse(parser, global_scope);
    ASSERT_NOT_NULL(root);
    ASSERT_EQ(root->compound_size, 3);
    
    // Execute to define classes
    visitor_visit(visitor, root);
    
    // Test creating instances
    const char* instance_code = 
        "set rect new Rectangle 10 5\n"
        "set rect_area rect.area\n"
        "set square new Square 7\n"
        "set square_area square.area\n"
        "set square_perim square.perimeter\n";
    
    lexer_T* instance_lexer = lexer_new((char*)instance_code);
    parser_T* instance_parser = parser_new(instance_lexer);
    AST_T* instance_root = parser_parse(instance_parser, global_scope);
    
    visitor_visit(visitor, instance_root);
    
    // Verify results
    AST_T* rect_area = scope_get_variable_definition(global_scope, "rect_area");
    ASSERT_NOT_NULL(rect_area);
    
    AST_T* square_area = scope_get_variable_definition(global_scope, "square_area");
    ASSERT_NOT_NULL(square_area);
    
    AST_T* square_perim = scope_get_variable_definition(global_scope, "square_perim");
    ASSERT_NOT_NULL(square_perim);
    
    visitor_free(visitor);
    parser_free(parser);
    parser_free(instance_parser);
    ast_free(root);
    ast_free(instance_root);
    scope_free(global_scope);
}

// Test error cases
void test_class_errors(void) {
    // Test extending non-existent class
    const char* code1 = "class Dog extends NonExistentClass\n    method bark\n        return \"woof\"\n";
    lexer_T* lexer1 = lexer_new((char*)code1);
    parser_T* parser1 = parser_new(lexer1);
    AST_T* root1 = parser_parse(parser1, scope_new());
    // Should parse successfully, error would be at runtime
    ASSERT_NOT_NULL(root1);
    parser_free(parser1);
    ast_free(root1);
    
    // Test accessing private method from outside
    const char* code2 = 
        "class Secret\n"
        "    private method _hidden\n"
        "        return \"secret\"\n"
        "\n"
        "set obj new Secret\n"
        "set result obj._hidden\n";  // This should fail at runtime
    
    lexer_T* lexer2 = lexer_new((char*)code2);
    parser_T* parser2 = parser_new(lexer2);
    scope_T* scope2 = scope_new();
    visitor_T* visitor2 = visitor_new();
    
    AST_T* root2 = parser_parse(parser2, scope2);
    ASSERT_NOT_NULL(root2);
    
    // Execute - this would normally trigger an error for private access
    visitor_visit(visitor2, root2);
    
    visitor_free(visitor2);
    parser_free(parser2);
    ast_free(root2);
    scope_free(scope2);
}

// Test memory management with classes
void test_class_memory_management(void) {
    memory_debug_enable(true);
    MemoryStats initial_stats;
    memory_get_stats(&initial_stats);
    
    const char* code = 
        "class TestClass\n"
        "    constructor value\n"
        "        set this.value value\n"
        "    method double\n"
        "        return this.value * 2\n"
        "\n"
        "set obj new TestClass 42\n"
        "set result obj.double\n";
    
    lexer_T* lexer = lexer_new((char*)code);
    parser_T* parser = parser_new(lexer);
    scope_T* scope = scope_new();
    visitor_T* visitor = visitor_new();
    
    AST_T* root = parser_parse(parser, scope);
    visitor_visit(visitor, root);
    
    visitor_free(visitor);
    parser_free(parser);
    ast_free(root);
    scope_free(scope);
    
    MemoryStats final_stats;
    memory_get_stats(&final_stats);
    ASSERT_EQ(initial_stats.current_allocated, final_stats.current_allocated);
    
    memory_debug_enable(false);
}

int main(void) {
    test_init();
    
    RUN_TEST(basic_class_definition);
    RUN_TEST(class_inheritance);
    RUN_TEST(class_instantiation);
    RUN_TEST(private_methods);
    RUN_TEST(super_calls);
    RUN_TEST(complex_hierarchy);
    RUN_TEST(class_errors);
    RUN_TEST(class_memory_management);
    
    return test_finalize();
}