#!/usr/bin/env bash
which indent &> /dev/null || { echo "missing indent command"; exit 1; }
indent -linux *.c *.h || exit 1
rm *~
