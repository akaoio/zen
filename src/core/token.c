#include "zen/core/token.h"

#include "zen/core/memory.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Create new token
 * @param type The token type
 * @param value The token value string
 * @return Pointer to newly allocated token
 */
token_T *token_new(int type, char *value)
{
    token_T *token = memory_alloc(sizeof(struct TOKEN_STRUCT));
    if (!token) {
        return NULL;
    }

    token->type = type;
    token->value = value;

    return token;
}

/**
 * @brief Free a token and its value
 * @param token Token to free
 */
void token_free(token_T *token)
{
    if (!token) {
        return;
    }

    // Only free token->value if it was dynamically allocated
    // String literals (like "+", "-", etc.) are in read-only memory and should not be freed
    // We can detect dynamic allocation by checking if the memory system tracks it
    if (token->value && memory_is_tracked(token->value)) {
        memory_free(token->value);
    }

    memory_free(token);
}
