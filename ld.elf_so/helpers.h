/*
 * Phoenix-RTOS
 *
 * ld.elf_so tests
 *
 * Helper functions
 *
 * Copyright 2024 by Phoenix Systems
 * Author: Hubert Badocha
 *
 * This file is a part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _TEST_LD_ELF_SO_HELPER_
#define _TEST_LD_ELF_SO_HELPER_


#include <stdio.h>
#include <unity_fixture.h>

#define	TEST_ASSERT_DL(x) do { if ((x) == NULL) { char *res; asprintf(&res, "%s: %s", #x, dlerror()); TEST_FAIL_MESSAGE(res);} } while (0)

#define TEST_ASSERT_MSGF(x, msg, ...) do { if (!(x)) { char *res; asprintf(&res, msg, ##__VA_ARGS__); TEST_FAIL_MESSAGE(res);} } while (0)
#define	TEST_ASSERT_EQ_MSGF(x, y, msg, ...) do { if ((x) != (y)) { char *res; asprintf(&res, msg, ##__VA_ARGS__); TEST_FAIL_MESSAGE(res);} } while (0)

#endif
