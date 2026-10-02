/*
 * Phoenix-RTOS
 *
 * phoenix-rtos-tests
 *
 * Testing the user/system cpu time split and the cpu clocks built on top of it.
 *
 * The split is exact, so it is read in microseconds through sys_cpuTime() rather than
 * times(), which floors to CLK_TCK ticks. Clocks are compared by reading one between
 * two reads of the other, which holds whatever the scheduler does in between.
 *
 * Copyright 2026 Phoenix Systems
 * Author: Adam Greloch
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include <errno.h>
#include <limits.h>
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


#define WORK_CPU_MS 200

#define CALIB_CPU_MS 50

/* The syscall pair burns the time, not the mapping size */
#define MMAP_PAGES 2

/* A microsecond split has no flooring, so a share cannot exceed the whole */
#define SHARE_MAX_PCT 100

/*
 * Bare syscalls are over 90% system time, but the timestamp taken at each boundary
 * lands on one side of the split, and a dear read pulls the share towards 60%.
 */
#define SYS_SHARE_MIN_PCT 50

/* Nothing of a sleep may be charged to the sleeper, bar the syscalls around it */
#define SLEEP_US        500000
#define SLEEP_CHARGE_US 20000

/* No thread has ever had this id, and no process this pid */
#define BOGUS_ID 0x4ffffff

#define WORKER_STACK_SZ 4096


