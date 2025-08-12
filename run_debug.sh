#!/bin/bash
ZEN_DEBUG=1 ./zen test_trace.zen 2>&1 | grep -E "Visiting function call|property"