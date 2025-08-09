#!/bin/bash
./zen test_class_verbose.zen 2>&1 | grep -A5 -B5 "After class parse"