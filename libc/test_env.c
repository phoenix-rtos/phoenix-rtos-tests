/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * Testing setenv, getenv, unsetenv, clearenv functions
 *
 * Copyright 2023 Phoenix Systems
 * Author: Adam Debek
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
#define INVALID_VAL1 ("VA=LUE")
#define INVALID_VAL2 ("=VALUE")
#define INVALID_VAL3 ("VALUE=")
#define INVALID_VAR1 ("NAME=")
#define INVALID_VAR2 ("NA=ME")
#define INVALID_VAR3 ("=NAME")
#define NOT_SET_VAR  ("NOT_SET_VAR")

typedef struct {
	char name[MAX_NAME_LEN];
	char value[MAX_VALUE_LEN];
	char s[MAX_NAME_LEN + MAX_VALUE_LEN];
	unsigned set;
} env_var_t;

static env_var_t vars[NUM_OF_VARIABLES];

extern char **environ;

TEST_GROUP(test_env);

TEST_SETUP(test_env)
{
}

TEST_TEAR_DOWN(test_env)
{
}


TEST(test_env, test_env_random)
{
    int res;
	unsigned i, j, clearenv_cnt = 0, putenv_cnt = 0, setenv_cnt = 0, unsetenv_cnt = 0;

	srand(time(NULL));

	/* Initialize variables */
	for (i = 0; i < NUM_OF_VARIABLES; i++) {
		vars[i].set = 0;
		sprintf(vars[i].name, "VARIABLE%u", i);
		vars[i].value[0] = '\0';
	}

	/* Run tests */
	for (i = 0; i < NUM_OF_TESTS; i++) {

		if (rand() % CLEARENV_APPROX_EVERY == 0) {
			clearenv_cnt++;
			res = clearenv();
			TEST_ASSERT_EQUAL_INT(0, res);

			for (j = 0; j < NUM_OF_VARIABLES; j++) {
				vars[j].set = 0;
			}

		} else {
			unsigned idx = rand() % NUM_OF_VARIABLES;
			unsigned action = rand() % 3;
			if (action == 2) { /* unsetenv */
				unsetenv_cnt++;
				res = unsetenv(vars[idx].name);
				TEST_ASSERT_EQUAL_INT(0, res);

				vars[idx].set = 0;

			} else { /* insert (setenv or putenv) */

				/* Generate random value */
				unsigned len = rand() % MAX_VALUE_LEN;
				char new_value[MAX_VALUE_LEN];
				for (j = 0; j < len; j++) {
					char c;
					do {
						c = 33 + rand() % 94;
					} while (c == '\0' || c == '=');
					new_value[j] = c;
				}
				new_value[len] = '\0';

				if (action) { /* setenv */
					unsigned overwrite = rand() % 2;

					setenv_cnt++;
					res = setenv(vars[idx].name, new_value, overwrite);
					TEST_ASSERT_EQUAL_INT(0, res);

					if (!vars[idx].set || overwrite) strcpy(vars[idx].value, new_value);

				} else { /* putenv */
					strcpy(vars[idx].value, new_value);

					char *s = vars[idx].s;
					strcpy(s, vars[idx].name);
					s += strlen(vars[idx].name);
					*(s++) = '=';
					strcpy(s, vars[idx].value);

					putenv_cnt++;
					res = putenv(vars[idx].s);
					TEST_ASSERT_EQUAL_INT(0, res);
				}

				vars[idx].set = 1;
			}
		}

		/* Verify */
		for (j = 0; j < NUM_OF_VARIABLES; j++) {
			char *v = getenv(vars[j].name);
			if (vars[j].set) { /* var set check if it is correct */
				TEST_ASSERT_NOT_NULL(v);
				TEST_ASSERT_EQUAL_STRING(vars[j].value, v);
			} else { /* var not set must be null */
				TEST_ASSERT_NULL(v);
			}
		}
	}
}


TEST(test_env, test_empty_name)
{
	int res;
	res = setenv("", TEST_VALUE, 0);
	TEST_ASSERT_EQUAL_INT(-1, res);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	res = setenv("", TEST_VALUE, 1);
	TEST_ASSERT_EQUAL_INT(-1, res);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	char *v = getenv("");
	TEST_ASSERT_NULL(v);

	res = unsetenv("");
	TEST_ASSERT_EQUAL_INT(-1, res);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	res = unsetenv(NULL);
	TEST_ASSERT_EQUAL_INT(-1, res);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST(test_env, test_empty_value)
{
	int res;
	errno = 0;
	res = setenv(TEST_VAR, "", 0);
	TEST_ASSERT_EQUAL_INT(0, res);
	TEST_ASSERT_NOT_EQUAL_INT(EINVAL, errno);

	res = setenv(TEST_VAR, "", 1);
	TEST_ASSERT_EQUAL_INT(0, res);
	TEST_ASSERT_NOT_EQUAL_INT(EINVAL, errno);

#ifdef __phoenix__
	res = setenv(NULL, TEST_VALUE, 0);
	TEST_ASSERT_EQUAL_INT(-1, res);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

/* gives exception for now 
	res = setenv(TEST_VAR, NULL, 0);
	TEST_ASSERT_EQUAL_INT(-1, res);
	TEST_ASSERT_NOT_EQUAL_INT(EINVAL, errno); */
#endif
}


TEST(test_env, test_invalid_value)
{
	int res;
	int i;
	for (i = 0; i < 2; i++) {
	res = setenv(INVALID_VAR1, INVALID_VAL1, i);
	TEST_ASSERT_EQUAL_INT(-1, res);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	res = setenv(INVALID_VAR2, INVALID_VAL2, i);
	TEST_ASSERT_EQUAL_INT(-1, res);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	res = setenv(INVALID_VAR3, INVALID_VAL3, i);
	TEST_ASSERT_EQUAL_INT(-1, res);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	res = unsetenv(INVALID_VAR1);
	TEST_ASSERT_EQUAL_INT(-1, res);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	res = unsetenv(INVALID_VAR2);
	TEST_ASSERT_EQUAL_INT(-1, res);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	res = unsetenv(INVALID_VAR3);
	TEST_ASSERT_EQUAL_INT(-1, res);
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	}
}


TEST(test_env, test_overwrite)
{
	int res;
	res = setenv(TEST_VAR, TEST_VALUE, 1);
	TEST_ASSERT_EQUAL_INT(0, res);

	res = setenv(TEST_VAR, TEST_VALUE, 1);
	TEST_ASSERT_EQUAL_INT(0, res);

	char *v = getenv(TEST_VAR);
	TEST_ASSERT_EQUAL_STRING(TEST_VALUE, v);
}


TEST(test_env, test_get_not_set)
{
	char *v = getenv(NOT_SET_VAR);
	TEST_ASSERT_NULL(v);
}


TEST_GROUP_RUNNER(test_env)
{
    RUN_TEST_CASE(test_env, test_env_random);
	RUN_TEST_CASE(test_env, test_invalid_value);
	RUN_TEST_CASE(test_env, test_empty_name);
	RUN_TEST_CASE(test_env, test_empty_value);
	RUN_TEST_CASE(test_env, test_get_not_set);
}