#!/bin/sh

LD=mold

echo "Compiling naws..."
mkdir -p build
nasm -f elf64 -o build/http.o -O3 http.asm
nasm -f elf64 -o build/naws.o -O3 naws.asm

echo "Linking naws..."
$LD -o naws build/*

echo "Ensuring it is executable..."
chmod +x naws

echo "Final binary size: `stat -c %s naws` bytes."

