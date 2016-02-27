/*
 * A simple Intel RAPL reader library
 *
 * This code requires the intel_rapl driver
 *
 * Limitation: if the time interval between two samples() is too long;
 * the counter wraps around twice or more, the energy diff becomes
 * incorrect.
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
		printf("read_uint64 open: %s\n",fn);
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
		printf("read_str open: %s\n",fn);
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
			printf("%d socket max_energy_range_uj %lu\n",
			       i, val);
		}
		rr->socket_max[i] = val;
			

		/* find dram entries */
		p = getpath_sysfs_sub(i, 0, "name");
		if (read_str(p, tmp) <= 0)
			continue;
		free(p);
		if (strncmp(tmp, "dram", 4) != 0)
			continue;

		rr->dram_available = 1;

		p = getpath_sysfs_sub(i, 0, "energy_uj");
		if (!p)
			break;
		rr->sysfs_dram[i] = p;

		p = getpath_sysfs_sub(i, 0, "max_energy_range_uj");
		if (!p)
			break;
		rr->sysfs_dram_max[i] = p;

		val = read_uint64(p);
		if (raplreader_debug) {
			printf("%d dram   max_energy_range_uj %lu\n",
			       i, val);
		}
		rr->dram_max[i] = val;

	}
	if (i == 0)
		return -1;

	rr->nsockets = i;

	if (raplreader_debug) {
		printf("nsockets=%d\n", rr->nsockets);
	}

	raplreader_sample(rr); /* sample once here. then the next sample can yield correct diff */
	usleep(10000);

	return 0;
}

static uint64_t delta_energy(uint64_t cur_e, uint64_t prev_e, uint64_t max_e)
{
	if (cur_e >= prev_e)
		return cur_e - prev_e;

	return cur_e + max_e - prev_e;
}

int raplreader_sample(struct raplreader *rr)
{
	int i, idx, previdx;
	uint64_t val;
	double total_p, energy_p;
	double t;
	static int  firstsample = 1;

	idx = rr->idx;
	if (idx == 0)
		previdx = 1;
	else
		previdx = 0;

	energy_p = total_p = 0.0;
	for (i = 0; i < rr->nsockets; i++) {
		t = gettimesec();
		val = read_uint64(rr->sysfs_socket[i]);
		rr->socket[i].e[idx] = val;
		rr->socket[i].t[idx] = t;

		/* calculate delta */
		if (firstsample) {
			rr->delta_socket[i] = 0.0;
			rr->delta_t[i]      = 0.0;
		} else {
			rr->delta_socket[i] = delta_energy(val,
							   rr->socket[i].e[previdx],
							   rr->socket_max[i]);
			rr->delta_t[i]      = t - rr->socket[i].t[previdx];
		}

		if (rr->delta_t[i] > 0.0) {
			rr->power_socket[i] = (double)rr->delta_socket[i] * 1e-6;
			rr->power_socket[i] /= rr->delta_t[i];
			energy_p += (double)rr->delta_socket[i] * 1e-6;
			total_p += rr->power_socket[i];
		}
		
		if (rr->dram_available) {

			val = read_uint64(rr->sysfs_dram[i]);

			rr->dram[i].e[idx] = val;
			rr->dram[i].t[idx] = t;

			if (firstsample) {
				rr->delta_dram[i] = 0.0;
			} else {
				rr->delta_dram[i] = delta_energy(val,
								 rr->dram[i].e[previdx], 
								 rr->dram_max[i]);
			}

			if (rr->delta_t[i] > 0.0) {
				rr->power_dram[i] = (double)rr->delta_dram[i] * 1e-6;
				rr->power_dram[i] /= rr->delta_t[i];
				energy_p += (double)rr->power_dram[i] * 1e-6;
				total_p += rr->power_dram[i];
			}
		}
	}
	rr->power_total = total_p;
	rr->energy_total = energy_p;
	rr->energy += energy_p;

	if (rr->idx == 0)
		rr->idx = 1;
	else
		rr->idx = 0;

	if (firstsample)
		firstsample = 0;

	return 0;
}

#ifdef __TEST_MAIN__

#include <assert.h>

int main()
{
	struct raplreader rr;
	int i, j, n;
	int rc;
	double acc_energy = 0.0; /* accumulated energy */
	double acc_time = 0.0;

	raplreader_debug ++;

	rc = raplreader_init(&rr);
	assert(rc == 0);

	n = rr.nsockets;

	raplreader_sample(&rr);
	sleep(1);
	for (i = 0; i < 10; i++) {
		raplreader_sample(&rr);
		acc_energy += rr.energy_total;
		acc_time += rr.delta_t[0];

		printf("acc.: %7.2lf [J] %5.1lf [S]  sample: %lf [W]  %lf [J] ",
		       acc_energy, acc_time, 
		       rr.power_total, rr.energy_total);
		for (j = 0; j < n; j++) {
			printf("socket%d=%lf ", j, rr.power_socket[j]);
			if (rr.dram_available)
				printf("dram%d=%lf ", j, rr.power_dram[j]);
		}
		printf("\n");

		sleep(1);
	}

	return 0;
}
#endif

