/*
 * Phoenix-RTOS
 *
 * test-libc-string
 *
 * Main entry point.
 *
 * Copyright 2023 Phoenix Systems
 * Author: Mateusz Bloch
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "unity_fixture.h"

/* no need for forward declarations, RUN_TEST_GROUP does it by itself */
void runner(void)
{
	RUN_TEST_GROUP(string_len);
	RUN_TEST_GROUP(string_chr);
	RUN_TEST_GROUP(string_memset);
	RUN_TEST_GROUP(string_memmove);
	RUN_TEST_GROUP(string_memmove_big);
	RUN_TEST_GROUP(string_memcpy);
	RUN_TEST_GROUP(string_memccpy);
	RUN_TEST_GROUP(string_strncpy);
	RUN_TEST_GROUP(string_stpncpy);
	RUN_TEST_GROUP(string_strcpy_stpcpy);
	RUN_TEST_GROUP(string_strlcpy);
	RUN_TEST_GROUP(string_strlcat);
	RUN_TEST_GROUP(string_errsign);
	RUN_TEST_GROUP(signal_psignal);
	RUN_TEST_GROUP(string_cat);
	RUN_TEST_GROUP(string_dup);
	RUN_TEST_GROUP(string_spn);
	RUN_TEST_GROUP(string_memcmp);
	RUN_TEST_GROUP(string_strncmp);
	RUN_TEST_GROUP(string_strcmp);
	RUN_TEST_GROUP(string_strcoll);
	RUN_TEST_GROUP(string_tok);
	RUN_TEST_GROUP(string_tok_r);
	RUN_TEST_GROUP(string_str);
	RUN_TEST_GROUP(string_pbrk);
}


int main(int argc, char *argv[])
{
	UnityMain(argc, (const char **)argv, runner);

	return 0;
}
