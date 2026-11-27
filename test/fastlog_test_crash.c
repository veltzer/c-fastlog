#include <fastlog.h> // for the fastlog_* functions
#include <sys/types.h> // for kill(2)
#include <signal.h> // for kill(2)
#include <unistd.h> // for pause(2)
#include <sys/time.h> // for getrlimit(2), setrlimit(2)
#include <sys/resource.h> // for getrlimit(2), setrlimit(2)

/*
 * This is an application that test what happens when fastlog crashes.
 * The idea is to be able to retrieve the log from the core file.
 */

int main(void) {
	// lets check that we can dump core
	struct rlimit limit;
	int ret=getrlimit(RLIMIT_CORE,&limit);
	if(ret==-1) {
		perror("getrlimit");
		return -1;
	}
	printf("core size is cur=%ld, max=%ld\n",limit.rlim_cur,limit.rlim_max);
	// lets set the limit to some good value
	limit.rlim_cur=50000;
	ret=setrlimit(RLIMIT_CORE,&limit);
	if(ret==-1) {
		perror("setrlimit");
		return -1;
	}
	printf("set rlimit for core\n");
	fastlog_init(NULL);
	for(unsigned int i=0;i<100;i++) {
		fastlog_log("did you know that %d+%d=%d",i,i+1,i+i+1);
	}
	fastlog_close();
	// lets kill ourselves
	ret=kill(0,SIGSEGV);
	if(ret==-1) {
		perror("kill");
		return -1;
	}
	// lets wait for the signal to arrive
	pause();
	return 0;
}
