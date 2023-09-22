/*
 * Phoenix-RTOS
 *
 * test-libc-time
 *
 * Code for generating test cases.
 *
 * Copyright 2020 Phoenix Systems
 * Author: Marcin Brzykcy
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "time_common.h"


#define NCOLS 9


/* help function for future test generation if needed - not used in test functions */
void generate_input_mktime(int input_length)
{
	int i;
	srand(time(NULL));
	for (i = 0; i < input_length; i++) {
		printf("{ %d, %d, %d, %d, %d, %d, %d, %d, %d }", rand() % 59, rand() % 59, rand() % 23,
			rand() % 30, rand() % 11, 80 + rand() % 60, 0, 0, 0);
		if (i != input_length - 1)
			printf(",\n");
	}
	printf("\n");
}


/* help function for future test generation if needed - not used in test functions */
void generate_output_mktime(const int input_vector[][NCOLS], int input_length)
{
	int i;
	struct tm t;
	time_t timestamp;
	printf("Printing host output data. Struct tm member values:\n");
	for (i = 0; i < input_length; i++) {
		init_tm(&t, input_vector[i]);
		timestamp = mktime(&t);
		printf("{ %d, %d, %d, %d, %d, %d, %d, %d, %d }",
			t.tm_sec, t.tm_min, t.tm_hour, t.tm_mday, t.tm_mon, t.tm_year,
			t.tm_wday, t.tm_yday, t.tm_isdst);
		if (i != input_length - 1)
			printf(",\n");
	}
	printf("\nTimestamp values:\n{");
	for (i = 0; i < input_length; i++) {
		init_tm(&t, input_vector[i]);
		timestamp = mktime(&t);
		printf("%lld", (long long int)timestamp);
		if (i != input_length - 1)
			printf(", ");
	}
	printf("}\n");
}


/* help function for future test generation if needed - not used in test functions */
void generate_input_host_gmtime(int input_length)
{
	int i;
	struct tm t;
	srand(time(NULL));
	for (i = 0; i < input_length; i++) {
		t = (struct tm) {
			.tm_sec = rand() % 59,
			.tm_min = rand() % 59,
			.tm_hour = rand() % 23,
			.tm_mday = rand() % 30,
			.tm_mon = rand() % 11,
			.tm_year = 80 + rand() % 60,
			.tm_wday = 0,
			.tm_yday = 0,
			.tm_isdst = 0,
		};
		printf("%lld", (long long int)mktime(&t));
		if (i != input_length - 1)
			printf(",\n");
	}
	printf("\n");
}


/* help function for future test generation if needed - not used in test functions */
void generate_output_host_gmtime(const time_t input_vector[], int input_length)
{
	int i;
	struct tm *t;
	printf("Printing host output data. Struct tm member values:\n");
	for (i = 0; i < input_length; i++) {
		t = gmtime(&input_vector[i]);
		printf("{ %d, %d, %d, %d, %d, %d, %d, %d, %d }",
			t->tm_sec, t->tm_min, t->tm_hour, t->tm_mday, t->tm_mon, t->tm_year,
			t->tm_wday, t->tm_yday, t->tm_isdst);
		if (i != input_length - 1)
			printf(",\n");
	}
	printf("\n");
}
