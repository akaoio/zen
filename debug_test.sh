#!/bin/bash
export ZEN_DEBUG=1
./zen test_zen_prop_access.zen 2>&1 | grep -E "Property access check|name"