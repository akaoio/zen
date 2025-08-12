#!/bin/bash
export ZEN_DEBUG=1
export ZEN_LOG_CATEGORIES=PARSER
./zen test_final.zen 2>&1 | head -100