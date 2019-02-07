#!/bin/bash

if [[ $TRAVIS_OS_NAME == "linux" ]]; then
    # Linux deps
elif [[ $TRAVIS_OS_NAME == "osx" ]]; then
    # OSX deps
    cd ci
    ./osx-xcb.sh
fi
