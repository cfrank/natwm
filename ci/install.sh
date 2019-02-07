#!/bin/bash

if [[ $TRAVIS_OS_NAME == "linux" ]]; then
    # Linux deps
    echo "Any linux deps would be installed now..."
elif [[ $TRAVIS_OS_NAME == "osx" ]]; then
    # OSX deps
    cd ci
    ./osx-xcb.sh
fi
