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
	int *pstate;
	int nthreads;
	int interval_usec = 500 * 1000;

	nthreads = omp_get_max_threads();
	pstate = (int *)malloc(sizeof(int)*nthreads);


	
#pragma omp parallel shared(pstate)
	{
		int rc, i;
		int tid;
		int nths;
		struct intercoolr ic;

		tid = omp_get_thread_num();
		nths = omp_get_num_threads();
		rc = intercoolr_init(&ic, tid);
		if (rc < 0) {
			printf("init_intercoolr failed\n");
			exit(1);
		}

		while (1) {
			intercoolr_sample(&ic);

			pstate[tid] = intercoolr_last_pstate(&ic);
#pragma omp barrier
			if (tid == 0) {
				for (i = 0; i < nths; i++)
					printf("%2d ", pstate[i]);
				printf("\n");
			}
			usleep(interval_usec);
		}
		
		intercoolr_fini(&ic);
	}
	return 0;
}

