/*
 * Phoenix-RTOS
 *
 * libc-tests
 *
 * Testing getuid functions form unistd.h
 *
 * Copyright 2022 Phoenix Systems
 * Author: Mateusz Niewiadomski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

#include <unity_fixture.h>


TEST_GROUP(unistd_uids);


TEST_SETUP(unistd_uids)
{
}


TEST_TEAR_DOWN(unistd_uids)
{
}

/*
	Testing correct return values for:
	getpid, getppid, getuid(), geteuid, 
	getpgid, getpgrp, getsid, getgid, getegid

	for self standing process
*/
TEST(unistd_uids, getuids_parent)
{
	pid_t pid = -1;

	TEST_ASSERT_GREATER_THAN_INT(0, getpid());
	TEST_ASSERT_GREATER_THAN_INT(0, getppid());

	pid = getpid();
	TEST_ASSERT_GREATER_THAN_INT(0, getpgid(pid));
	TEST_ASSERT_GREATER_THAN_INT(0, getpgrp());
	TEST_ASSERT_GREATER_THAN_INT(0, getsid(pid));

	/*
		tests are run as root, but following functions are
		declared but unimplemented in libphoenix.
		Reference: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/280
	*/
	TEST_IGNORE();
	TEST_ASSERT_GREATER_THAN_INT(0, getgid());
	TEST_ASSERT_GREATER_THAN_INT(0, getegid());
	TEST_ASSERT_EQUAL_INT(0, getuid());
	TEST_ASSERT_EQUAL_INT(0, geteuid());
}

TEST(unistd_uids, setuids_parent)
{
	/*
		setuid(), seteuid(), setgid(), setegid() are stub implemented so testing them
		is pointless, yet marking this in testing process should occur.
		Reference: https://github.com/phoenix-rtos/phoenix-rtos-project/issues/280
	*/
	TEST_IGNORE();
}

TEST(unistd_uids, setpuids_setsid)
{
	volatile pid_t child[6], parent[3], pid;
	int err = -1;
	volatile int sidret = -1;

	pid = parent[0] = parent[1] = parent[2] = -1;

	parent[0] = getpid();
	parent[1] = getpgrp();
	parent[2] = getsid(getpid());

	pid = vfork();
	if (pid == 0) {
		child[0] = getpid();
		child[1] = getpgrp();
		child[2] = getsid(getpid());

		sidret = setsid();

		child[3] = getpid();
		child[4] = getpgrp();
		child[5] = getsid(getpid());

		_exit(0);
	}
	else {
		waitpid(pid, &err, 0);
	}

	/* sid return value == sid */
	TEST_ASSERT_EQUAL_INT(sidret, child[5]);

	/* assert correctness of parent pid, group and session */
	TEST_ASSERT_GREATER_THAN_INT(0, parent[0]);  /* nonzero pid */
	TEST_ASSERT_GREATER_THAN_INT(0, parent[1]);  /* nonzero pgid */
	TEST_ASSERT_GREATER_THAN_INT(0, parent[2]);  /* nonzero sid */
	/* we don't check parent pgid against sid as we don't know who was parents session leader */

	/* assert correctness of child pid/group/session before setsid */
	TEST_ASSERT_NOT_EQUAL_INT(parent[0], child[0]); /* parent-child have different pid */
	TEST_ASSERT_EQUAL_INT(parent[1], child[1]);     /* equal pgrp */
	TEST_ASSERT_EQUAL_INT(parent[2], child[2]);     /* equal sid */

	/* after setsid the pid, group id and session id of child should be equal */
	TEST_ASSERT_NOT_EQUAL_INT(parent[0], child[3]); /* parent-child have different pid */
	TEST_ASSERT_EQUAL_INT(child[3], child[4]);      /* pid == pgrp */
	TEST_ASSERT_EQUAL_INT(child[4], child[5]);      /* pgrp == sid */
}

TEST(unistd_uids, setpuids_setpgid)
{
	/* 
		setpgid() changes child session for no reason.
		The same applies for set setpgrp()
		https://github.com/phoenix-rtos/phoenix-rtos-project/issues/282
	*/
	TEST_IGNORE();
	volatile pid_t child[6], parent[3], pid;
	int err = -1;
	volatile int pgrperr = -1;

	pid = parent[0] = parent[1] = parent[2] = -1;

	parent[0] = getpid();
	parent[1] = getpgrp();
	parent[2] = getsid(getpid());

	pid = vfork();
	if (pid == 0) {
		child[0] = getpid();
		child[1] = getpgrp();
		child[2] = getsid(getpid());

		pgrperr = setpgid(0, 0);

		child[3] = getpid();
		child[4] = getpgrp();
		child[5] = getsid(getpid());

		_exit(0);
	}
	else {
		waitpid(pid, &err, 0);
	}

	/* assert setpgid succeeded */
	TEST_ASSERT_EQUAL_INT(0, pgrperr);

	/* assert correctness of parent pid, group and session */
	TEST_ASSERT_GREATER_THAN_INT(0, parent[0]);  /* nonzero pid */
	TEST_ASSERT_GREATER_THAN_INT(0, parent[1]);  /* nonzero pgid */
	TEST_ASSERT_GREATER_THAN_INT(0, parent[2]);  /* nonzero sid */
	TEST_ASSERT_EQUAL_INT(parent[0], parent[1]); /* pid == pgid */
	TEST_ASSERT_EQUAL_INT(parent[1], parent[2]); /* pgid == sid */

	/* assert correctness of child pid/group/session before setpgid */
	TEST_ASSERT_NOT_EQUAL_INT(parent[0], child[0]); /* parent-child have different pid */
	TEST_ASSERT_EQUAL_INT(parent[1], child[1]);     /* equal pgrp */
	TEST_ASSERT_EQUAL_INT(parent[2], child[2]);     /* equal sid */

	/* after setpgid the pid, group id and session id of child should be equal */
	TEST_ASSERT_NOT_EQUAL_INT(parent[0], child[3]); /* parent-child have different pid */
	TEST_ASSERT_NOT_EQUAL_INT(parent[2], child[5]); /* parent-child have different sid */
	TEST_ASSERT_EQUAL_INT(child[3], child[4]);      /* pid == pgrp */
	TEST_ASSERT_NOT_EQUAL_INT(child[3], child[5]);  /* pid != sid */
}

TEST_GROUP_RUNNER(unistd_uids)
{
	RUN_TEST_CASE(unistd_uids, getuids_parent);
	RUN_TEST_CASE(unistd_uids, setuids_parent);
	RUN_TEST_CASE(unistd_uids, setpuids_setsid);
	RUN_TEST_CASE(unistd_uids, setpuids_setpgid)
}
