#!/bin/bash

# Install XCB libraries required to compile on OSX

BASE_GIT_URL="git://anongit.freedesktop.org/git"
XCB_GIT_URL="$BASE_GIT_URL/xcb"
XORG_GIT_URL="$BASE_GIT_URL/xorg"

DEP_DOWNLOAD_DIR="$TRAVIS_BUILD_DIR/.cache/dep-dl"

git_clone() {
    local url=$1
    local module=$2
    local output=$3

    echo $url $module $output

    cd $output

    git clone --recursive $url $module

    cd $module

    ACLOCAL="aclocal -I /usr/local/share/aclocal" ./autogen.sh

    make

    if [[ $CI == "true" ]]; then
        sudo make install
    fi
}

install_xorg() {
    echo "Installing XORG libraries..."

    git_clone "$XORG_GIT_URL/util/macros" "util-macros" $DEP_DOWNLOAD_DIR
    git_clone "$XORG_GIT_URL/proto/xorgproto" "xorgproto" $DEP_DOWNLAOD_DIR
    git_clone "$XORG_GIT_URL/lib/libXau" "libXau" $DEP_DOWNLOAD_DIR
    git_clone "$XORG_GIT_URL/lib/libxtrans" "libxtrans" $DEP_DOWNLOAD_DIR
    git_clone "$XORG_GIT_URL/lib/libX11" "libX11" $DEP_DOWNLOAD_DIR
}

install_xcb() {
    echo "Installing XCB libraries..."

    git_clone "$XCB_GIT_URL/pthread-stubs" "pthread-stubs" $DEP_DOWNLOAD_DIR
    git_clone "$XCB_GIT_URL/proto" "proto" $DEP_DOWNLOAD_DIR
    git_clone "$XCB_GIT_URL/libxcb" "libxcb" $DEP_DOWNLOAD_DIR
    git_clone "$XCB_GIT_URL/util" "xcb-util" $DEP_DOWNLOAD_DIR
    git_clone "$XCB_GIT_URL/util-keysyms" "util-keysyms" $DEP_DOWNLOAD_DIR

    git_clone "https://github.com/Airblader/xcb-util-xrm.git" "util-xrm" $DEP_DOWNLOAD_DIR
}

install() {
    echo "Installing X libraries"

    mkdir -p $DEP_DOWNLOAD_DIR

    install_xorg
    install_xcb
}

if [[ $TRAVIS_OS_NAME == "osx" ]]; then
    install
fi