static struct {
	unsigned long spinIterPerMs;
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
static void spin(unsigned long long iters)
{
	unsigned long long i;
	unsigned long acc = common.spinSink;

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


static void userWorkload(unsigned long ms)
{
	spin((unsigned long long)ms * common.spinIterPerMs);
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
	int i;

	do {
		for (i = 0; i < 16; i++) {
			TEST_ASSERT_EQUAL_INT(0, kernelWorkloadStep());
		}
	} while (procCpuUs() < deadline);
}


static void assertShare(time_t part, time_t whole, long minPct, long maxPct, const char *what)
{
	long long pct;

	(void)snprintf(common.msg, sizeof(common.msg), "%s: nothing to divide by (whole = %lld)",
			what, (long long)whole);
	TEST_ASSERT_TRUE_MESSAGE(whole > 0, common.msg);

	pct = ((long long)part * 100) / (long long)whole;

	(void)snprintf(common.msg, sizeof(common.msg), "%s: %lld%% (%lld of %lld us), expected %ld%%-%ld%%",
			what, pct, (long long)part, (long long)whole, minPct, maxPct);
	TEST_ASSERT_TRUE_MESSAGE((pct >= (long long)minPct) && (pct <= (long long)maxPct), common.msg);
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


/* Priced against the cpu clock, so preemption cannot shorten a workload */
static void calibrate(void)
{
	time_t start, elapsed;
	unsigned long iters = 10000;

	if (common.spinIterPerMs != 0UL) {
		return;
	}

	for (;;) {
		start = procCpuUs();
		spin(iters);
		elapsed = procCpuUs() - start;

		if (elapsed >= ((time_t)CALIB_CPU_MS * 1000)) {
			break;
		}

		/* Nothing sensible left to measure if the loop is this cheap */
		TEST_ASSERT_TRUE(iters < (ULONG_MAX / 4UL));
		iters *= 2UL;
	}

	common.spinIterPerMs = (unsigned long)(((unsigned long long)iters * 1000ULL) / (unsigned long long)elapsed);
	TEST_ASSERT_TRUE(common.spinIterPerMs > 0UL);
}


TEST_GROUP(cputime);


TEST_SETUP(cputime)
{
	calibrate();
}


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


TEST(cputime, user_work_is_charged_to_user_time)
{
	split_t before, after;

	splitGet(&before);
	userWorkload(WORK_CPU_MS);
	splitGet(&after);

	assertShare(after.utime - before.utime, (after.utime - before.utime) + (after.stime - before.stime),
			90, SHARE_MAX_PCT, "user share of a pure user space workload");
}


TEST(cputime, kernel_work_is_charged_to_system_time)
{
	split_t before, after;

	splitGet(&before);
	kernelWorkload(WORK_CPU_MS);
	splitGet(&after);

	assertShare(after.stime - before.stime, (after.utime - before.utime) + (after.stime - before.stime),
			SYS_SHARE_MIN_PCT, SHARE_MAX_PCT, "system share of an mmap()/munmap() workload");
}


TEST(cputime, sleeping_is_charged_to_nobody)
{
	split_t before, after;

	/* Burn user time first, so a stray charge would visibly move the system half */
	userWorkload(WORK_CPU_MS);

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

	userWorkload(WORK_CPU_MS / 2);
	kernelWorkload(WORK_CPU_MS / 2);

	splitGet(&us);
	wall = times(&tms);
	TEST_ASSERT_TRUE(wall != (clock_t)-1);

	/* times() floors, and both halves grow between the two calls - one tick either way */
	TEST_ASSERT_TRUE(tms.tms_utime >= (clock_t)((us.utime / common.usPerTick) - 1));
	TEST_ASSERT_TRUE(tms.tms_utime <= (clock_t)((us.utime / common.usPerTick) + 1));
	TEST_ASSERT_TRUE(tms.tms_stime >= (clock_t)((us.stime / common.usPerTick) - 1));
	TEST_ASSERT_TRUE(tms.tms_stime <= (clock_t)((us.stime / common.usPerTick) + 1));

	/* The return value is elapsed real time, which cpu time cannot exceed */
	TEST_ASSERT_TRUE(wall >= tms.tms_utime + tms.tms_stime);
}


TEST_GROUP_RUNNER(cputime)
{
	RUN_TEST_CASE(cputime, halves_add_up_to_the_process_cpu_clock);
	RUN_TEST_CASE(cputime, user_work_is_charged_to_user_time);
	RUN_TEST_CASE(cputime, kernel_work_is_charged_to_system_time);
	RUN_TEST_CASE(cputime, sleeping_is_charged_to_nobody);
	RUN_TEST_CASE(cputime, times_agrees_with_the_microsecond_split);
}


TEST_GROUP(cpuclock);


TEST_SETUP(cpuclock)
{
	calibrate();
}


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
	calibrate();

	worker.run = 1;
	worker.tid = 0;
	worker.err = 0;
}


TEST_TEAR_DOWN(cputime_thread) { }


static void workerStart(handle_t *handle)
{
	TEST_ASSERT_TRUE(beginthreadex(workerThread, 4, worker.stack, sizeof(worker.stack), NULL, handle) >= 0);

	while (worker.tid == 0) {
		(void)usleep(1000);
	}
}


static void workerStop(handle_t handle)
{
	worker.run = 0;
	TEST_ASSERT_TRUE(threadJoin((int)handle, 0) >= 0);
	TEST_ASSERT_EQUAL_INT(0, worker.err);
}


TEST(cputime_thread, split_is_tracked_per_thread)
{
	time_t selfCpu[2], selfSys[2], workerCpu[2], workerSys[2];
	int selfTid = gettid();
	handle_t workerHandle;

	workerStart(&workerHandle);

	threadTimeGet(selfTid, &selfCpu[0], &selfSys[0]);
	threadTimeGet(worker.tid, &workerCpu[0], &workerSys[0]);

	userWorkload(WORK_CPU_MS);

	threadTimeGet(selfTid, &selfCpu[1], &selfSys[1]);
	threadTimeGet(worker.tid, &workerCpu[1], &workerSys[1]);

	workerStop(workerHandle);

	assertShare(selfSys[1] - selfSys[0], selfCpu[1] - selfCpu[0], 0, 10, "system share of the spinning thread");
	assertShare(workerSys[1] - workerSys[0], workerCpu[1] - workerCpu[0], SYS_SHARE_MIN_PCT,
			SHARE_MAX_PCT, "system share of the mmap thread");
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
	RUN_TEST_CASE(cputime_thread, split_is_tracked_per_thread);
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
