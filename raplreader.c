/*
 * A simple Intel RAPL reader library
 *
 * This code requires the intel_rapl driver
 *
 * Kaz Yoshii <ky@anl.gov>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include "raplreader.h"

static int raplreader_debug = 0;

#define RAPLBASEPATH "/sys/devices/virtual/powercap/intel-rapl/"

static clockid_t clk_id = CLOCK_MONOTONIC_RAW;

static double gettimesec(void)
{
	struct timespec tp;
	double ret = 0.0;

	if( clock_gettime(clk_id, &tp) == 0 ) {
		ret = (double)tp.tv_sec + (double)tp.tv_nsec/1000.0/1000.0/1000.0;
	}
	return ret;
}


static uint64_t read_uint64(const char *fn)
{
	FILE *fp;
	char buf[128];
	uint64_t ret = 0;

	fp = fopen(fn, "r");
	if (!fp) {
		perror("open");
		return -1;
	}
	fgets(buf, sizeof(buf), fp);
	fclose(fp);

	ret = strtoul(buf,0,10);
	return ret;
}


static int read_str(const char *fn, char *buf)
{
	FILE *fp;

	fp = fopen(fn, "r");
	if (!fp) {
		perror("open");
		return -1;
	}
	fgets(buf, sizeof(buf), fp);
	fclose(fp);

	return strlen(buf);
}


/*
$ find /sys/devices/virtual/powercap/intel-rapl/ -name "*energy*"
/sys/devices/virtual/powercap/intel-rapl/intel-rapl:0/energy_uj
/sys/devices/virtual/powercap/intel-rapl/intel-rapl:0/max_energy_range_uj
/sys/devices/virtual/powercap/intel-rapl/intel-rapl:0/intel-rapl:0:0/energy_uj
/sys/devices/virtual/powercap/intel-rapl/intel-rapl:0/intel-rapl:0:0/max_energy_range_uj
/sys/devices/virtual/powercap/intel-rapl/intel-rapl:1/energy_uj
/sys/devices/virtual/powercap/intel-rapl/intel-rapl:1/max_energy_range_uj
/sys/devices/virtual/powercap/intel-rapl/intel-rapl:1/intel-rapl:1:0/energy_uj
/sys/devices/virtual/powercap/intel-rapl/intel-rapl:1/intel-rapl:1:0/max_energy_range_uj
*/

static char *getpath_sysfs_top(int n, const char *fn)
{
	char buf[BUFSIZ];
	snprintf(buf, sizeof(buf), "%s/intel-rapl:%d/%s",
		 RAPLBASEPATH, n, fn);

	if (access(buf, R_OK) != 0)
		return NULL;

	return strdup(buf);
}

static char *getpath_sysfs_sub(int n, int subn, const char *fn)
{
	char buf[BUFSIZ];
	snprintf(buf, sizeof(buf), "%s/intel-rapl:%d/intel-rapl:%d:%d/%s",
		 RAPLBASEPATH, n, n, subn, fn);

	if (access(buf, R_OK) != 0)
		return NULL;

	return strdup(buf);
}


int raplreader_init(struct raplreader *rr)
{
	char *p;
	char tmp[BUFSIZ];
	int i;
	uint64_t val;

	memset(rr, 0, sizeof(struct raplreader));
	
	/* detect how many sockets and fill sysfs filenames*/
	for (i = 0; i < RAPLREADER_MAXSOCKETS; i++) {
		p = getpath_sysfs_top(i, "energy_uj");
		if (!p)
			break;
		rr->sysfs_socket[i] = p;

		p = getpath_sysfs_top(i, "max_energy_range_uj");
		if (!p)
			break;
		rr->sysfs_socket_max[i] = p;

		val = read_uint64(p);
		if (raplreader_debug) {
			printf("socket%d max_energy_range_uj %lu\n",
			       i, val);
		}
		rr->socket_max[i] = val;
			

		/* find dram entries */
		p = getpath_sysfs_sub(i, 0, "name");
		if (read_str(p, tmp) <= 0)
			continue;
		free(p);
		if (strcmp(tmp, "dram") != 0)
			continue;

		p = getpath_sysfs_sub(i, 0, "energy_uj");
		if (!p)
			break;
		rr->sysfs_dram[i] = p;

		p = getpath_sysfs_sub(i, 0, "max_energy_range_uj");
		if (!p)
			break;
		rr->sysfs_dram_max[i] = p;
	}

	rr->nsockets = i;

	return 0;
}

#if 0
int raplreader_sample(struct raplreader *rr)
{
	int i;
}



void read_rapl(struct raplsample *rs)
{
        rs->e[0] = read_uint64( RAPLBASEPATH "intel-rapl:0/energy_uj" );
	rs->e[1] = read_uint64( RAPLBASEPATH "intel-rapl:1/energy_uj" );
	rs->t = gettimesec();
}


uint64_t read_rapl_maxenergy()
{
	char *fn =  RAPLBASEPATH "intel-rapl:0/max_energy_range_uj";

	if (maxenergy > 0) return maxenergy;
	maxenergy = read_uint64(fn);
	return maxenergy;
}

void diff_energy(struct raplsample *rs1, struct raplsample *rs2, double *e)
{
	uint64_t maxrange = read_rapl_maxenergy();
	uint64_t ediff;
	int i;

	for (i = 0; i < 2; i++ ) {
		if (rs2->e[i] >= rs1->e[i]) {
			ediff = rs2->e[i] - rs1->e[i];
		} else {
			ediff = maxrange - rs1->e[i] + rs2->e[i];
		}
                e[i] = (double)ediff/1000.0/1000.0;
	}
}
#endif

#ifdef __TEST_MAIN__

#include <assert.h>

int main()
{
	struct raplreader rr;
	int rc;

	raplreader_debug ++;

	rc = raplreader_init(&rr);
	assert(rc == 0);

	printf("nsockets: %d\n", rr.nsockets);

	return 0;
}
#endif

