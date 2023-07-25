/*
 * Phoenix-RTOS
 *
 * libc-test-data
 *
 * Helpers regarding data for libc testing purposes
 *
 * Copyright 2023 Phoenix Systems
 * Author: Damian Loewnau, Mateusz Bloch
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


/* 
 * for (size >= ALL_CHARS_STRING_SIZE) returns string filled with all possible chars.
 * for (size < ALL_CHARS_STRING_SIZE) returns string with the following content: {1,1,2,...(size-2),0}.
 * The returned pointer has to be freed after use!
 */
extern char *testdata_createCharStr(int size);


/* large string filled with lorem ipsum data */
extern const char testdata_hugeStr[];


/* returns size of the testdata_huge_str */
extern const size_t testdata_hugeSize;


#endif
