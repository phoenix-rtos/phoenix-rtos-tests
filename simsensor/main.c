/*
 * Phoenix-Pilot
 *
 * Unit tests for simsensor functionality
 *
 * Copyright 2023 Phoenix Systems
 * Author: Piotr Nieciecki
 *
 * This file is part of Phoenix-Pilot software
 *
 * %LICENSE%
 */

#include <unity_fixture.h>


void runner(void)
{
	RUN_TEST_GROUP(group_event_queue);
	RUN_TEST_GROUP(group_sim_reader);
}


int main(int argc, char **argv)
{
	UnityMain(argc, (const char **)argv, runner);
	return 0;
}