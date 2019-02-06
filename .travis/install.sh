#!/bin/bash

# Install dependencies for both Linux and OSX

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
    # OSX
    brew install cmake
    echo "OSX Build step"
else
    # Linux
    sudo apt-get install -y libxcb
fi
