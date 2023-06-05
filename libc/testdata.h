/*
 * Phoenix-RTOS
 *
 * libc-test-data
 *
 * Helpers regarding data for libc testing purposes
 *
 * Copyright 2023 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _TEST_DATA_LIBC_H
#define _TEST_DATA_LIBC_H

#include <stddef.h>


/* useful for testdata_createCharStr {1,1..255,0} -> 257 elements */
#define ALL_CHARS_STRING_SIZE (CHARS_SET_SIZE + 1)


/* returns string filled with all possible chars, which has to be freed after use */
extern char *testdata_createCharStr(int size);


#endif
