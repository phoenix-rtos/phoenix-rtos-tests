/*
 * Phoenix-RTOS
 *
 * libc-test-data
 *
 * Helpers regarding data for libc testing puproses
 *
 * Copyright 2023 Phoenix Systems
 * Author: Damian Loewnau
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <stdlib.h>
#include <limits.h>

char *testdata_createCharStr(int size)
{
	char *dataStr = calloc(size, 1);
	int i;

	if (dataStr == NULL) {
		return NULL;
	}

	for (i = 0; i < size - 1; i++) {
		dataStr[i] = i % (UCHAR_MAX + 1);
		/* double one to prevent setting NUL term */
		if (dataStr[i] == 0) {
			dataStr[i] = 1;
		}
	}

	return dataStr;
}
