/*
 * Phoenix-RTOS
 *
 * phoenix-rtos-tests
 *
 * Tests the cpu clocks and the user/system time split.
 *
 * Copyright 2026 Phoenix Systems
 * Author: Adam Greloch
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/threads.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/times.h>
#include <time.h>
#include <unistd.h>

#include "unity_fixture.h"


/* Declared by no header: the raw syscall behind times() and the cpu clocks */
int sys_cpuTime(pid_t pid, int tid, time_t *cpuTime, cpuTimes_t *cpuTimes);


#define WORK_CPU_MS 100

/* Short enough to not overshoot the deadline of a workload by much */
#define SPIN_CHUNK 4096

/* The syscall pair burns the time, not the mapping size */
#define MMAP_PAGES 2

/* Nothing of a sleep may be charged to the sleeper, bar the syscalls around it */
#define SLEEP_US        500000
#define SLEEP_CHARGE_US 20000

/* No thread has ever had this id, and no process this pid */
#define BOGUS_ID 0x4ffffff

#define WORKER_STACK_SZ 4096

/* Bounded, so a thread that never runs fails the test instead of hanging the runner */
#define WORKER_START_MS 1000


static struct {
	long usPerTick;
	size_t mmapSize;
	volatile unsigned long spinSink;
	char msg[192];
} common;


typedef struct {
	time_t utime;
	time_t stime;
} split_t;


/* The sink keeps the compiler from optimising the loop away */
static void spin(unsigned long iters)
{
	unsigned long i, acc = common.spinSink;

	for (i = 0; i < iters; i++) {
		acc = (acc * 1103515245UL) + 12345UL;
	}

	common.spinSink = acc;
}


static time_t monotonicUs(void)
{
	struct timespec ts;

	TEST_ASSERT_EQUAL_INT(0, clock_gettime(CLOCK_MONOTONIC, &ts));

	return ((time_t)ts.tv_sec * 1000000) + ((time_t)ts.tv_nsec / 1000);
}


static time_t clockUs(clockid_t clk)
{
	struct timespec ts;

	TEST_ASSERT_EQUAL_INT(0, clock_gettime(clk, &ts));

	return ((time_t)ts.tv_sec * 1000000) + ((time_t)ts.tv_nsec / 1000);
}


static time_t procCpuUs(void)
{
	time_t us;

	TEST_ASSERT_TRUE_MESSAGE(sys_cpuTime(0, 0, &us, NULL) >= 0, "sys_cpuTime(0, 0) failed");

	return us;
}


static void splitGet(split_t *s)
{
	cpuTimes_t ct;

	TEST_ASSERT_TRUE_MESSAGE(sys_cpuTime(0, 0, NULL, &ct) >= 0, "sys_cpuTime split failed");

	s->utime = ct.user;
	s->stime = ct.sys;
}


/* Priced against the cpu clock, so preemption cannot shorten a workload */
static void userWorkload(unsigned long ms)
{
	time_t deadline = procCpuUs() + ((time_t)ms * 1000);

	do {
		spin(SPIN_CHUNK);
	} while (procCpuUs() < deadline);
}


