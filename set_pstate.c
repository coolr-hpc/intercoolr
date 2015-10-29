#define _GNU_SOURCE
#include <sched.h>

#include <omp.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "intercoolr.h"
#include "rdtsc.h"


int main(int argc, char *argv[])
{
	int pstate;
	int threadno = -1;

	if (argc < 2) {
		printf("Usage: %s pstate [threadno]\n", argv[0]);
		return 1;
	}

	pstate = atoi(argv[1]);
	if (argc >= 3)
		threadno = atoi(argv[2]);
	
#pragma omp parallel shared(pstate)
	{
		int rc;
		int tid;
		struct intercoolr ic;

		tid = omp_get_thread_num();
		/* intercoolr_init needs to be called from each thread */
		rc = intercoolr_init(&ic, tid);
		if (rc < 0) {
			printf("init_intercoolr failed\n");
			exit(1);
		}


		if (threadno == -1 || threadno == tid) {
			printf("tid=%2d min=%d max=%d turbo=%d  requested=%d\n",
			       tid, ic.psmin, ic.psmax, ic.psturbo, pstate);
			intercoolr_set_pstate(&ic, pstate);
		}
#pragma omp barrier
		
		intercoolr_fini(&ic);
	}
	return 0;
}
