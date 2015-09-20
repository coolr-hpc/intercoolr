#define _GNU_SOURCE
#include <sched.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "intercoolr.h"
#include "rdtsc.h"

static double gettime(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return (double)tv.tv_sec + (double)tv.tv_usec * 1e-6;
}

static void measure_sample_latency(struct intercoolr *ic)
{
	uint64_t tsc_start, tsc_elapsed;
	double time_start, time_elapsed;
	int i, niters = 1000000;

	intercoolr_set_pstate(ic, ic->psmax); /* set to TDP max */

	time_start = gettime();
	tsc_start = rdtsc();
	for (i = 0; i < niters; i++)
		intercoolr_sample(ic);

	tsc_elapsed = rdtsc() - tsc_start;
	time_elapsed = gettime() - time_start;

	printf("elsped: %lf [sec]  %lu [cycles]\n", time_elapsed, tsc_elapsed);
	printf("latency: %lf [usec/call]  %lf [cycles/call]\n",
	       time_elapsed/(double)niters * 1e+6,
	       tsc_elapsed/(double)niters);
}

static double fixedwork(struct intercoolr *ic)
{
	uint64_t tsc_start;

	intercoolr_sample(ic);
	tsc_start = rdtsc();
	while (rdtsc() - tsc_start < ic->psmax * 1000)
		;
	intercoolr_sample(ic);
	return intercoolr_diff_aperf(ic) / 
		intercoolr_diff_time(ic);
}

static void change_pstate_test(struct intercoolr *ic)
{
	int i, j;

	intercoolr_set_pstate(ic, ic->psmin);

	for (i = ic->psmin; i < ic->psmax ; i++) {
		printf("#request=%d\n", i);
		intercoolr_set_pstate(ic, i);
		for (j= 0; j < 1000 ; j++)
			printf("%lf\n",  fixedwork(ic));
		
	}
}

int main(int argc, char *argv[])
{
	struct intercoolr ic;
	int rc;

	rc = intercoolr_init(&ic, 0);
	if (rc < 0) {
		printf("init_intercoolr failed\n");
		return 1;
	}

	printf("# info: min=%d max=%d turbo=%d\n",
	       ic.psmin, ic.psmax, ic.psturbo);


	// measure_sample_latency(&ic);
	change_pstate_test(&ic);

	intercoolr_fini(&ic);

	return 0;
}

