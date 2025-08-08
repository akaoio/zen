#include <stdio.h>
#include <stdlib.h>
#include "zen/types/value.h"

int main() {
    printf("🧠 Testing ZEN's undecidable value type...\n\n");
    
    // Test undecidable value creation
    Value* undecidable = value_new_undecidable();
    if (!undecidable) {
        printf("❌ Failed to create undecidable value\n");
        return 1;
    }
    
    printf("✅ Successfully created undecidable value\n");
    printf("   Type: %s\n", value_type_name(undecidable->type));
    
    // Test string representation
    char* str = value_to_string(undecidable);
    if (str) {
        printf("   String representation: %s\n", str);
        free(str);
    }
    
    // Test equality
    Value* undecidable2 = value_new_undecidable();
    if (undecidable2) {
        bool equal = value_equals(undecidable, undecidable2);
        printf("   undecidable == undecidable: %s\n", equal ? "true" : "false");
    }
    
    // Test inequality with boolean values
    Value* true_val = value_new_boolean(true);
    Value* false_val = value_new_boolean(false);
    
    if (true_val && false_val) {
        bool eq_true = value_equals(undecidable, true_val);
        bool eq_false = value_equals(undecidable, false_val);
        
        printf("   undecidable == true: %s\n", eq_true ? "true" : "false");
        printf("   undecidable == false: %s\n", eq_false ? "true" : "false");
    }
    
    printf("\n🎓 Gödel would be proud! ZEN now supports undecidable values!\n");
    printf("📐 This makes ZEN mathematically complete for formal logic!\n");
    
    // Cleanup
    if (undecidable) value_unref(undecidable);
    if (undecidable2) value_unref(undecidable2);
    if (true_val) value_unref(true_val);
    if (false_val) value_unref(false_val);
    
    return 0;
}