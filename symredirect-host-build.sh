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

if [ $(uname -s) = "Darwin" ] && [ "$(sw_vers -productName)" != "macOS" ]; then
    echo "*** Signing the symredirect-host with entitlements ..."
    ldid -S./entitlements.plist ./symredirect-host

    if [ -f /basebin/fastPathSign ]; then
        echo "*** Signing the symredirect-host with fastPathSign ..."
        ldid -M -S/basebin/bootstrap.entitlements ./symredirect-host
        /basebin/fastPathSign ./symredirect-host
    fi
fi

echo "*** Checking built symredirect-host executable ..."

chmod +x ./symredirect-host

./symredirect-host

echo "*** Installing symredirect-host to /usr/local/bin/symredirect ..."
sudo rm -f /usr/local/bin/symredirect #clear signature cache
[ -d /usr/local/bin ] || sudo mkdir -p /usr/local/bin
sudo cp symredirect-host /usr/local/bin/symredirect
