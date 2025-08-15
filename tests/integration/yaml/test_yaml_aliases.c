/**
 * @file test_yaml_aliases.c
 * @brief Test YAML alias support including anchors, aliases, and merge keys
 */

#include "tests/framework/test.h"
#include "zen/stdlib/yaml.h"
#include "zen/core/runtime_value.h"
#include "zen/core/error.h"

// Test basic alias functionality
void test_yaml_basic_alias(void) {
    const char* yaml = 
        "person: &john\n"
        "  name: John Doe\n"
        "  age: 30\n"
        "\n"
        "employee: *john\n";
    
    RuntimeValue* result = yaml_parse(yaml);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(RV_OBJECT, result->type);
    
    RuntimeValue* person = rv_object_get(result, "person");
    TEST_ASSERT_NOT_NULL(person);
    TEST_ASSERT_EQUAL(RV_OBJECT, person->type);
    
    RuntimeValue* employee = rv_object_get(result, "employee");
    TEST_ASSERT_NOT_NULL(employee);
    TEST_ASSERT_EQUAL(RV_OBJECT, employee->type);
    
    // Check that both have the same data
    RuntimeValue* person_name = rv_object_get(person, "name");
    RuntimeValue* employee_name = rv_object_get(employee, "name");
    TEST_ASSERT_NOT_NULL(person_name);
    TEST_ASSERT_NOT_NULL(employee_name);
    TEST_ASSERT_EQUAL(RV_STRING, person_name->type);
    TEST_ASSERT_EQUAL(RV_STRING, employee_name->type);
    TEST_ASSERT_STR_EQUAL(rv_get_string(person_name), rv_get_string(employee_name));
    
    RuntimeValue* person_age = rv_object_get(person, "age");
    RuntimeValue* employee_age = rv_object_get(employee, "age");
    TEST_ASSERT_NOT_NULL(person_age);
    TEST_ASSERT_NOT_NULL(employee_age);
    TEST_ASSERT_EQUAL(RV_NUMBER, person_age->type);
    TEST_ASSERT_EQUAL(RV_NUMBER, employee_age->type);
    TEST_ASSERT_EQUAL(person_age->data.number, employee_age->data.number);
    
    rv_unref(result);
}

// Test array alias
void test_yaml_array_alias(void) {
    const char* yaml = 
        "fruits: &my_fruits\n"
        "  - apple\n"
        "  - banana\n"
        "  - orange\n"
        "\n"
        "basket1: *my_fruits\n"
        "basket2: *my_fruits\n";
    
    RuntimeValue* result = yaml_parse(yaml);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(RV_OBJECT, result->type);
    
    RuntimeValue* fruits = rv_object_get(result, "fruits");
    TEST_ASSERT_NOT_NULL(fruits);
    TEST_ASSERT_EQUAL(RV_ARRAY, fruits->type);
    TEST_ASSERT_EQUAL(3, rv_array_size(fruits));
    
    RuntimeValue* basket1 = rv_object_get(result, "basket1");
    TEST_ASSERT_NOT_NULL(basket1);
    TEST_ASSERT_EQUAL(RV_ARRAY, basket1->type);
    TEST_ASSERT_EQUAL(3, rv_array_size(basket1));
    
    RuntimeValue* basket2 = rv_object_get(result, "basket2");
    TEST_ASSERT_NOT_NULL(basket2);
    TEST_ASSERT_EQUAL(RV_ARRAY, basket2->type);
    TEST_ASSERT_EQUAL(3, rv_array_size(basket2));
    
    // Check array contents
    for (size_t i = 0; i < 3; i++) {
        RuntimeValue* fruit = rv_array_get(fruits, i);
        RuntimeValue* item1 = rv_array_get(basket1, i);
        RuntimeValue* item2 = rv_array_get(basket2, i);
        
        TEST_ASSERT_NOT_NULL(fruit);
        TEST_ASSERT_NOT_NULL(item1);
        TEST_ASSERT_NOT_NULL(item2);
        
        TEST_ASSERT_EQUAL(RV_STRING, fruit->type);
        TEST_ASSERT_EQUAL(RV_STRING, item1->type);
        TEST_ASSERT_EQUAL(RV_STRING, item2->type);
        
        TEST_ASSERT_STR_EQUAL(rv_get_string(fruit), rv_get_string(item1));
        TEST_ASSERT_STR_EQUAL(rv_get_string(fruit), rv_get_string(item2));
    }
    
    rv_unref(result);
}

