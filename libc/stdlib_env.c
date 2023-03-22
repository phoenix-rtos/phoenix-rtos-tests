/*
 * Phoenix-RTOS
 *
 *    POSIX.1-2017 standard library functions tests
 * HEADER:
 *    - stdlib.h
 * TESTED:
 *    - setenv()
 *    - putenv()
 *    - getenv()
 *    - unsetenv()
 *    - clearenv()
 *
 * Copyright 2017, 2023 Phoenix Systems
 * Author: Krystian Wasik, Adam Debek
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <unity_fixture.h>

#define NUM_OF_VARIABLES (64)
#define NUM_OF_TESTS     (1000)
#define MAX_NAME_LEN     (64)
#define MAX_VALUE_LEN    (32)

/* Defines how often environment should be cleared (approximately) */
#define CLEARENV_APPROX_EVERY (200)

#define TEST_VAR     ("TEST_VARIABLE")
#define TEST_VALUE   ("TEST_VALUE")
#define TEST_VALUE2  ("TEST_VALUE2")
#define INVALID_VAL1 ("VA=LUE")
#define INVALID_VAL2 ("=VALUE")
#define INVALID_VAL3 ("VALUE=")
#define INVALID_VAR1 ("NAME=")
#define INVALID_VAR2 ("NA=ME")
#define INVALID_VAR3 ("=NAME")
#define NOT_SET_VAR  ("NOT_SET_VAR")
#define VAR_VAL      ("NAME=VALUE")


typedef struct {
	char name[MAX_NAME_LEN];                /* string containing name of env variable */
	char value[MAX_VALUE_LEN];              /* string containing value of env variable */
	char str[MAX_NAME_LEN + MAX_VALUE_LEN]; /* string of the form "name=value" */
	unsigned int set;                       /* flag indicating if env variable is set */
} env_var_t;

static env_var_t vars[NUM_OF_VARIABLES];

extern char **environ;


TEST_GROUP(stdlib_env);


TEST_SETUP(stdlib_env)
{
}


TEST_TEAR_DOWN(stdlib_env)
{
}


static int test_getEnvironLen(char **ep)
{
	int i = 0;
	if (ep != NULL)
		while (*ep != NULL) {
			++i;
			++ep;
		}
	else {
		return 0;
	}

	return i;
}


static char *test_getAsciiStr(void)
{
	char *str = (char *)malloc(128 * sizeof(char));
	TEST_ASSERT_NOT_NULL(str);
	int i;

	for (i = 1; i < 128; i++) {
		if (i == '=') {
			str[i - 1] = 'a';
		}
		else {
			str[i - 1] = i;
		}
	}
	str[i - 1] = '\0';

	return str;
}


static char *test_getLongStr(void)
{
	char *str = (char *)malloc(1024 * sizeof(char));
	TEST_ASSERT_NOT_NULL(str);
	int i;

	for (i = 0; i < 1023; i++) {
		str[i] = 'a';
	}
	str[i] = '\0';

	return str;
}


TEST(stdlib_env, clearenv)
{
	TEST_ASSERT_EQUAL_INT(0, clearenv());
	TEST_ASSERT_EQUAL_INT(0, test_getEnvironLen(environ));

	errno = 0;
	TEST_ASSERT_EQUAL_INT(0, setenv(TEST_VAR, TEST_VALUE, 0));
	TEST_ASSERT_EQUAL_INT(0, errno);

	TEST_ASSERT_EQUAL_STRING(TEST_VALUE, getenv(TEST_VAR));

	TEST_ASSERT_EQUAL_INT(0, clearenv());
	TEST_ASSERT_EQUAL_INT(0, test_getEnvironLen(environ));
}


TEST(stdlib_env, basic)
{
	errno = 0;
	TEST_ASSERT_EQUAL_INT(0, setenv(TEST_VAR, TEST_VALUE, 0));
	TEST_ASSERT_EQUAL_INT(0, errno);

	TEST_ASSERT_EQUAL_STRING(TEST_VALUE, getenv(TEST_VAR));

	errno = 0;
	TEST_ASSERT_EQUAL_INT(0, putenv(VAR_VAL));
	TEST_ASSERT_EQUAL_INT(0, errno);

	TEST_ASSERT_EQUAL_STRING("VALUE", getenv("NAME"));

	errno = 0;
	TEST_ASSERT_EQUAL_INT(0, unsetenv("NAME"));
	TEST_ASSERT_EQUAL_INT(0, errno);

	TEST_ASSERT_NULL(getenv("NAME"));
}


