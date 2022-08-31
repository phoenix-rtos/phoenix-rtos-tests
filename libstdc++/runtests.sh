#!/bin/bash
# $1 - toolchain target (e.g. arm-phoenix)
# $2 - toolchain install absolute path (i.e. no "." or ".." in the path)

set -e

log() {
    echo -e "\e[35;1m$*\e[0m"
}

update_config_file() {
    FILE=$1

    if [ -f "$FILE" ]; then
        echo "Update config file..."
        LINE_1="set phoenix_rtos_system_files \"$(pwd)\""

        if [ "$(grep -w "$LINE_1" "$FILE")" != "$LINE_1" ]; then
            printf "\n%s" "$LINE_1" >> "$FILE"
            echo "Added line: $LINE_1"
        fi

        LINE_2="lappend boards_dir \"$(pwd)/phoenix-rtos-tests/libstdc++\""
        if [ "$(grep -w "$LINE_2" "$FILE")" != "$LINE_2" ]; then
            printf "\n%s" "$LINE_2" >> "$FILE"
            echo "Added line: $LINE_2"
        fi

        return 0
    fi

    echo "No config file"
    return 1
}

TARGET_B="$1"
TARGET_T=""

case $TARGET_B in
    "ia32-generic-qemu")
        TARGET_T="i386-pc-phoenix"
    ;;

    *)
        echo "Target $TARGET_B in not available."
        return 1
    ;;
esac

GCC=gcc-9.3.0

BUILD_ROOT="$2"
BILDIR="$BUILD_ROOT/$GCC/build"
FILE="$BILDIR/$TARGET_T/libstdc++-v3/testsuite/site.exp"
DEJAGNU_FILES="$(pwd)/phoenix-rtos-tests/libstdc++"

if [ $# == 3 ]; then
    if [[ "$3" =~ ^[\ ]*-v([\ ]+-v)*[\ ]*$ ]]; then
        RUNTESTFLAGS="--target_board=${TARGET_B} $3"
    else
        RUNTESTFLAGS="conformance.exp=$3/* --target_board=${TARGET_B}"
    fi
elif [ $# == 4 ]; then
    RUNTESTFLAGS="conformance.exp=$3/* --target_board=${TARGET_B} $4"
else
    RUNTESTFLAGS="--target_board=${TARGET_B}"
fi

TARGET=$TARGET_B
CONSOLE=serial

export RUNTESTFLAGS TARGET CONSOLE

# all qemu files needed for communication with the target should be present
if [ ! -f "$DEJAGNU_FILES/qemu.exp" ] || [ ! -f "$DEJAGNU_FILES/basic-qemu.exp" ]; then 
    echo "Files qemu.exp and basic-qemu.exp doesn't exist."
    return 1
fi

if [ ! -f "$DEJAGNU_FILES/${TARGET_B}.exp" ]; then
    echo "Files ${TARGET_B}.exp doesn't exist."
    return 1
fi

# complete libstdc++-v3 local config file with the target and system related data
log "Preparing config file"
if ! update_config_file "$FILE" ; then
    pushd "$BILDIR" > /dev/null
    echo "---------------- Genereting necessary files ----------------"
    make check-target-libstdc++-v3
    echo "---------------- -------------------------- ----------------"
    popd > /dev/null

    if ! update_config_file "$FILE" ; then
        echo "File $FILE doesn't exist."
        return 1
    fi
fi

pushd "$BILDIR" > /dev/null

log "Starting tests"
make check-target-libstdc++-v3

popd > /dev/null
