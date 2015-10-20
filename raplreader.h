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
	uint64_t e[RAPLREADER_MAXSOCKETS]; /* uj */
	double t; /* sec */
};

struct raplreader {
	int nsockets;

	char *sysfs_socket[RAPLREADER_MAXSOCKETS];
	char *sysfs_dram[RAPLREADER_MAXSOCKETS];
	char *sysfs_socket_max[RAPLREADER_MAXSOCKETS];
	char *sysfs_dram_max[RAPLREADER_MAXSOCKETS];

	/* XXX: implement pp1 if needed */

	uint64_t socket_max[RAPLREADER_MAXSOCKETS];
	uint64_t dram_max[RAPLREADER_MAXSOCKETS];

	/* XXX: only server class machine */

	/* _sample() updates the following varaibles */
	struct raplsample s[2];
	int idx;
	uint64_t delta_e[RAPLREADER_MAXSOCKETS]; /* uj */
	double   delta_t[RAPLREADER_MAXSOCKETS]; /* sec */
};

/* returns 0 if successful */
extern int raplreader_init(struct raplreader *rr);
extern int raplreader_sample(struct raplreader *rr);

#endif