TEST(stdlib_env, long_args)
{
	char *name = test_getLongStr();
	char *value = test_getLongStr();
	int len = strlen(name) + strlen(value) + 2;
	char s[len];
	len = strlen(name);

	strcpy(s, name);
	s[len] = '=';
	strcpy(s + (len + 1), value);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(0, setenv(name, value, 0));
	TEST_ASSERT_EQUAL_INT(0, errno);

	TEST_ASSERT_EQUAL_STRING(value, getenv(name));

	errno = 0;
	TEST_ASSERT_EQUAL_INT(0, unsetenv(name));
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(0, putenv(s));
	TEST_ASSERT_EQUAL_INT(0, errno);

	TEST_ASSERT_EQUAL_STRING(value, getenv(name));

	errno = 0;
	TEST_ASSERT_EQUAL_INT(0, unsetenv(name));
	TEST_ASSERT_EQUAL_INT(0, errno);

	TEST_ASSERT_NULL(getenv(name));

	free(name);
	free(value);
}


TEST(stdlib_env, ascii)
{
	char *name = test_getAsciiStr();
	char *value = test_getAsciiStr();
	int len = strlen(name) + strlen(value) + 2;
	char s[len];
	len = strlen(name);

	strcpy(s, name);
	s[len] = '=';
	strcpy(s + (len + 1), value);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(0, setenv(name, value, 1));
	TEST_ASSERT_EQUAL_INT(0, errno);

	TEST_ASSERT_EQUAL_STRING(value, getenv(name));

	errno = 0;
	TEST_ASSERT_EQUAL_INT(0, putenv(s));
	TEST_ASSERT_EQUAL_INT(0, errno);

	TEST_ASSERT_EQUAL_STRING(value, getenv(name));

	errno = 0;
	TEST_ASSERT_EQUAL_INT(0, unsetenv(name));
	TEST_ASSERT_EQUAL_INT(0, errno);

	TEST_ASSERT_NULL(getenv(name));

	free(name);
	free(value);
}


TEST(stdlib_env, random)
{
	unsigned int i, j, clearenvCnt = 0, putenvCnt = 0, setenvCnt = 0, unsetenvCnt = 0;

	srand(9);

	/* Initialize variables */
	for (i = 0; i < NUM_OF_VARIABLES; i++) {
		vars[i].set = 0;
		sprintf(vars[i].name, "VARIABLE%u", i);
		vars[i].value[0] = '\0';
	}

	/* Run tests */
	for (i = 0; i < NUM_OF_TESTS; i++) {

		if (rand() % CLEARENV_APPROX_EVERY == 0) {
			clearenvCnt++;
			TEST_ASSERT_EQUAL_INT(0, clearenv());

			for (j = 0; j < NUM_OF_VARIABLES; j++) {
				vars[j].set = 0;
			}
		}
		else {
			unsigned int idx = rand() % NUM_OF_VARIABLES;
			unsigned int action = rand() % 3;
			if (action == 2) { /* unsetenv */
				unsetenvCnt++;
				TEST_ASSERT_EQUAL_INT(0, unsetenv(vars[idx].name));

				vars[idx].set = 0;
			}
			else { /* insert (setenv or putenv) */

				/* Generate random value */
				unsigned int len = rand() % MAX_VALUE_LEN;
				char newValue[MAX_VALUE_LEN];
				for (j = 0; j < len; j++) {
					char c;
					do {
						c = 33 + rand() % 94;
					} while (c == '=');
					newValue[j] = c;
				}
				newValue[len] = '\0';

				if (action) { /* setenv */
					unsigned int overwrite = rand() % 2;

					setenvCnt++;
					TEST_ASSERT_EQUAL_INT(0, setenv(vars[idx].name, newValue, overwrite));

					if (!vars[idx].set || overwrite) {
						strcpy(vars[idx].value, newValue);
					}
				}
				else { /* putenv */
					strcpy(vars[idx].value, newValue);

					char *s = vars[idx].str;
					strcpy(s, vars[idx].name);
					s += strlen(vars[idx].name);
					*(s++) = '=';
					strcpy(s, vars[idx].value);

					putenvCnt++;
					TEST_ASSERT_EQUAL_INT(0, putenv(vars[idx].str));
				}

				vars[idx].set = 1;
			}
		}

		/* Verify */
		for (j = 0; j < NUM_OF_VARIABLES; j++) {
			char *var = getenv(vars[j].name);
			if (vars[j].set) { /* var set check if it is correct */
				TEST_ASSERT_NOT_NULL(var);
				TEST_ASSERT_EQUAL_STRING(vars[j].value, var);
			}
			else { /* var not set must be null */
				TEST_ASSERT_NULL(var);
			}
		}
	}
}


TEST(stdlib_env, empty_name)
{
	int len;

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, setenv("", TEST_VALUE, 0));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, setenv("", TEST_VALUE, 1));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	TEST_ASSERT_NULL(getenv(""));

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, unsetenv(""));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	len = test_getEnvironLen(environ);
	TEST_ASSERT_EQUAL_INT(0, putenv(INVALID_VAL2));
	TEST_ASSERT_EQUAL_INT(len + 1, test_getEnvironLen(environ));