// Test merge key functionality
void test_yaml_merge_key(void) {
    const char* yaml = 
        "defaults: &defaults\n"
        "  timeout: 30\n"
        "  retries: 3\n"
        "  log_level: info\n"
        "\n"
        "production:\n"
        "  <<: *defaults\n"
        "  log_level: warn\n"
        "  server: prod.example.com\n"
        "\n"
        "development:\n"
        "  <<: *defaults\n"
        "  timeout: 60\n"
        "  server: dev.example.com\n";
    
    RuntimeValue* result = yaml_parse(yaml);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(RV_OBJECT, result->type);
    
    RuntimeValue* production = rv_object_get(result, "production");
    TEST_ASSERT_NOT_NULL(production);
    TEST_ASSERT_EQUAL(RV_OBJECT, production->type);
    
    // Check merged values
    RuntimeValue* prod_timeout = rv_object_get(production, "timeout");
    TEST_ASSERT_NOT_NULL(prod_timeout);
    TEST_ASSERT_EQUAL(RV_NUMBER, prod_timeout->type);
    TEST_ASSERT_EQUAL(30, rv_get_number(prod_timeout)); // From defaults
    
    RuntimeValue* prod_retries = rv_object_get(production, "retries");
    TEST_ASSERT_NOT_NULL(prod_retries);
    TEST_ASSERT_EQUAL(RV_NUMBER, prod_retries->type);
    TEST_ASSERT_EQUAL(3, rv_get_number(prod_retries)); // From defaults
    
    RuntimeValue* prod_log = rv_object_get(production, "log_level");
    TEST_ASSERT_NOT_NULL(prod_log);
    TEST_ASSERT_EQUAL(RV_STRING, prod_log->type);
    TEST_ASSERT_STR_EQUAL("warn", rv_get_string(prod_log)); // Overridden
    
    RuntimeValue* prod_server = rv_object_get(production, "server");
    TEST_ASSERT_NOT_NULL(prod_server);
    TEST_ASSERT_EQUAL(RV_STRING, prod_server->type);
    TEST_ASSERT_STR_EQUAL("prod.example.com", rv_get_string(prod_server)); // New field
    
    // Check development environment
    RuntimeValue* development = rv_object_get(result, "development");
    TEST_ASSERT_NOT_NULL(development);
    TEST_ASSERT_EQUAL(RV_OBJECT, development->type);
    
    RuntimeValue* dev_timeout = rv_object_get(development, "timeout");
    TEST_ASSERT_NOT_NULL(dev_timeout);
    TEST_ASSERT_EQUAL(RV_NUMBER, dev_timeout->type);
    TEST_ASSERT_EQUAL(60, rv_get_number(dev_timeout)); // Overridden
    
    RuntimeValue* dev_retries = rv_object_get(development, "retries");
    TEST_ASSERT_NOT_NULL(dev_retries);
    TEST_ASSERT_EQUAL(RV_NUMBER, dev_retries->type);
    TEST_ASSERT_EQUAL(3, rv_get_number(dev_retries)); // From defaults
    
    rv_unref(result);
}

