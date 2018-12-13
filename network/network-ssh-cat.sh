#!/bin/bash
# Phoenix-RTOS
#
# Login using ssh, print file contents and calculates md5
#
# Copyright 2018 Phoenix Systems
# Author: Bartosz Ciesla
#
# This file is part of Phoenix-RTOS.

if [ $# -ne 3 ];
then
    echo "Usage: $(basename $0) username ip_address test_iterations"
    echo "Tests network connection robustness by repeatedly 'downloading' file using SSH and calculating it's checksum"
    exit
fi

USER=$1
IP_ADDRESS=$2
TEST_ITERATIONS=$3
MAX_SCP_RETRIES=3
SSH_TIMEOUT_SEC=3

RANDOM_SOURCE="/dev/urandom"
RANDOM_FILE="./random_sample.txt"
DOWNLOADED_FILE=${RANDOM_FILE}

# Create sample file
dd if=${RANDOM_SOURCE} of=${RANDOM_FILE} bs=1M count=1 > /dev/null 2>&1

# Try to copy file to device
copy_success=false
for ((i=1;i<=${MAX_SCP_RETRIES};i+=1))
do
    scp -o ConnectTimeout=${SSH_TIMEOUT_SEC} ${RANDOM_FILE} ${USER}@${IP_ADDRESS}:${RANDOM_FILE} > /dev/null 2>&1
    if [ $? -eq 0 ];
    then
        copy_success=true
        break
    fi
done

if ! $copy_success;
then
    echo "SCP: Failed to copy sample file to device"
    rm -f ${RANDOM_FILE}
    exit
fi

# Calculate it's checksum
RANDOM_FILE_CHECKSUM=$(md5sum -b ${RANDOM_FILE})
RANDOM_FILE_CHECKSUM=${RANDOM_FILE_CHECKSUM:0:32} # Extract only md5sum

SSH_FAIL=0
TEST_SUCCESS=0
TEST_FAIL=0

for ((i=1;i<=${TEST_ITERATIONS};i+=1))
do
    ssh -o ConnectTimeout=${SSH_TIMEOUT_SEC} ${USER}@${IP_ADDRESS} cat ${RANDOM_FILE} > ${DOWNLOADED_FILE}
    if [ ! $? -eq 0 ];
    then
        SSH_FAIL=$((SSH_FAIL+1))
        TEST_FAIL=$((TEST_FAIL+1))
        # There's no use to calculate MD5
        continue
    fi

    DOWNLOADED_CHECKSUM=$(md5sum -b ${DOWNLOADED_FILE})
    DOWNLOADED_CHECKSUM=${DOWNLOADED_CHECKSUM:0:32} # Extract only md5sum

    if [ ${DOWNLOADED_CHECKSUM} == ${RANDOM_FILE_CHECKSUM} ];
    then
        TEST_SUCCESS=$((TEST_SUCCESS+1))
    else
        TEST_FAIL=$((TEST_FAIL+1))
    fi
done

echo "Test run ${TEST_ITERATIONS} times"
echo "Success: ${TEST_SUCCESS}/${TEST_ITERATIONS}"
echo "Fail: ${TEST_FAIL}/${TEST_ITERATIONS}, where SSH failed ${SSH_FAIL} times"

rm -f ${RANDOM_FILE}
rm -f ${DOWNLOADED_FILE}

