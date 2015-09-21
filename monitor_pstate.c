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
	struct ic_perf_sample *perfdiff;

	int nthreads;
	int interval_usec = 500 * 1000;

	nthreads = omp_get_max_threads();
	perfdiff = (struct ic_perf_sample *)malloc(sizeof(struct ic_perf_sample)*nthreads);

	
#pragma omp parallel shared(perfdiff)
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

			perfdiff[tid].aperf = intercoolr_diff_aperf(&ic);
			perfdiff[tid].mperf = intercoolr_diff_mperf(&ic);
			perfdiff[tid].tsc = intercoolr_diff_tsc(&ic);
			perfdiff[tid].time  = intercoolr_diff_time(&ic);
			perfdiff[tid].pstate = intercoolr_last_pstate(&ic);

#pragma omp barrier
			if (tid == 0) {
				for (i = 0; i < nths; i++)
					printf("%.2lf ",
					       (double)perfdiff[i].aperf/
					       perfdiff[i].time * 1e-9);
				printf("\n");
			}
			usleep(interval_usec);
		}
		
		intercoolr_fini(&ic);
	}
	return 0;
}

