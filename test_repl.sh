#!/bin/bash
{
    echo "set x 42"
    sleep 1
    echo "print x"
    sleep 1
    echo "exit"
} | timeout 15 ./zen