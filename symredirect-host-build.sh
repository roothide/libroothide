#!/bin/bash

set -e
set -x

if [ $(uname -s) != "Darwin" ]; then
    echo "*** Cloning ld64 repository ..."
    [ -d ld64 ] && rm -rf ld64
    git clone https://github.com/ProcursusTeam/ld64.git

    echo "*** Checking the non-darwin headers ..."
    if [ ! -d ./ld64/EXTERNAL_HEADERS/non-darwin ]; then
        echo "The non-darwin headers are not present in the ld64 repository."
    fi

    EXTRA_HEADER_FLAG="-I./ld64/EXTERNAL_HEADERS/non-darwin"
fi

echo "*** Building the symredirect-host executable ..."
clang++ -v -std=c++11 $EXTRA_HEADER_FLAG -o symredirect-host symredirect.cpp

echo "*** Checking built symredirect-host executable ..."

chmod +x ./symredirect-host

./symredirect-host

echo "*** Installing symredirect-host to /usr/local/bin/symredirect ..."
sudo cp symredirect-host /usr/local/bin/symredirect
