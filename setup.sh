#!/bin/bash
#
set -e
git init ELFIO
pushd ELFIO

git sparse-checkout init
git sparse-checkout set "elfio" "LICENSE.txt"
git remote add origin https://github.com/serge1/ELFIO.git
git pull --depth=1 origin Release_3.12
popd
