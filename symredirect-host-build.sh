#!/bin/bash

set -e
set -x

echo "*** Cloning ld64 repository ..."
[ -d ld64 ] && rm -rf ld64
git clone https://github.com/ProcursusTeam/ld64.git

echo "*** Checking the non-darwin headers ..."
if [ ! -d ./ld64/EXTERNAL_HEADERS/non-darwin ]; then
    echo "The non-darwin headers are not present in the ld64 repository."
fi

echo "*** Building the symredirect-host executable ..."
clang++ -v -std=c++11 -I./ld64/EXTERNAL_HEADERS/non-darwin -o symredirect-host symredirect.cpp

echo "*** Checking built symredirect-host executable ..."

chmod +x ./symredirect-host

./symredirect-host

echo "*** Installing symredirect-host to /usr/local/bin/symredirect ..."
sudo cp symredirect-host /usr/local/bin/symredirect
