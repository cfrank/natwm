#!/bin/bash
set -e

readonly BUILD="dev"
readonly SYSTEM="system"

function show_help() {
    echo "Usage: ./run <$BUILD|$SYSTEM> <wxh> <wait?>"
}

readonly XEPHYR="Xephyr"

if [ ! -x $(command -v $XEPHYR) ]; then
    echo Xephyr is required but was not found
fi

readonly APP_NAME="natwm"
readonly BUILD_BIN_DIR="./build/bin"
readonly SYSTEM_BIN_DIR="/usr/bin"
readonly BUILD_BIN="$BUILD_BIN_DIR"/"$APP_NAME"
readonly SYSTEM_BIN="$SYSTEM_BIN_DIR"/"$APP_NAME"

if [ $# -lt 2 ] || [ $# -gt 3 ]; then
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
    echo Invalid argument: \""$2"\" - Expected to be in the form \"800x600\"

    show_help

    exit
fi

SHOULD_WAIT=false

if [ $# -eq 3 ] && [ "$3" == "wait" ]; then
    SHOULD_WAIT=true
fi

readonly EXEC_TYPE=$1
readonly SCREEN_RES=$2

BIN=""

if [ "$EXEC_TYPE" == "$BUILD" ] && [ -f "$BUILD_BIN" ]; then
    BIN=$BUILD_BIN
elif [ "$EXEC_TYPE" == "$SYSTEM" ] && [ -f "$SYSTEM_BIN" ]; then
    BIN=$SYSTEM_BIN
else
    echo Could not find \"$EXEC_TYPE\" binary

    exit
fi

#$XEPHYR :14 -ac -nolisten tcp -extension RANDR -screen $SCREEN_RES &
#$XEPHYR :14 -ac -nolisten tcp -extension RANDR -screen $SCREEN_RES -screen $SCREEN_RES &
#$XEPHYR :14 -ac -nolisten tcp +extension RANDR -screen $SCREEN_RES &
#$XEPHYR :14 -ac -nolisten tcp +extension RANDR -screen $SCREEN_RES -screen $SCREEN_RES &
$XEPHYR -ac -nolisten tcp +xinerama -extension RANDR -screen 800x600+0+0 -screen 800x600+800+0 -origin 800,0 :14 &

if [ $SHOULD_WAIT == true ]; then
    sleep 5
else
    sleep 1
fi

export DISPLAY=:14
xterm &
$($BIN)

killall $XEPHYR
