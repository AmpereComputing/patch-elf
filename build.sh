#!/bin/bash
#
set -e
# Intended for native AARCH64 use
export CPLUS_INCLUDE_PATH=`pwd`/ELFIO
c++ -std=c++17 -I./ELFIO -o patch-elf patch-elf.cpp