/* Anonymous, so no server thread is charged for the work */
static int kernelWorkloadStep(void)
{
	void *p = mmap(NULL, common.mmapSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (p == MAP_FAILED) {
		return -1;
	}

	return munmap(p, common.mmapSize);
}


static void kernelWorkload(unsigned long ms)
{
	time_t deadline = procCpuUs() + ((time_t)ms * 1000);

	do {
		TEST_ASSERT_EQUAL_INT(0, kernelWorkloadStep());
	} while (procCpuUs() < deadline);
}


/*
 * Both clocks count the same time, so a value read between two reads of the other
 * must fall between them, however the threads were scheduled.
 */
static void assertBetween(time_t low, time_t mid, time_t high, const char *what)
{
	(void)snprintf(common.msg, sizeof(common.msg), "%s: %lld us, expected between %lld and %lld us",
			what, (long long)mid, (long long)low, (long long)high);
	TEST_ASSERT_TRUE_MESSAGE((mid >= low) && (mid <= high), common.msg);
}


TEST_GROUP(cputime);


TEST_SETUP(cputime) { }


TEST_TEAR_DOWN(cputime) { }


TEST(cputime, halves_add_up_to_the_process_cpu_clock)
{
	cpuTimes_t ct;
	time_t cpu;

	userWorkload(WORK_CPU_MS / 2);
	kernelWorkload(WORK_CPU_MS / 2);

	/* One call is one snapshot, so the sum has to match exactly */
	TEST_ASSERT_TRUE_MESSAGE(sys_cpuTime(0, 0, &cpu, &ct) >= 0, "sys_cpuTime failed");

	(void)snprintf(common.msg, sizeof(common.msg), "user %lld + system %lld us against the process clock %lld us",
			(long long)ct.user, (long long)ct.sys, (long long)cpu);
	TEST_ASSERT_TRUE_MESSAGE((ct.user + ct.sys) == cpu, common.msg);
}


TEST(cputime, both_halves_advance_under_work_of_their_kind)
{
	split_t before, after;

	splitGet(&before);
	userWorkload(WORK_CPU_MS);
	splitGet(&after);
	TEST_ASSERT_TRUE_MESSAGE(after.utime > before.utime, "user time did not grow in user space");

	splitGet(&before);
	kernelWorkload(WORK_CPU_MS);
	splitGet(&after);
	TEST_ASSERT_TRUE_MESSAGE(after.stime > before.stime, "system time did not grow over syscalls");
}


TEST(cputime, neither_half_ever_goes_backwards)
{
	split_t prev, now;
	int i;

	splitGet(&prev);

	for (i = 0; i < 8; i++) {
		userWorkload(WORK_CPU_MS / 8);
		kernelWorkload(WORK_CPU_MS / 8);

		splitGet(&now);
		TEST_ASSERT_TRUE_MESSAGE(now.utime >= prev.utime, "user time went backwards");
		TEST_ASSERT_TRUE_MESSAGE(now.stime >= prev.stime, "system time went backwards");
		prev = now;
	}
}


TEST(cputime, sleeping_is_charged_to_nobody)
{
	split_t before, after;

	splitGet(&before);
	(void)usleep(SLEEP_US);
	splitGet(&after);

	TEST_ASSERT_TRUE((after.utime - before.utime) < SLEEP_CHARGE_US);
	TEST_ASSERT_TRUE((after.stime - before.stime) < SLEEP_CHARGE_US);
}


TEST(cputime, times_agrees_with_the_microsecond_split)
{
	split_t us;
	struct tms tms;
	clock_t wall;
	time_t uticks, sticks;

	userWorkload(WORK_CPU_MS / 2);
	kernelWorkload(WORK_CPU_MS / 2);

	splitGet(&us);
	wall = times(&tms);
	TEST_ASSERT_TRUE(wall != (clock_t)-1);

	/* Compared as time_t, as clock_t may be unsigned and the tolerance reaches below zero */
	uticks = us.utime / common.usPerTick;
	sticks = us.stime / common.usPerTick;

	/* times() floors, and both halves grow between the two calls - one tick either way */
	TEST_ASSERT_TRUE((time_t)tms.tms_utime >= (uticks - 1));
	TEST_ASSERT_TRUE((time_t)tms.tms_utime <= (uticks + 1));
	TEST_ASSERT_TRUE((time_t)tms.tms_stime >= (sticks - 1));
	TEST_ASSERT_TRUE((time_t)tms.tms_stime <= (sticks + 1));

	/* The return value is elapsed real time, which cpu time cannot exceed */
	TEST_ASSERT_TRUE(wall >= tms.tms_utime + tms.tms_stime);
}


TEST_GROUP_RUNNER(cputime)
{
	RUN_TEST_CASE(cputime, halves_add_up_to_the_process_cpu_clock);
	RUN_TEST_CASE(cputime, both_halves_advance_under_work_of_their_kind);
	RUN_TEST_CASE(cputime, neither_half_ever_goes_backwards);
	RUN_TEST_CASE(cputime, sleeping_is_charged_to_nobody);
	RUN_TEST_CASE(cputime, times_agrees_with_the_microsecond_split);
}


TEST_GROUP(cpuclock);


TEST_SETUP(cpuclock) { }


TEST_TEAR_DOWN(cpuclock) { }


TEST(cpuclock, thread_clock_fits_inside_the_process_clock)
{
	time_t before, thread, after;

	userWorkload(WORK_CPU_MS);

	before = procCpuUs();
	TEST_ASSERT_TRUE_MESSAGE(sys_cpuTime(0, gettid(), &thread, NULL) >= 0, "sys_cpuTime by tid failed");
	after = procCpuUs();

	assertBetween(before, thread, after, "clock of the only thread against the process clock");
}


TEST(cpuclock, the_process_can_be_named_by_pid)
{
	time_t before, byPid, after;

	userWorkload(WORK_CPU_MS / 4);

	before = procCpuUs();
	TEST_ASSERT_TRUE_MESSAGE(sys_cpuTime(getpid(), 0, &byPid, NULL) >= 0, "sys_cpuTime by pid failed");
	after = procCpuUs();

	assertBetween(before, byPid, after, "process clock read by pid");
}


TEST(cpuclock, posix_clocks_read_the_same_cpu_time)
{
	time_t before, posix, after;

	userWorkload(WORK_CPU_MS / 4);

	before = procCpuUs();
	posix = clockUs(CLOCK_PROCESS_CPUTIME_ID);
	after = procCpuUs();
	assertBetween(before, posix, after, "CLOCK_PROCESS_CPUTIME_ID against the process clock");

	TEST_ASSERT_TRUE(sys_cpuTime(0, gettid(), &before, NULL) >= 0);
	posix = clockUs(CLOCK_THREAD_CPUTIME_ID);
	TEST_ASSERT_TRUE(sys_cpuTime(0, gettid(), &after, NULL) >= 0);
	assertBetween(before, posix, after, "CLOCK_THREAD_CPUTIME_ID against the thread clock");
}


TEST(cpuclock, clock_getcpuclockid_names_the_process_clock)
{
	time_t before, byId, after;
	clockid_t id;

	userWorkload(WORK_CPU_MS / 4);

	TEST_ASSERT_EQUAL_INT(0, clock_getcpuclockid(getpid(), &id));

	before = procCpuUs();
	byId = clockUs(id);
	after = procCpuUs();

	assertBetween(before, byId, after, "clock read through clock_getcpuclockid()");
}


TEST(cpuclock, the_clock_advances_but_never_outruns_the_wall_clock)
{
	time_t cpuBefore, cpuAfter, wallBefore, wallAfter;

	wallBefore = monotonicUs();
	cpuBefore = procCpuUs();

	userWorkload(WORK_CPU_MS);

	cpuAfter = procCpuUs();
	wallAfter = monotonicUs();

	TEST_ASSERT_TRUE(cpuAfter > cpuBefore);
	TEST_ASSERT_TRUE((cpuAfter - cpuBefore) <= (wallAfter - wallBefore));
}


TEST(cpuclock, clocks_of_nothing_are_refused)
{
	time_t us;
	cpuTimes_t ct;

	/* A thread is named by tid alone - a pid alongside it is an error, not a filter */
	TEST_ASSERT_EQUAL_INT(-EINVAL, sys_cpuTime(getpid(), gettid(), &us, NULL));

	/* Nothing asked for is an error too */
	TEST_ASSERT_EQUAL_INT(-EINVAL, sys_cpuTime(0, 0, NULL, NULL));

	TEST_ASSERT_EQUAL_INT(-ESRCH, sys_cpuTime(0, BOGUS_ID, &us, NULL));
	TEST_ASSERT_EQUAL_INT(-ESRCH, sys_cpuTime(BOGUS_ID, 0, &us, NULL));
	TEST_ASSERT_EQUAL_INT(-ESRCH, sys_cpuTime(BOGUS_ID, 0, NULL, &ct));
}


TEST(cpuclock, clock_ids_out_of_range_are_refused)
{
	struct timespec ts;

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, clock_gettime((clockid_t)-1, &ts));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);

	errno = 0;
	TEST_ASSERT_EQUAL_INT(-1, clock_getres((clockid_t)-1, &ts));
	TEST_ASSERT_EQUAL_INT(EINVAL, errno);
}


