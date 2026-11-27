#include <fastlog.h>

int main(void) {
	fastlog_config_t conf;
	if(fastlog_config_init(&conf)!=0) {
		return 1;
	}
	fastlog_init(&conf);
	for(unsigned int i=0;i<100;i++) {
		fastlog_log("did you know that %d+%d=%d",i,i+1,i+i+1);
	}
	fastlog_close();
	fastlog_config_destroy(&conf);
	return 0;
}
