/*
 * Phoenix-RTOS
 *
 * ld.elf_so tests
 *
 * Exec assert lib
 *
 * Copyright 2024 by Phoenix Systems
 * Author: Hubert Badocha
 *
 * This file is a part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#ifndef _TEST_LD_ELF_SO_EXEC_ASSERT_
#define _TEST_LD_ELF_SO_EXEC_ASSERT_


void execAssert_execve(const char *path, char *const argv[], char *const envp[], const int *code, const char **out, const char **err);


#endif
