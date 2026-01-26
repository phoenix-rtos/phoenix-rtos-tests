/*
 * Phoenix-RTOS
 *
 * Meterfs migration test
 *
 * Takes in a path to the meterfs partition image in pre-v1 meterfs format then
 * initializes v1 meterfs on it to provoke migration.
 *
 * The migration is tested by iterating over fault injection scenarios. In each
 * iteration, the migration is attempted first with parametrized injected
 * faults. If it succeeds, the iteration ends, otherwise a second migration
 * attempt is performed with no faults injected - this time the migration must
 * recover and succeed.
 *
 * Copyright 2026 Phoenix Systems
 * Author: Adam Greloch
 *
 * %LICENSE%
 */

#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <sys/wait.h>
#include <unistd.h>

#include <unity_fixture.h>

#include <host-flashsrv.h>

#include "file.h"

#if !defined(METERFS_DEBUG_UTILS) || !METERFS_DEBUG_UTILS
#error "METERFS_DEBUG_UTILS must be enabled for the migration test"
#endif

#define BUF_SIZE 4096

static struct {
	char *meterfsPath;
	char buf[BUF_SIZE];
} common;

#define EXIT_REBOOT_TRIGGER 42

#define WRITE_TRIGGER_MAX  64
#define WRITE_TRIGGER_STEP 16

#define REBOOT_TRIGGER_MAX  200
#define REBOOT_TRIGGER_STEP 1

#define TMPDIR_PATH  "/tmp"
#define TMPFILE_PATH (TMPDIR_PATH "/meterfs_migration_test")


TEST_GROUP(meterfs_migration);


TEST_SETUP(meterfs_migration)
{
	if (mkdir(TMPDIR_PATH, 0755) == -1) {
		if (errno != EEXIST) {
			FAIL("mkdir");
		}
	}
}


TEST_TEAR_DOWN(meterfs_migration)
{
}


static int copyFile(const char *path1, const char *path2)
{
	int inFd, outFd;

	inFd = open(path1, O_RDONLY);
	if (inFd < 0) {
		return -1;
	}

	struct stat st;
	if (fstat(inFd, &st) < 0) {
		close(inFd);
		return -1;
	}

	outFd = open(path2, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode);
	if (outFd < 0) {
		close(inFd);
		return -1;
	}

	off_t offs = 0;
	ssize_t sent;
	int err = 0;

	while (offs < st.st_size) {
		sent = sendfile(outFd, inFd, &offs, st.st_size - offs);
		if (sent <= 0) {
			err = -1;
			break;
		}
	}

	close(inFd);
	close(outFd);

	return err;
}


static void exitOnRebootTrigger(void)
{
	exit(EXIT_REBOOT_TRIGGER);
}


static void initMeterfs(int unreliableWriteTrigger, int rebootTrigger)
{
	meterfs_debugCtx_t debugCtx = {
		.rebootTrigger = rebootTrigger,
		.unreliableWriteTrigger = unreliableWriteTrigger,
		.dryErase = true,
		.onRebootCb = exitOnRebootTrigger,
	};

	hostflashsrv_setDebugCtx(&debugCtx);

	if (copyFile(common.meterfsPath, TMPFILE_PATH) != 0) {
		exit(EXIT_FAILURE);
	}

	if (file_init(TMPFILE_PATH) != 0) {
		exit(EXIT_FAILURE);
	}
}


static int fork_migrate(int unreliableWriteTrigger, int rebootTrigger)
{
	pid_t pid, wpid;
	int status;

	pid = fork();
	if (pid < 0) {
		FAIL("fork");
	}
	else if (pid == 0) {
		initMeterfs(unreliableWriteTrigger, rebootTrigger);
		exit(EXIT_SUCCESS);
	}
	else {
		wpid = wait(&status);
		if (wpid < 0) {
			FAIL("wait");
		}
		if (WIFEXITED(status)) {
			return WEXITSTATUS(status);
		}
		else {
			FAIL("unexpected wait status");
		}
	}
}


TEST(meterfs_migration, test_migration)
{
	for (int wt = 0; wt < WRITE_TRIGGER_MAX; wt += WRITE_TRIGGER_STEP) {
		for (int rt = 0; rt < REBOOT_TRIGGER_MAX; rt += REBOOT_TRIGGER_STEP) {
			int exit = fork_migrate(wt, rt);
			TEST_ASSERT_NOT_EQUAL(exit, EXIT_FAILURE);

			if (exit == EXIT_REBOOT_TRIGGER) {
				exit = fork_migrate(0, 0);
			}

			TEST_ASSERT_EQUAL(exit, EXIT_SUCCESS);
		}
	}
}


TEST_GROUP_RUNNER(meterfs_migration)
{
	RUN_TEST_CASE(meterfs_migration, test_migration);
}


void runner(void)
{
	RUN_TEST_GROUP(meterfs_migration);
}


int main(int argc, char *argv[])
{
	if (argc != 2) {
		(void)printf("Usage: %s METERFS_TO_MIGRATE_PATH\n", argv[0]);
		return 1;
	}

	common.meterfsPath = argv[1];

	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
