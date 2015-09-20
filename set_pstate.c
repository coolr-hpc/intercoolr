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

	if (argc < 2) {
		printf("Usage: %s pstate\n", argv[0]);
		return 1;
	}

	pstate = atoi(argv[1]);
	
#pragma omp parallel shared(pstate)
	{
		int rc;
		int tid;
		struct intercoolr ic;

		tid = omp_get_thread_num();
		rc = intercoolr_init(&ic, tid);
		if (rc < 0) {
			printf("init_intercoolr failed\n");
			exit(1);
		}

		printf("min=%d max=%d turbo=%d  requested=%d\n",
		       ic.psmin, ic.psmax, ic.psturbo, pstate);

		intercoolr_set_pstate(&ic, pstate);
		
		intercoolr_fini(&ic);
	}
	return 0;
}

