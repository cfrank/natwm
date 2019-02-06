#!/bin/bash

# Install dependencies for both Linux and OSX

if [[ $TRAVIS_OS_NAME == 'linux' ]]; then
    # Linux
    ./dependencies/cmake.sh
else
    # OSX
    echo "OSX Build step"
fi
