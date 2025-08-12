#!/bin/bash
export ZEN_DEBUG=1
export ZEN_LOG_CATEGORIES=PARSER
./zen test_debug_prop2.zen 2>&1 | grep -E "Function call|Parsing argument|Parsed stdlib|Property access"