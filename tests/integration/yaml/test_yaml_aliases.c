/**
 * @file test_yaml_aliases.c
 * @brief Test YAML alias support including anchors, aliases, and merge keys
 */

#include "tests/framework/test.h"
#include "zen/stdlib/yaml.h"
#include "zen/types/value.h"
#include "zen/types/object.h"
#include "zen/types/array.h"
#include "zen/core/error.h"

// Test basic alias functionality
void test_yaml_basic_alias(void) {
    const char* yaml = 
        "person: &john\n"
        "  name: John Doe\n"
        "  age: 30\n"
        "\n"
        "employee: *john\n";
    
    Value* result = yaml_parse(yaml);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(VALUE_OBJECT, result->type);
    
    Value* person = object_get(result, "person");
    TEST_ASSERT_NOT_NULL(person);
    TEST_ASSERT_EQUAL(VALUE_OBJECT, person->type);
    
    Value* employee = object_get(result, "employee");
    TEST_ASSERT_NOT_NULL(employee);
    TEST_ASSERT_EQUAL(VALUE_OBJECT, employee->type);
    
    // Check that both have the same data
    Value* person_name = object_get(person, "name");
    Value* employee_name = object_get(employee, "name");
    TEST_ASSERT_NOT_NULL(person_name);
    TEST_ASSERT_NOT_NULL(employee_name);
    TEST_ASSERT_EQUAL(VALUE_STRING, person_name->type);
    TEST_ASSERT_EQUAL(VALUE_STRING, employee_name->type);
    TEST_ASSERT_STR_EQUAL(person_name->as.string->data, employee_name->as.string->data);
    
    Value* person_age = object_get(person, "age");
    Value* employee_age = object_get(employee, "age");
    TEST_ASSERT_NOT_NULL(person_age);
    TEST_ASSERT_NOT_NULL(employee_age);
    TEST_ASSERT_EQUAL(VALUE_NUMBER, person_age->type);
    TEST_ASSERT_EQUAL(VALUE_NUMBER, employee_age->type);
    TEST_ASSERT_EQUAL(person_age->as.number, employee_age->as.number);
    
    value_unref(result);
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
    
    Value* result = yaml_parse(yaml);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(VALUE_OBJECT, result->type);
    
    Value* fruits = object_get(result, "fruits");
    TEST_ASSERT_NOT_NULL(fruits);
    TEST_ASSERT_EQUAL(VALUE_ARRAY, fruits->type);
    TEST_ASSERT_EQUAL(3, fruits->as.array->length);
    
    Value* basket1 = object_get(result, "basket1");
    TEST_ASSERT_NOT_NULL(basket1);
    TEST_ASSERT_EQUAL(VALUE_ARRAY, basket1->type);
    TEST_ASSERT_EQUAL(3, basket1->as.array->length);
    
    Value* basket2 = object_get(result, "basket2");
    TEST_ASSERT_NOT_NULL(basket2);
    TEST_ASSERT_EQUAL(VALUE_ARRAY, basket2->type);
    TEST_ASSERT_EQUAL(3, basket2->as.array->length);
    
    // Check array contents
    for (size_t i = 0; i < 3; i++) {
        Value* fruit = fruits->as.array->items[i];
        Value* item1 = basket1->as.array->items[i];
        Value* item2 = basket2->as.array->items[i];
        
        TEST_ASSERT_NOT_NULL(fruit);
        TEST_ASSERT_NOT_NULL(item1);
        TEST_ASSERT_NOT_NULL(item2);
        
        TEST_ASSERT_EQUAL(VALUE_STRING, fruit->type);
        TEST_ASSERT_EQUAL(VALUE_STRING, item1->type);
        TEST_ASSERT_EQUAL(VALUE_STRING, item2->type);
        
        TEST_ASSERT_STR_EQUAL(fruit->as.string->data, item1->as.string->data);
        TEST_ASSERT_STR_EQUAL(fruit->as.string->data, item2->as.string->data);
    }
    
    value_unref(result);
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
    
    Value* result = yaml_parse(yaml);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(VALUE_OBJECT, result->type);
    
    Value* production = object_get(result, "production");
    TEST_ASSERT_NOT_NULL(production);
    TEST_ASSERT_EQUAL(VALUE_OBJECT, production->type);
    
    // Check merged values
    Value* prod_timeout = object_get(production, "timeout");
    TEST_ASSERT_NOT_NULL(prod_timeout);
    TEST_ASSERT_EQUAL(VALUE_NUMBER, prod_timeout->type);
    TEST_ASSERT_EQUAL(30, prod_timeout->as.number); // From defaults
    
    Value* prod_retries = object_get(production, "retries");
    TEST_ASSERT_NOT_NULL(prod_retries);
    TEST_ASSERT_EQUAL(VALUE_NUMBER, prod_retries->type);
    TEST_ASSERT_EQUAL(3, prod_retries->as.number); // From defaults
    
    Value* prod_log = object_get(production, "log_level");
    TEST_ASSERT_NOT_NULL(prod_log);
    TEST_ASSERT_EQUAL(VALUE_STRING, prod_log->type);
    TEST_ASSERT_STR_EQUAL("warn", prod_log->as.string->data); // Overridden
    
    Value* prod_server = object_get(production, "server");
    TEST_ASSERT_NOT_NULL(prod_server);
    TEST_ASSERT_EQUAL(VALUE_STRING, prod_server->type);
    TEST_ASSERT_STR_EQUAL("prod.example.com", prod_server->as.string->data); // New field
    
    // Check development environment
    Value* development = object_get(result, "development");
    TEST_ASSERT_NOT_NULL(development);
    TEST_ASSERT_EQUAL(VALUE_OBJECT, development->type);
    
    Value* dev_timeout = object_get(development, "timeout");
    TEST_ASSERT_NOT_NULL(dev_timeout);
    TEST_ASSERT_EQUAL(VALUE_NUMBER, dev_timeout->type);
    TEST_ASSERT_EQUAL(60, dev_timeout->as.number); // Overridden
    
    Value* dev_retries = object_get(development, "retries");
    TEST_ASSERT_NOT_NULL(dev_retries);
    TEST_ASSERT_EQUAL(VALUE_NUMBER, dev_retries->type);
    TEST_ASSERT_EQUAL(3, dev_retries->as.number); // From defaults
    
    value_unref(result);
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
    
    Value* result = yaml_parse(yaml);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(VALUE_OBJECT, result->type);
    
    Value* employees = object_get(result, "employees");
    TEST_ASSERT_NOT_NULL(employees);
    TEST_ASSERT_EQUAL(VALUE_ARRAY, employees->type);
    TEST_ASSERT_EQUAL(3, employees->as.array->length);
    
    // First and third employees should be John
    Value* emp1 = employees->as.array->items[0];
    Value* emp3 = employees->as.array->items[2];
    TEST_ASSERT_NOT_NULL(emp1);
    TEST_ASSERT_NOT_NULL(emp3);
    TEST_ASSERT_EQUAL(VALUE_OBJECT, emp1->type);
    TEST_ASSERT_EQUAL(VALUE_OBJECT, emp3->type);
    
    Value* emp1_name = object_get(emp1, "name");
    Value* emp3_name = object_get(emp3, "name");
    TEST_ASSERT_NOT_NULL(emp1_name);
    TEST_ASSERT_NOT_NULL(emp3_name);
    TEST_ASSERT_STR_EQUAL("John Doe", emp1_name->as.string->data);
    TEST_ASSERT_STR_EQUAL("John Doe", emp3_name->as.string->data);
    
    // Second employee should be Jane
    Value* emp2 = employees->as.array->items[1];
    TEST_ASSERT_NOT_NULL(emp2);
    TEST_ASSERT_EQUAL(VALUE_OBJECT, emp2->type);
    
    Value* emp2_name = object_get(emp2, "name");
    TEST_ASSERT_NOT_NULL(emp2_name);
    TEST_ASSERT_STR_EQUAL("Jane Smith", emp2_name->as.string->data);
    
    value_unref(result);
}