TEST_GROUP_RUNNER(cpuclock)
{
	RUN_TEST_CASE(cpuclock, thread_clock_fits_inside_the_process_clock);
	RUN_TEST_CASE(cpuclock, the_process_can_be_named_by_pid);
	RUN_TEST_CASE(cpuclock, posix_clocks_read_the_same_cpu_time);
	RUN_TEST_CASE(cpuclock, clock_getcpuclockid_names_the_process_clock);
	RUN_TEST_CASE(cpuclock, the_clock_advances_but_never_outruns_the_wall_clock);
	RUN_TEST_CASE(cpuclock, clocks_of_nothing_are_refused);
	RUN_TEST_CASE(cpuclock, clock_ids_out_of_range_are_refused);
}


static struct {
	volatile int run;
	volatile int tid;
	volatile int err;
	char stack[WORKER_STACK_SZ] __attribute__((aligned(8)));
} worker;


static void workerThread(void *arg)
{
	(void)arg;

	worker.tid = gettid();

	while (worker.run != 0) {
		if (kernelWorkloadStep() != 0) {
			worker.err = -1;
			break;
		}
	}

	endthread();
}


/* threadinfo gives the total and the system half, not the user one */
static void threadTimeGet(int tid, time_t *cpuUs, time_t *sysUs)
{
	threadinfo_t info;

	TEST_ASSERT_TRUE(threadinfo(tid, PH_THREADINFO_BASIC, &info) >= 1);

	*cpuUs = (time_t)info.cpuTime;
	*sysUs = (time_t)info.sysTime;
}


