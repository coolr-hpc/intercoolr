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
	uint64_t delta_socket[RAPLREADER_MAXSOCKETS]; /* uj */
	uint64_t delta_dram[RAPLREADER_MAXSOCKETS]; /* uj */
	double   delta_t[RAPLREADER_MAXSOCKETS]; /* sec */
	double  energy_total; /* J */

	double  power_socket[RAPLREADER_MAXSOCKETS]; /* W */
	double  power_dram[RAPLREADER_MAXSOCKETS]; /* W */
	double  power_total; /* W */
};

/* returns 0 if successful */
extern int raplreader_init(struct raplreader *rr);

extern int raplreader_sample(struct raplreader *rr);

#endif
