#!/bin/sh
sh-elf-objcopy -R .stack -O binary $1 `basename $1`.bin

#scrambling process - make sure you have your path right
$KOS_BASE/utils/scramble/scramble `basename $1`.bin 1ST_READ.BIN
