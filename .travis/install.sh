#!/bin/bash

# Install dependencies for both Linux and OSX

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
    # OSX
    echo "OSX Build step"
else
    # Linux
    sudo apt-get install -y libxcb
fi
