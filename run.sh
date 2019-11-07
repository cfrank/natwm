#!/bin/bash
set -e

function show_help() {
    echo "Usage: ./run <build|system> <wxh>"
}

readonly XEPHYR="Xephyr"

if [ ! -x $(command -v $XEPHYR) ]; then
    echo Xephyr is required but was not found
fi

readonly BUILD="build"
readonly SYSTEM="system"

readonly APP_NAME="natwm"
readonly BUILD_BIN_DIR="./build/bin"
readonly SYSTEM_BIN_DIR="/usr/bin"
readonly BUILD_BIN="$BUILD_BIN_DIR"/"$APP_NAME"
readonly SYSTEM_BIN="$SYSTEM_BIN_DIR"/"$APP_NAME"

if [ $# -lt 2 ]; then
    echo Invalid arguments count

    show_help

    exit
fi

if [ "$1" != "$BUILD" ] && [ "$1" != "$SYSTEM" ]; then
    echo Invalid argument: \""$1"\" - Expected either \"$BUILD\" or \"$SYSTEM\"

    show_help

    exit
fi

# Stupidly simple validation that this string at least contains an x
if [[ "$2" != *"x"* ]]; then
    echo Invalid argument: "$2" - Expected to be in the form \"800x600\"

    show_help

    exit
fi

readonly EXEC_TYPE=$1
readonly SCREEN_RES=$2

BIN=""

if [ "$EXEC_TYPE" == "$BUILD" ] && [ -f "$BUILD_BIN" ]; then
    BIN=$BUILD_BIN
elif [ "$EXEC_TYPE" == "$SYSTEM" ] && [ -f "$SYSTEM_BIN" ]; then
    BIN=$SYSTEM_BIN
fi

if [ "$BIN" == "" ]; then
    echo Could not find \"$EXEC_TYPE\" binary

    exit
fi

$XEPHYR :14 -ac -nolisten tcp -screen $SCREEN_RES &
sleep 5

export DISPLAY=:14
$($BIN)

killall $XEPHYR

echo $EXEC_TYPE
echo $SCREEN_RES
echo $BIN