TEST_GROUP(cputime_thread);


TEST_SETUP(cputime_thread)
{
	worker.run = 1;
	worker.tid = 0;
	worker.err = 0;
}


TEST_TEAR_DOWN(cputime_thread) { }


static void workerStart(handle_t *handle)
{
	int ms;

	TEST_ASSERT_TRUE(beginthreadex(workerThread, 4, worker.stack, sizeof(worker.stack), NULL, handle) >= 0);

	for (ms = 0; worker.tid == 0; ms++) {
		TEST_ASSERT_TRUE_MESSAGE(ms < WORKER_START_MS, "the worker thread never started");
		(void)usleep(1000);
	}
}


static void workerStop(handle_t handle)
{
	worker.run = 0;
	TEST_ASSERT_TRUE(threadJoin(handle, 0) >= 0);
	TEST_ASSERT_EQUAL_INT(0, worker.err);
}


TEST(cputime_thread, the_system_half_is_a_part_of_each_thread_clock)
{
	time_t selfCpu, selfSys, workerCpu[2], workerSys[2];
	handle_t workerHandle;

	workerStart(&workerHandle);

	threadTimeGet(worker.tid, &workerCpu[0], &workerSys[0]);

	userWorkload(WORK_CPU_MS);

	threadTimeGet(gettid(), &selfCpu, &selfSys);
	threadTimeGet(worker.tid, &workerCpu[1], &workerSys[1]);

	workerStop(workerHandle);

	TEST_ASSERT_TRUE(selfSys <= selfCpu);
	TEST_ASSERT_TRUE(workerSys[1] <= workerCpu[1]);

	/* The worker does nothing but syscalls, so its system half has to move */
	TEST_ASSERT_TRUE_MESSAGE(workerSys[1] > workerSys[0], "system time of the mmap thread did not grow");
}


TEST(cputime_thread, the_process_clock_is_the_sum_of_its_threads)
{
	time_t before, after, self, workerCpu, goneLo, goneHi, sysUnused;
	int selfTid = gettid();
	handle_t workerHandle;

	/*
	 * Exited threads keep their time in the process clock, so price them in while
	 * this thread is the only other term of the sum.
	 */
	before = procCpuUs();
	threadTimeGet(selfTid, &self, &sysUnused);
	after = procCpuUs();

	goneLo = before - self;
	goneHi = after - self;

	workerStart(&workerHandle);

	userWorkload(WORK_CPU_MS);

	/* Both threads keep running, so the sum can only land between the two process reads */
	before = procCpuUs();
	threadTimeGet(selfTid, &self, &sysUnused);
	threadTimeGet(worker.tid, &workerCpu, &sysUnused);
	after = procCpuUs();

	workerStop(workerHandle);

	assertBetween(before - goneHi, self + workerCpu, after - goneLo,
			"sum of the thread clocks against the process clock");
}


TEST_GROUP_RUNNER(cputime_thread)
{
	RUN_TEST_CASE(cputime_thread, the_system_half_is_a_part_of_each_thread_clock);
	RUN_TEST_CASE(cputime_thread, the_process_clock_is_the_sum_of_its_threads);
}


void runner(void)
{
	RUN_TEST_GROUP(cputime);
	RUN_TEST_GROUP(cpuclock);
	RUN_TEST_GROUP(cputime_thread);
}


int main(int argc, char *argv[])
{
	common.usPerTick = 1000000L / sysconf(_SC_CLK_TCK);
	common.mmapSize = (size_t)(MMAP_PAGES * sysconf(_SC_PAGESIZE));

	return (UnityMain(argc, (const char **)argv, runner) == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
