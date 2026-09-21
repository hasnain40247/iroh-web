#!/usr/bin/env bash

echo "=== Compiling Java interpreter ==="
find interpreter -name "*.java" | xargs javac -d interpreter

echo "=== Compiling C interpreter ==="
cd ciroh-src
gcc -O2 -o ciroh main.c code/*.c frontend/*.c runtime/*.c -Iheaders -lm 2>/dev/null || \
  gcc -O2 -o ciroh main.c $(find . -name "*.c" ! -name "main.c") -Iheaders -lm
chmod +x ciroh
cd ..

echo "=== Done ==="