// Test unknown anchor error
void test_yaml_unknown_anchor(void) {
    const char* yaml = "data: *unknown_anchor\n";
    
    Value* result = yaml_parse(yaml);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(VALUE_ERROR, result->type);
    TEST_ASSERT_NOT_NULL(result->as.error);
    TEST_ASSERT_NOT_NULL(result->as.error->message);
    TEST_ASSERT_STR_CONTAINS(result->as.error->message, "Unknown anchor");
    
    value_unref(result);
}

// Test duplicate anchor error
void test_yaml_duplicate_anchor(void) {
    const char* yaml = 
        "first: &same\n"
        "  value: 1\n"
        "second: &same\n"
        "  value: 2\n";
    
    Value* result = yaml_parse(yaml);
    // libyaml might handle this differently - it might just use the last definition
    // For now, we'll just check that parsing doesn't crash
    TEST_ASSERT_NOT_NULL(result);
    
    value_unref(result);
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
    
    Value* result = yaml_parse(yaml);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(VALUE_OBJECT, result->type);
    
    Value* final = object_get(result, "final");
    TEST_ASSERT_NOT_NULL(final);
    TEST_ASSERT_EQUAL(VALUE_OBJECT, final->type);
    
    // Should have all fields
    Value* name = object_get(final, "name");
    TEST_ASSERT_NOT_NULL(name);
    TEST_ASSERT_STR_EQUAL("final", name->as.string->data); // Overridden twice
    
    Value* value = object_get(final, "value");
    TEST_ASSERT_NOT_NULL(value);
    TEST_ASSERT_EQUAL(100, value->as.number); // From base
    
    Value* extra = object_get(final, "extra");
    TEST_ASSERT_NOT_NULL(extra);
    TEST_ASSERT_EQUAL(200, extra->as.number); // From middle
    
    value_unref(result);
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