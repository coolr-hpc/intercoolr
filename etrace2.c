/*
  An enery trace code 

  support spawn mode(e.g. strace) and polling mode that priodically 
  print power usage.

  currently only support RAPL
  
  Kazutomo Yoshii <kazutomo@mcs.anl.gov>
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <getopt.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "raplreader.h"

#define Z printf("%d\n",__LINE__)

#define ETRACE2_VERSION "0.1"

static char *program_name;

char *_basename(char *path)
{
	char *base = strrchr(path, '/');
	return base ? base+1 : path;
}

/*
  XXX: clean up this later
*/
#define LOCKFN  "/var/tmp/etrace.lock"
static void  touch_lock(void) 
{
	int hackfd;
	hackfd = open(LOCKFN, O_WRONLY|O_CREAT|O_NOCTTY|O_NONBLOCK, 0666);
	close(hackfd);
}

static int access_lock(void)
{
	return access(LOCKFN, R_OK);
}

static void unlink_lock(void)
{
	unlink(LOCKFN);
}

static void sigh(int signum) 
{
	unlink(LOCKFN);
	exit(1);
}

static double gettimesec(void)
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return (double)tv.tv_sec + (double)tv.tv_usec/1000.0/1000.0;
}

/*
  if child_pid > 0, it doesn't use timeout and waits until the child process is terminated
*/
static void  sampling_loop(FILE *fp, double interval, double timeout, int child_pid, int verbose, int relative_time)
{
	double ts, time_start, time_delta;
	struct raplreader rr;
	useconds_t usec;
	int status;
	int nsockets;
	int rc;
	int i;

	fprintf(fp, "# ETRACE2_VERSION=%s\n",ETRACE2_VERSION);

	rc = access_lock();
	if( rc == 0 ) {
		fprintf(stderr, "\n");
		fprintf(stderr, "Failed to start etrace!\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "Someone else is running etrace. RAPL is a sytem-wide resource.\n");
		fprintf(stderr, "\n");
		exit(1);
	}

	touch_lock();
	signal(SIGINT, sigh);

	rc = raplreader_init(&rr);
	if (rc) {
		fprintf(stderr, "Error: raplreader_init() failed\n");
		exit(1);
	}

	nsockets = raplreader_get_nsockets(&rr);

	raplreader_sample(&rr);
	ts = raplreader_get_ts(&rr);
	time_start = ts;

	if (interval <= 0.0) {
		/* just energy usage */
		if (child_pid > 0) {
			waitpid(child_pid, &status, 0);
		} else {
			usleep(timeout*1000.0*1000.0);
		}
	} else {

		if (relative_time)  {
			fprintf(fp, "# START_TIME=%lf\n",gettimesec());
		}

		/* header */
		fprintf(fp, "# Time [S]   Power [W]   Energy [J]");
		fprintf(fp, "\n");
		fflush(fp);

		/* polling loop */
		for (;;) {
			time_delta = fmod(ts, interval);
			usec = (useconds_t)((interval - time_delta)*1000.0*1000.0);
			usleep(usec);
		
			raplreader_sample(&rr);
			ts = raplreader_get_ts(&rr);

			if (child_pid > 0) {
				if (waitpid(child_pid, &status, WNOHANG) != 0) 
					break;
			} else  if ((ts - time_start) > timeout) {
				break;
			}

			if (relative_time) 
				fprintf(fp, "%.3lf  ", ts - time_start);
			else 
				fprintf(fp, "%.3lf  ", ts);

			fprintf(fp, "%.3lf  ", raplreader_get_total_power(&rr));
			fprintf(fp, "%.3lf  ", raplreader_get_total_energy(&rr));
			fprintf(fp, "\n");
			fflush(fp);
		}
	}

	raplreader_sample(&rr);
	ts = raplreader_get_ts(&rr);

	fprintf(fp, "# ELAPSED=%lf\n", ts - time_start);
	fprintf(fp, "# ENERGY=%lf\n", raplreader_get_total_energy(&rr));
	for (i = 0; i < raplreader_get_nsockets(&rr); i++) {
		fprintf(fp, "# ENERGY_SOCKET%d=%lf\n", i, raplreader_get_energy_socket(&rr,i));
		if (raplreader_is_dram_available(&rr))
			fprintf(fp, "# ENERGY_DRAM%d=%lf\n", i, raplreader_get_energy_dram(&rr,i));
	}
}

static void usage(void)
{
	printf("\n");
	printf("Usage: %s [options] [command]\n",program_name);
	printf("\n");
	printf("[Options]\n");
	printf("\t-i sec : interval\n");
	printf("\t-t sec : timeout\n");
	printf("\t-o filename : output filename\n");
	printf("\t-r : relative time, instead of epoch\n");
	printf("\n");
#if 0
	printf("\t--setplim1 watt : limit power\n");
	printf("\t--setplim2 watt : limit power (turbo)\n");
	printf("\n");
#endif
	printf("[usage examples]\n");
	printf("\n");
	printf("# spawn your program and print energy consumption \n");
	printf("$ ./%s your_program\n",program_name);
	printf("\n");
	printf("# also print power consumption every 0.5 sec\n");
	printf("$ ./%s -i 0.5  your_program\n",program_name);
	printf("\n");
	printf("# just print power consumption every 1sec for 10sec\n");
	printf("$ ./%s -i 1.0 -t 10\n",program_name);
	printf("\n");
}

int main(int argc, char *argv[])
{
	int opt;
	int verbose=0;
	int relative_time=0;
	double timeout=-1.0, interval=-1.0;
	char fn[1024];
	FILE *fp = stdout;
	// double plim1=-1.0, plim2=-1.0;

	fn[0] = 0;

	program_name = _basename(argv[0]);
	while(1) {
		int option_index = 0;
		static struct option long_options[] = {
#if 0
			// name, has_arg, flag, val
			{"setplim1", 1, 0,  0 },
			{"setplim2", 1, 0,  0 },
#endif
			{0,          0, 0,  0 }
		};

		opt=getopt_long(argc, argv, "+i:t:o:rhv",
				long_options, &option_index);
		if(opt==-1) break;
					       
		switch(opt) {
#if 0
		case 0:
			switch( option_index ) {
			case 0:
				plim1 = strtod(optarg, (char**)0);
				break;
			case 1:
				plim2 = strtod(optarg, (char**)0);
				break;
			}
			break;
#endif
		case 'i':
			interval = strtod(optarg,(char**)0);
			break;
		case 't':
			timeout = strtod(optarg,(char**)0);
			break;
		case 'o':
			strncpy(fn,optarg,sizeof(fn)-1);
			fn[sizeof(fn)-1] = 0;
			break;
		case 'r':
			relative_time = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'h':
			usage();
			exit(0);
		}
	}
	
	if (strlen(fn) > 0) {
		fp = fopen(fn, "w");
		if (!fp) {
			perror("fopen");
			exit(1);
		}
	}

	if (argc - optind) {
		pid_t child_pid;

		/* spawn mode */
		child_pid = fork();
		if (child_pid == 0) {
			execvp(argv[optind], argv+optind);
		} else {
			sampling_loop(fp, interval, 0, child_pid, verbose, relative_time);
		}
	} else {
		/* polling mode */
		if (timeout < 0.0 || interval < 0.0) {
			printf("Please specify timeout(-t) and interval(-i)\n");
			usage();
			exit(1);
		}
		sampling_loop(fp, interval, timeout, 0, verbose, relative_time);
	}

	if (fp != stdout) {
		fclose(fp);
	}

	unlink_lock();

	return 0;
}
