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
    TEST_ASSERT_NOT_NULL(lexer, "Lexer creation failed");
    
    parser_T* parser = parser_new(lexer);
    TEST_ASSERT_NOT_NULL(parser, "Parser creation failed");
    
    AST_T* root = parser_parse(parser, scope_new());
    TEST_ASSERT_NOT_NULL(root, "Parsing failed");
    TEST_ASSERT_EQUAL(root->type, AST_COMPOUND, "Expected compound root");
    TEST_ASSERT_EQUAL(root->compound_size, 1, "Expected one statement");
    
    AST_T* class_node = root->compound_value[0];
    TEST_ASSERT_EQUAL(class_node->type, AST_CLASS, "Expected class node");
    TEST_ASSERT_STRING_EQUAL(class_node->class_name, "Animal", "Wrong class name");
    TEST_ASSERT_NULL(class_node->class_parent, "Should have no parent");
    TEST_ASSERT_NOT_NULL(class_node->class_body, "Should have body");
    
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
    
    TEST_ASSERT_NOT_NULL(root, "Parsing failed");
    TEST_ASSERT_EQUAL(root->compound_size, 2, "Expected two classes");
    
    AST_T* dog_class = root->compound_value[1];
    TEST_ASSERT_EQUAL(dog_class->type, AST_CLASS, "Expected class node");
    TEST_ASSERT_STRING_EQUAL(dog_class->class_name, "Dog", "Wrong class name");
    TEST_ASSERT_STRING_EQUAL(dog_class->class_parent, "Animal", "Wrong parent class");
    
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
    TEST_ASSERT_NOT_NULL(visitor, "Visitor creation failed");
    
    AST_T* root = parser_parse(parser, global_scope);
    TEST_ASSERT_NOT_NULL(root, "Parsing failed");
    
    // Execute the code
    visitor_visit(visitor, root);
    
    // Check that class was defined
    AST_T* animal_class = scope_get_variable(global_scope, "Animal");
    TEST_ASSERT_NOT_NULL(animal_class, "Class Animal not defined");
    TEST_ASSERT_EQUAL(animal_class->type, AST_CLASS, "Animal should be a class");
    
    // Check that instance was created
    AST_T* cat_instance = scope_get_variable(global_scope, "cat");
    TEST_ASSERT_NOT_NULL(cat_instance, "Instance cat not created");
    
    // Check the method call result
    AST_T* sound_result = scope_get_variable(global_scope, "sound");
    TEST_ASSERT_NOT_NULL(sound_result, "Method result not stored");
    
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
    
    TEST_ASSERT_NOT_NULL(root, "Parsing failed");
    AST_T* class_node = root->compound_value[0];
    TEST_ASSERT_EQUAL(class_node->type, AST_CLASS, "Expected class node");
    
    // Find the private method in the class body
    bool found_private = false;
    for (size_t i = 0; i < class_node->class_body->compound_size; i++) {
        AST_T* stmt = class_node->class_body->compound_value[i];
        if (stmt->type == AST_FUNCTION_DEFINITION && 
            strcmp(stmt->function_definition_name, "_validate") == 0) {
            TEST_ASSERT_TRUE(stmt->function_is_private, "Method should be private");
            found_private = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(found_private, "Private method not found");
    
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
    
    TEST_ASSERT_NOT_NULL(root, "Parsing failed");
    TEST_ASSERT_EQUAL(root->compound_size, 2, "Expected two classes");
    
    // Check Dog class has super call in constructor
    AST_T* dog_class = root->compound_value[1];
    TEST_ASSERT_NOT_NULL(dog_class->class_body, "Dog should have body");
    
    // Find constructor
    AST_T* constructor = NULL;
    for (size_t i = 0; i < dog_class->class_body->compound_size; i++) {
        AST_T* stmt = dog_class->class_body->compound_value[i];
        if (stmt->type == AST_FUNCTION_DEFINITION && 
            strcmp(stmt->function_definition_name, "constructor") == 0) {
            constructor = stmt;
            break;
        }
    }
    
    TEST_ASSERT_NOT_NULL(constructor, "Constructor not found");
    TEST_ASSERT_NOT_NULL(constructor->function_definition_body, "Constructor should have body");
    
    // Check for super call in constructor body
    bool found_super = false;
    for (size_t i = 0; i < constructor->function_definition_body->compound_size; i++) {
        AST_T* stmt = constructor->function_definition_body->compound_value[i];
        if (stmt->type == AST_FUNCTION_CALL && 
            stmt->function_call_name && 
            strcmp(stmt->function_call_name, "super") == 0) {
            found_super = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(found_super, "Super call not found in constructor");
    
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
    TEST_ASSERT_NOT_NULL(root, "Parsing failed");
    TEST_ASSERT_EQUAL(root->compound_size, 3, "Expected three classes");
    
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
    AST_T* rect_area = scope_get_variable(global_scope, "rect_area");
    TEST_ASSERT_NOT_NULL(rect_area, "Rectangle area not calculated");
    
    AST_T* square_area = scope_get_variable(global_scope, "square_area");
    TEST_ASSERT_NOT_NULL(square_area, "Square area not calculated");
    
    AST_T* square_perim = scope_get_variable(global_scope, "square_perim");
    TEST_ASSERT_NOT_NULL(square_perim, "Square perimeter not calculated");
    
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
    TEST_ASSERT_NOT_NULL(root1, "Should parse even with unknown parent");
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
    TEST_ASSERT_NOT_NULL(root2, "Should parse private method access");
    
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
    size_t initial_allocations = memory_get_allocation_count();
    
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
    
    size_t final_allocations = memory_get_allocation_count();
    TEST_ASSERT_EQUAL(initial_allocations, final_allocations, 
                      "Memory leak detected in class operations");
    
    memory_debug_enable(false);
}

int main(void) {
    TEST_INIT();
    
    TEST_RUN(test_basic_class_definition);
    TEST_RUN(test_class_inheritance);
    TEST_RUN(test_class_instantiation);
    TEST_RUN(test_private_methods);
    TEST_RUN(test_super_calls);
    TEST_RUN(test_complex_hierarchy);
    TEST_RUN(test_class_errors);
    TEST_RUN(test_class_memory_management);
    
    TEST_SUMMARY();
    return TEST_EXIT_CODE();
}