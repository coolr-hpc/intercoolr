/*
 * Header file for a simple Intel RAPL reader library
 *
 * This code requires the intel_rapl driver
 *
 * Kaz Yoshii <ky@anl.gov>
 */
#ifndef __RAPLREADER_DEFINED
#define __RAPLREADER_DEFINED

#include <stdint.h>

#define RAPLREADER_MAXSOCKETS  (8)

struct raplsample {
	uint64_t e[2]; /* uj */
	double t[2]; /* sec */
};
/*
  NOTE: all values are sampled at almost the same time.  (the latency
  is one read syscall plus some function overhead) maybe we can just
  move out 't[2]' from struct raplsample and add them as a member of
  raplreader.
*/

struct raplreader {
	int nsockets;

	char *sysfs_socket[RAPLREADER_MAXSOCKETS];
	char *sysfs_dram[RAPLREADER_MAXSOCKETS];
	char *sysfs_socket_max[RAPLREADER_MAXSOCKETS];
	char *sysfs_dram_max[RAPLREADER_MAXSOCKETS];

	uint64_t socket_max[RAPLREADER_MAXSOCKETS];
	uint64_t dram_max[RAPLREADER_MAXSOCKETS];

	int dram_available;

	/* _sample() updates the following varaibles */
	struct raplsample socket[RAPLREADER_MAXSOCKETS];
	struct raplsample dram[RAPLREADER_MAXSOCKETS];

	int idx;
	double   delta_t[RAPLREADER_MAXSOCKETS]; /* sec */
	uint64_t delta_socket[RAPLREADER_MAXSOCKETS]; /* uj */
	uint64_t delta_dram[RAPLREADER_MAXSOCKETS]; /* uj */
	double  energy_total; /* J */

	double  power_socket[RAPLREADER_MAXSOCKETS]; /* W */
	double  power_dram[RAPLREADER_MAXSOCKETS]; /* W */
	double  power_total; /* W */
};

/* returns 0 if successful */
extern int raplreader_init(struct raplreader *rr);

extern int raplreader_sample(struct raplreader *rr);


static inline double raplreader_get_power_socket(struct raplreader *rr, int n)
{
	if (!rr) return -1.0;
	return rr->power_socket[n];
}

static inline double raplreader_get_power_dram(struct raplreader *rr, int n)
{
	if (!rr) return -1.0;
	return rr->power_dram[n];
}

static inline double raplreader_get_total_power(struct raplreader *rr)
{
	if (!rr) return -1.0;
	return rr->power_total;
}

static inline double raplreader_get_total_energy(struct raplreader *rr)
{
	if (!rr) return -1.0;
	return rr->energy_total;
}

static inline int raplreader_is_dram_available(struct raplreader *rr)
{
	if (!rr) return 0;

	return rr->dram_available;
}

static inline int raplreader_get_nsockets(struct raplreader *rr)
{
	if (rr) {
		return rr->nsockets;
	}
	return -1;
}

static inline double raplreader_get_ts(struct raplreader *rr)
{
	if (rr) {
		if (rr->idx == 0)
			return rr->socket[0].t[1];
		else
			return rr->socket[0].t[0];
	}
	return 0.0;
}

static inline double raplreader_get_ts2(struct raplreader *rr)
{
	if (rr) {
		if (rr->idx == 0)
			return rr->socket[0].t[1];
		else
			return rr->socket[0].t[0];
	}
	return 0.0;
}

#endif
