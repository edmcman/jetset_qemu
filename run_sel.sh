#!/usr/bin/env bash

$( dirname "${BASH_SOURCE[0]}" )/build/arm-softmmu/qemu-system-m68k -cpu cfv4e -machine an5206 \
  -kernel /dev/null \
  -device loader,file=$(dirname "${BASH_SOURCE[0]}")/../jetset_private_data/sel/r2003cf1.bin,addr=0,force-raw=true  \
  -device loader,file=$(dirname "${BASH_SOURCE[0]}")/../jetset_private_data/sel/r512351s.bin.skipped,addr=0x100000,force-raw=true \
  -device loader,addr=0x420,cpu-num=0 \
  -serial stdio \
  -d nochain -singlestep -S -qmp tcp:localhost:$1,server,nowait