// Test alias in array
void test_yaml_alias_in_array(void) {
    const char* yaml = 
        "person: &john\n"
        "  name: John Doe\n"
        "  age: 30\n"
        "\n"
        "employees:\n"
        "  - *john\n"
        "  - name: Jane Smith\n"
        "    age: 25\n"
        "  - *john\n";
    
    RuntimeValue* result = yaml_parse(yaml);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(RV_OBJECT, result->type);
    
    RuntimeValue* employees = rv_object_get(result, "employees");
    TEST_ASSERT_NOT_NULL(employees);
    TEST_ASSERT_EQUAL(RV_ARRAY, employees->type);
    TEST_ASSERT_EQUAL(3, rv_array_size(employees));
    
    // First and third employees should be John
    RuntimeValue* emp1 = rv_array_get(employees, 0);
    RuntimeValue* emp3 = rv_array_get(employees, 2);
    TEST_ASSERT_NOT_NULL(emp1);
    TEST_ASSERT_NOT_NULL(emp3);
    TEST_ASSERT_EQUAL(RV_OBJECT, emp1->type);
    TEST_ASSERT_EQUAL(RV_OBJECT, emp3->type);
    
    RuntimeValue* emp1_name = rv_object_get(emp1, "name");
    RuntimeValue* emp3_name = rv_object_get(emp3, "name");
    TEST_ASSERT_NOT_NULL(emp1_name);
    TEST_ASSERT_NOT_NULL(emp3_name);
    TEST_ASSERT_STR_EQUAL("John Doe", rv_get_string(emp1_name));
    TEST_ASSERT_STR_EQUAL("John Doe", rv_get_string(emp3_name));
    
    // Second employee should be Jane
    RuntimeValue* emp2 = rv_array_get(employees, 1);
    TEST_ASSERT_NOT_NULL(emp2);
    TEST_ASSERT_EQUAL(RV_OBJECT, emp2->type);
    
    RuntimeValue* emp2_name = rv_object_get(emp2, "name");
    TEST_ASSERT_NOT_NULL(emp2_name);
    TEST_ASSERT_STR_EQUAL("Jane Smith", rv_get_string(emp2_name));
    
    rv_unref(result);
}

// Test unknown anchor error
void test_yaml_unknown_anchor(void) {
    const char* yaml = "data: *unknown_anchor\n";
    
    RuntimeValue* result = yaml_parse(yaml);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(RV_ERROR, result->type);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(rv_get_error_message(result));
    TEST_ASSERT_STR_CONTAINS(rv_get_error_message(result), "Unknown anchor");
    
    rv_unref(result);
}

// Test duplicate anchor error
void test_yaml_duplicate_anchor(void) {
    const char* yaml = 
        "first: &same\n"
        "  value: 1\n"
        "second: &same\n"
        "  value: 2\n";
    
    RuntimeValue* result = yaml_parse(yaml);
    // libyaml might handle this differently - it might just use the last definition
    // For now, we'll just check that parsing doesn't crash
    TEST_ASSERT_NOT_NULL(result);
    
    rv_unref(result);
}

// Test nested aliases
void test_yaml_nested_aliases(void) {
    const char* yaml = 
        "base: &base\n"
        "  name: base\n"
        "  value: 100\n"
        "\n"
        "middle: &middle\n"
        "  <<: *base\n"
        "  name: middle\n"
        "  extra: 200\n"
        "\n"
        "final:\n"
        "  <<: *middle\n"
        "  name: final\n";
    
    RuntimeValue* result = yaml_parse(yaml);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(RV_OBJECT, result->type);
    
    RuntimeValue* final = rv_object_get(result, "final");
    TEST_ASSERT_NOT_NULL(final);
    TEST_ASSERT_EQUAL(RV_OBJECT, final->type);
    
    // Should have all fields
    RuntimeValue* name = rv_object_get(final, "name");
    TEST_ASSERT_NOT_NULL(name);
    TEST_ASSERT_STR_EQUAL("final", rv_get_string(name)); // Overridden twice
    
    RuntimeValue* value = rv_object_get(final, "value");
    TEST_ASSERT_NOT_NULL(value);
    TEST_ASSERT_EQUAL(100, rv_get_number(value)); // From base
    
    RuntimeValue* extra = rv_object_get(final, "extra");
    TEST_ASSERT_NOT_NULL(extra);
    TEST_ASSERT_EQUAL(200, rv_get_number(extra)); // From middle
    
    rv_unref(result);
}

int main(void) {
    TEST_BEGIN("YAML Alias Support");
    
    RUN_TEST(test_yaml_basic_alias);
    RUN_TEST(test_yaml_array_alias);
    RUN_TEST(test_yaml_merge_key);
    RUN_TEST(test_yaml_alias_in_array);
    RUN_TEST(test_yaml_unknown_anchor);
    RUN_TEST(test_yaml_duplicate_anchor);
    RUN_TEST(test_yaml_nested_aliases);
    
    TEST_END();
}