#pragma GCC diagnostic ignored "-Wunused-result"
	/* Invoking getenv() with empty string is implementation defined,
	   invoked below to check if it won't crash anything */
	getenv("");
}


TEST(stdlib_env, empty_value)
{
	errno = 0;
	TEST_ASSERT_EQUAL_INT(0, unsetenv(TEST_VAR));
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(0, setenv(TEST_VAR, "", 0));
	TEST_ASSERT_EQUAL_INT(0, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(0, setenv(TEST_VAR, "", 1));
	TEST_ASSERT_EQUAL_INT(0, errno);

	TEST_ASSERT_EQUAL_STRING("", getenv(TEST_VAR));

	errno = 0;
	TEST_ASSERT_EQUAL_INT(0, unsetenv(TEST_VAR));
	TEST_ASSERT_EQUAL_INT(0, errno);

	/* In putenv case INVALID_VAR1 is valid */
	TEST_ASSERT_EQUAL_INT(0, putenv(INVALID_VAR1));

	TEST_ASSERT_EQUAL_STRING("", getenv("NAME"));
}


TEST(stdlib_env, name_null)
{
	/* This is the only possible case of passing NULL as argument */
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, setenv(NULL, TEST_VALUE, 0));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(stdlib_env, putenv_invalid)
{
#ifndef __phoenix__
	TEST_ASSERT_EQUAL_INT(0, putenv(""));
#endif

/* Implementation of putenv in libphoenix checks if string argument contains '=' sign */
#ifdef __phoenix__
	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, putenv(""));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, putenv(TEST_VAR));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
#endif
}


TEST(stdlib_env, invalid)
{
	int i;
	for (i = 0; i < 2; i++) {
		errno = 0;
		TEST_ASSERT_EQUAL_INT(-1, setenv(INVALID_VAR1, INVALID_VAL1, i));
		TEST_ASSERT_EQUAL_INT(EINVAL, errno);

		errno = 0;
		TEST_ASSERT_EQUAL_INT(-1, setenv(INVALID_VAR2, INVALID_VAL2, i));
		TEST_ASSERT_EQUAL_INT(EINVAL, errno);

		errno = 0;
		TEST_ASSERT_EQUAL_INT(-1, setenv(INVALID_VAR3, INVALID_VAL3, i));
		TEST_ASSERT_EQUAL_INT(EINVAL, errno);

		errno = 0;
		TEST_ASSERT_EQUAL_INT(-1, unsetenv(INVALID_VAR1));
		TEST_ASSERT_EQUAL_INT(EINVAL, errno);

		errno = 0;
		TEST_ASSERT_EQUAL_INT(-1, unsetenv(INVALID_VAR2));
		TEST_ASSERT_EQUAL_INT(EINVAL, errno);

		errno = 0;
		TEST_ASSERT_EQUAL_INT(-1, unsetenv(INVALID_VAR3));
		TEST_ASSERT_EQUAL_INT(EINVAL, errno);
	}
}


TEST(stdlib_env, overwrite)
{
	TEST_ASSERT_EQUAL_INT(0, setenv(TEST_VAR, TEST_VALUE, 1));

	TEST_ASSERT_EQUAL_INT(0, setenv(TEST_VAR, TEST_VALUE2, 1));

	TEST_ASSERT_EQUAL_STRING(TEST_VALUE2, getenv(TEST_VAR));
}


TEST(stdlib_env, env_not_set)
{
	TEST_ASSERT_NULL(getenv(NOT_SET_VAR));
}

TEST(stdlib_env, environ_len)
{
	int len, len2;
	len = test_getEnvironLen(environ);
	TEST_ASSERT_EQUAL_INT(0, setenv("len_test", TEST_VALUE, 1));
	len2 = test_getEnvironLen(environ);
	TEST_ASSERT_EQUAL_INT(len + 1, len2);

	TEST_ASSERT_EQUAL_INT(0, unsetenv("len_test"));
	len2 = test_getEnvironLen(environ);
	TEST_ASSERT_EQUAL_INT(len, len2);
}


TEST_GROUP_RUNNER(stdlib_env)
{
	RUN_TEST_CASE(stdlib_env, clearenv);
	RUN_TEST_CASE(stdlib_env, basic);
	RUN_TEST_CASE(stdlib_env, long_args);
	RUN_TEST_CASE(stdlib_env, ascii);
	RUN_TEST_CASE(stdlib_env, invalid);
	RUN_TEST_CASE(stdlib_env, empty_name);
	RUN_TEST_CASE(stdlib_env, overwrite);
	RUN_TEST_CASE(stdlib_env, empty_value);
	RUN_TEST_CASE(stdlib_env, putenv_invalid);
	RUN_TEST_CASE(stdlib_env, env_not_set);
	RUN_TEST_CASE(stdlib_env, name_null);
	RUN_TEST_CASE(stdlib_env, environ_len);
	RUN_TEST_CASE(stdlib_env, random);
}
