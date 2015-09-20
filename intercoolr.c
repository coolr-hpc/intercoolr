/** @brief APIs to change hardware state such as P-state
 *
 *  Requirements and assumptions:
 *  - turbofreq or succesor kernel module
 *  - cpu affinity is set
 *  - be called from process models or threading models
 *
 *  Contact: Kaz Yoshii <ky@anl.gov>
 */
#define _GNU_SOURCE
#include <sched.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <inttypes.h>

#include "intercoolr.h"
#include "rdtsc.h"

static inline int corebind(int rank)
{
	cpu_set_t  cpuset_mask;

	CPU_ZERO(&cpuset_mask);
	CPU_SET(rank, &cpuset_mask);

	if (sched_setaffinity(0, sizeof(cpuset_mask), &cpuset_mask) == -1)
		return -1;

	return 0;
}

/* low-level APIs */

int open_pstate_user(void)
{
	int fd;

	fd = open("/dev/pstate_user", O_RDWR);
	if (fd < 0) {
		perror("open");
		return -1;
	}
	return fd;
}

int pstate_user(int fd, int cmd, void *p)
{
	int rc;

	rc = ioctl(fd, cmd, p);

	return rc;
}

static int open_amperf_bin(int cpuid)
{
	char fn[128];
	int fd;

	snprintf(fn, sizeof(fn),
		 "/sys/devices/system/cpu/cpu%d/amperf_bin",
		 cpuid);

	fd = open(fn, O_RDONLY);
	if (fd < 0) {
		perror("open");
		return -1;
	}
	return fd;
}

struct amperf {
	uint64_t aperf, mperf;
	uint16_t pstate;
};

static void read_amperf_bin(int fd, struct amperf *p)
{
	lseek(fd, 0, SEEK_SET);
	read(fd, p, sizeof(struct amperf));
}

/* higher-level APIs */

/** @brief intercoolr initialization function
 *
 */

int intercoolr_sample(struct intercoolr *ic)
{
	int i;
	struct timeval tv;
	double t;
	struct amperf p;

	gettimeofday(&tv, NULL);
	t = (double)tv.tv_sec + (double)tv.tv_usec * 1e-6;

	read_amperf_bin(ic->fd_amperf_bin, &p);
	i = ic->sidx;

	ic->s[i].mperf = p.mperf;
	ic->s[i].aperf = p.aperf;
	ic->s[i].tsc = rdtsc();
	ic->s[i].time = t;
	ic->s[i].pstate = p.pstate;

	if (ic->sidx == 0)
		ic->sidx = 1;
	else
		ic->sidx = 0;

	return 0;
}

#define FIRSTIDX(ic)   ((ic->sidx == 0)?0:1)
#define SECONDIDX(ic)  ((ic->sidx == 0)?1:0)

uint16_t intercoolr_last_pstate(struct intercoolr *ic)
{
	return ic->s[SECONDIDX(ic)].pstate;
}

uint64_t intercoolr_diff_aperf(struct intercoolr *ic)
{
	return
		ic->s[SECONDIDX(ic)].aperf -
		ic->s[FIRSTIDX(ic)].aperf;
}

uint64_t intercoolr_diff_mperf(struct intercoolr *ic)
{
	return
		ic->s[SECONDIDX(ic)].mperf -
		ic->s[FIRSTIDX(ic)].mperf;
}

uint64_t intercoolr_diff_tsc(struct intercoolr *ic)
{
	return
		ic->s[SECONDIDX(ic)].tsc -
		ic->s[FIRSTIDX(ic)].tsc;
}

double intercoolr_diff_time(struct intercoolr *ic)
{
	return
		ic->s[SECONDIDX(ic)].time -
		ic->s[FIRSTIDX(ic)].time;
}


int intercoolr_init(struct intercoolr *ic, int cpuid)
{
	int rc;
	union  pstate_param p;

	memset(ic, 0, sizeof(struct intercoolr));

	if (!ic)
		return -1;

	if (cpuid >= 0) {
		corebind(cpuid);
		ic->cpuid = cpuid;
	} else
		ic->cpuid = sched_getcpu();

	/* XXX: check whether /dev/pstate_user exists */

	ic->fd = open_pstate_user();
	if (ic->fd < 0)
		return -1;


	ic->fd_amperf_bin = open_amperf_bin(ic->cpuid);
	if (ic->fd_amperf_bin < 0)
		return -1;

	rc = pstate_user(ic->fd, PSTATE_INFO, &p);
	if (rc < 0)
		goto out;


	ic->psmin = p.info.min;
	ic->psmax = p.info.max;
	ic->psturbo = p.info.turbo;

	ic->sidx = 0;

	rc = intercoolr_sample(ic);
	if (rc < 0)
		goto out;
	usleep(1000);
	rc = intercoolr_sample(ic);
	if (rc < 0)
		goto out;

	pstate_user(ic->fd, PSTATE_DEBUG, &p);

	return 0;
 out:
	close(ic->fd);
	return -1;
}

void intercoolr_fini(struct intercoolr *ic)
{
	close(ic->fd);
}

int intercoolr_set_pstate(struct intercoolr *ic, int pstate)
{
	union pstate_param p;
	int rc;

	p.set.request = pstate;
	p.set.old = 0;

	rc = pstate_user(ic->fd, PSTATE_SET, &p);
	if (rc < 0) {
		close(ic->fd);
		return -1;
	}

	return p.set.old;
}
