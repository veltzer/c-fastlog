#include <us_helper.h> // for micro_diff
#include <syslog.h> // for openlog(3), syslog(3), closelog(3)
#include <stdio.h> // for printf(3), fopen(3), fclose(3), fflush(3)
#include <sys/time.h> // for gettimeofday(2)
#include <assert.h> // for assert(3)
#include <pthread.h> // for pthread_mutex_t, pthread_mutex_lock, pthread_mutex_unlock
#include <stdarg.h> // for va_list, va_start, va_end
#include <sched_utils.h> // for sched_print_info, sched_print_table, sched_run_priority

#include <fastlog.h>

/*
 * This example explores syslog speed as compared to writing to a simple file.
 *
 * Notes:
 * - every messsage that I'm sending to syslog or to file has a number in it. This
 *	has two reasons: the most important is for syslog to actually log the message
 *	since if syslog gets two identical messages it does not log the second. Instead
 *	it waits for sometime and then says that the previous message repeated so and so
 *	times. The second reason is to include the printf like formatting code in the
 *	measurements.
 * - the tested code runs in a high priority thread to make sure that we measure times
 *	correctly.
 * - the fwrite implementation is fast because it does buffering. Maybe you are ok with
 *	that (you may lose data if you crash) and in that case you can use it.
 *
 * Results:
 * - by default you will find that syslog is much much slower than write.
 *	Output for example from this program:
 *	=====================================
 *	doing 1000 syslogs
 *	time in micro of one syslog: 55.897000
 *	doing 1000 writes
 *	time in micro of one write: 0.330000
 *	=====================================
 *	This is because rsyslogd in ubuntu is synchroneous. Most syslogd implementations are
 *	like that.
 *
 * TODO:
 * - add three more test cases: open(2), write(2), close(2) with
 *	- standard flags.
 *	- O_ASYNC
 *	- O_SYNC
 * - add another test with syslog which writes to a sysfs file instead.
 * - add another test case of asynchroneous syslog (damn it! how do I configure that?!?).
 * - explain the results in the text above.
 * - do better stats (min, max, variance and more).
 * - add some atomic operations to the fastlog checking (to emulate what will really be going on there).
 * - move the run_high_priority to my utils and do error checking in it. Print nice message to user
 *	about how to set right permissions if I cannot run in a high priority.
 *
 * 					Mark Veltzer
 * EXTRA_LIBS=-lpthread
 */

static void empty_noinline(const char* fmt,...) __attribute__((format(printf, 1, 2), noinline));
static void empty_noinline(const char* fmt,...) {
	// measures the cost of the call alone: the arguments are not read
	(void)fmt;
}

static void empty_inline(const char* fmt,...) __attribute__((format(printf, 1, 2)));
static void empty_inline(const char* fmt,...) {
	(void)fmt;
}

static void* func(void* arg) {
	// the signature is the thread library's: this thread takes no argument
	(void)arg;
	sched_print_info();
	printf("all time measurements are in micro seconds...\n");
	// the name of this app
	const char* myname="syslog_speed";
	// number of messages to measure
	const unsigned int number=10000;
	// timevals to store before and after time...
	struct timeval t1, t2;
	// name of the test currently running
	const char* test;
	// loop variable
	unsigned int i;

	test="standard syslog";
	openlog(myname, LOG_PID, LOG_USER);
	printf("doing %u syslogs\n",number);
	// start timing...
	gettimeofday(&t1, NULL);
	for (i = 0; i < number; i++) {
		syslog(LOG_ERR,"this is a message %u", i);
	}
	// end timing...
	gettimeofday(&t2, NULL);
	closelog();
	// print timing...
	printf("%s: %lf\n",test,micro_diff(&t1,&t2)/(double)number);
	// let io buffers be flushed...
	sleep(1);

	test="regular file operations (nonbuffreed, flushed, synchroneous)";
	FILE* f=fopen("/tmp/syslog_test","w+");
	assert(f!=NULL);
	printf("doing %u writes\n",number);
	// start timing...
	gettimeofday(&t1, NULL);
	for (i = 0; i < number; i++) {
		fprintf(f,"this is a message %u", i);
		fflush(f);
	}
	// end timing...
	gettimeofday(&t2, NULL);
	fclose(f);
	// print timing...
	printf("%s: %lf\n",test,micro_diff(&t1,&t2)/(double)number);
	// let io buffers be flushed...
	sleep(1);

	test="regular file operations (buffered, non flushed, non synchronized)";
	f=fopen("/tmp/syslog_test","w+");
	assert(f!=NULL);
	printf("doing %u writes\n",number);
	// start timing...
	gettimeofday(&t1, NULL);
	for (i = 0; i < number; i++) {
		fprintf(f,"this is a message %u", i);
	}
	// end timing...
	gettimeofday(&t2, NULL);
	fclose(f);
	// print timing...
	printf("%s: %lf\n",test,micro_diff(&t1,&t2)/(double)number);
	// let io buffers be flushed...
	sleep(1);

	// now lets measure how long it would take to memcpy...
	printf("doing %u fastlog_log\n",number);
	fastlog_init(NULL);
	// start timing...
	gettimeofday(&t1, NULL);
	for (i = 0; i < number; i++) {
		fastlog_log("this is a message %u", i);
	}
	// end timing...
	gettimeofday(&t2, NULL);
	// print timing...
	printf("time in micro of one fastlog_log: %lf\n", micro_diff(&t1,&t2)/(double)number);
	// let io buffers be flushed...
	fastlog_close();
	sleep(1);

	// now lets measure how long it would take to do nothing (with method call)...
	printf("doing %u empty (non-inlined) methods\n",number);
	// start timing...
	gettimeofday(&t1, NULL);
	for (i = 0; i < number; i++) {
		empty_noinline("this is a message %u", i);
	}
	// end timing...
	gettimeofday(&t2, NULL);
	// print timing...
	printf("time in micro of one empty (non-inlined) method: %lf\n", micro_diff(&t1,&t2)/(double)number);

	// now lets measure how long it would take to do nothing...
	printf("doing %u empty (inlined) methods\n",number);
	// start timing...
	gettimeofday(&t1, NULL);
	for (i = 0; i < number; i++) {
		empty_inline("this is a message %u", i);
	}
	// end timing...
	gettimeofday(&t2, NULL);
	// print timing...
	printf("time in micro of one empty (inlined) method: %lf\n", micro_diff(&t1,&t2)/(double)number);
	// let io buffers be flushed...
	sleep(1);

	return NULL;
}

int main(void) {
	//sched_print_table();
	sched_run_priority(func, NULL, SCHED_FIFO_HIGH_PRIORITY, SCHED_FIFO);
	return 0;
}
