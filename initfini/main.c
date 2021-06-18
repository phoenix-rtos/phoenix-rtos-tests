/*
 * Phoenix-RTOS
 *
 * phoenix-rtos-tests
 *
 * Copyright 2021 Phoenix Systems
 * Author: Hubert Buczynski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */


#include <stdio.h>


__attribute__((constructor(101))) static void test_constr0(void)
{
	printf("Constructor 0\n");
}


__attribute__((constructor(102))) static void test_constr1(void)
{
	printf("Constructor 1\n");
}


int main(int argc, char **argv)
{
	printf("Main function\n");
	return 0;
}


__attribute__((destructor(101))) static void test_destr0(void)
{
	printf("Destructor 0\n");
}


__attribute__((destructor(102))) static void test_destr1(void)
{
	printf("Destructor 1\n");
}
