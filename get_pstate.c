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
	int rc;
	struct intercoolr ic;

	/* intercoolr_init needs to be called from each thread.
	   internally this function sets the cpu affinity for now
	*/
	rc = intercoolr_init(&ic, 0);
	if (rc < 0) {
		printf("init_intercoolr failed\n");
		exit(1);
	}
	/* assume pstate range is the same between cores */
	printf("pmin=%2d pmax=%2d pturbo=%2d\n",
		ic.psmin, ic.psmax, ic.psturbo);

	intercoolr_fini(&ic);

	return 0;
}
