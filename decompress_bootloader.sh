#!/bin/bash


dd if="$1" of="$1.lzma" bs=1 skip=$(( 0x1fdd )) 
lzma -v -d -F raw --lzma1=dict=$(( 0x00020000 )) $1.lzma --stdout > "$1.decompressed"
mipsel-linux-gnu-objdump -D -m mips:isa32r2 -b binary  --endian=big "$1.decompressed" > "$1.asm"
