/**
 * @file regex.h
 * @brief PCRE2-based regular expression support for ZEN language - Phase 2B
 * @details Function declarations for natural language regex patterns
 * 
 * This header declares the PCRE2-based regex functionality for ZEN,
 * supporting pattern matching, replacement, splitting, and compilation
 * with natural language integration.
 * 
 * @author ZEN Multi-Swarm Development System
 * @version 1.0.0
 */

#ifndef ZEN_STDLIB_REGEX_H
#define ZEN_STDLIB_REGEX_H

#include "zen/types/value.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Match text against regular expression pattern
 * @param text_value String value containing text to match
 * @param pattern_value String value containing regex pattern
 * @return Object with match results or error Value
 * 
 * Returns an object containing:
 * - matched: boolean indicating if any match was found
 * - matches: array of match objects with text, start, and end properties
 * - count: number of matches found
 * 
 * Example ZEN usage:
 * ```zen
 * set result regex_match "Hello 123 World" "\\d+"
 * if result.matched
 *     print "Found number: " + result.matches[0].text
 * ```
 */
Value* regex_match(const Value* text_value, const Value* pattern_value);

/**
 * @brief Replace pattern matches in text with replacement
 * @param text_value String value containing text
 * @param pattern_value String value containing regex pattern  
 * @param replacement_value String value containing replacement text
 * @return New string with replacements or error Value
 * 
 * Performs global replacement of all pattern matches.
 * Supports PCRE2 substitution syntax for backreferences.
 * 
 * Example ZEN usage:
 * ```zen
 * set result regex_replace "Hello 123 World 456" "\\d+" "NUMBER"
 * print result  # "Hello NUMBER World NUMBER"
 * ```
 */
Value* regex_replace(const Value* text_value, const Value* pattern_value, const Value* replacement_value);

/**
 * @brief Split text by regular expression pattern
 * @param text_value String value containing text to split
 * @param pattern_value String value containing regex pattern
 * @return Array of string parts or error Value
 * 
 * Splits the input text at each occurrence of the pattern.
 * Returns array of substrings between matches.
 * 
 * Example ZEN usage:
 * ```zen
 * set parts regex_split "apple,banana;orange" "[,;]"
 * # parts = ["apple", "banana", "orange"]
 * ```
 */
Value* regex_split(const Value* text_value, const Value* pattern_value);

/**
 * @brief Compile regular expression pattern for reuse
 * @param pattern_value String value containing regex pattern
 * @return Compiled pattern object or error Value
 * 
 * Pre-compiles a regex pattern for performance when using the same
 * pattern multiple times. Returns an object with pattern information.
 * 
 * Returns an object containing:
 * - pattern: original pattern string
 * - compiled: boolean true if compilation succeeded
 * - capture_count: number of capturing groups
 * - utf8: boolean indicating UTF-8 support
 * - multiline: boolean indicating multiline mode
 * 
 * Example ZEN usage:
 * ```zen
 * set compiled_pattern regex_compile "\\b\\w+@\\w+\\.\\w+\\b"
 * if compiled_pattern.compiled
 *     print "Pattern has " + compiled_pattern.capture_count + " groups"
 * ```
 */
Value* regex_compile(const Value* pattern_value);

/**
 * @brief Cleanup regex cache on shutdown
 * @internal
 * 
 * This function cleans up the internal pattern cache and should be
 * called during program shutdown. Not exposed to ZEN code.
 */
void regex_cleanup(void);

// Stdlib wrapper functions (for ZEN runtime integration)

/**
 * @brief Regex match stdlib wrapper
 * @param args Arguments array: [text_string, pattern_string]
 * @param argc Number of arguments
 * @return Match results object or error value
 */
Value* regex_match_stdlib(Value** args, size_t argc);

/**
 * @brief Regex replace stdlib wrapper
 * @param args Arguments array: [text_string, pattern_string, replacement_string]
 * @param argc Number of arguments
 * @return New string with replacements or error value
 */
Value* regex_replace_stdlib(Value** args, size_t argc);

/**
 * @brief Regex split stdlib wrapper
 * @param args Arguments array: [text_string, pattern_string]
 * @param argc Number of arguments
 * @return Array of string parts or error value
 */
Value* regex_split_stdlib(Value** args, size_t argc);

/**
 * @brief Regex compile stdlib wrapper
 * @param args Arguments array: [pattern_string]
 * @param argc Number of arguments
 * @return Compiled pattern object or error value
 */
Value* regex_compile_stdlib(Value** args, size_t argc);

#ifdef __cplusplus
}
#endif

#endif /* ZEN_STDLIB_REGEX_H */