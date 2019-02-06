#!/bin/bash

if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
    echo "Installing Cmake from travis dependencies"

    source_dir=$TRAVIS_BUILD_DIR/.travis/dependencies
    target_dir=$TRAVIS_BUILD_DIR/.travis/resolved_dependencies

    if [[ -f $target_dir/cmake/bin/cmake ]]; then
        echo "Found cmake - no need to re-install"
    else
        echo "Starting to build cmake..."

        mkdir -p $target_dir

        cd $source_dir/CMake
        ./bootstrap --prefix=$target_dir
        make
        ls -la
    fi
elif [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
    echo "Upgrading cmake with brew..."
    brew upgrade cmake
fi
