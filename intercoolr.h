#ifndef __INTERCOOLR_H_DEFINED__
#define __INTERCOOLR_H_DEFINED__

#include <stdint.h>


/* defined in pstate_user.c */
enum {
	PSTATE_INFO = 0,
	PSTATE_SET,
	PSTATE_DEBUG,
};

union  pstate_param {
	struct _info {
		uint16_t min, max, turbo, scaling;
	} info;
	struct _set {
		uint16_t request, old;
	} set;
};

struct ic_perf_sample {
	uint64_t aperf, mperf, tsc;
	uint16_t pstate;
	double time;
};

struct intercoolr {
	int cpuid;
	int fd; /* /dev/pstate_user */
	int fd_amperf_bin; /* per-cpu amperf_bin */

	int psmin, psmax, psturbo;

	struct ic_perf_sample s[2];
	int sidx;
};

extern int  intercoolr_init(struct intercoolr *ic, int cpuid);
extern void intercoolr_fini(struct intercoolr *ic);
extern int  intercoolr_set_pstate(struct intercoolr *ic, int pstate);
extern int  intercoolr_sample(struct intercoolr *ic);

extern uint64_t intercoolr_diff_aperf(struct intercoolr *ic);
extern uint64_t intercoolr_diff_mperf(struct intercoolr *ic);
extern uint64_t intercoolr_diff_tsc(struct intercoolr *ic);
extern double   intercoolr_diff_time(struct intercoolr *ic);
extern uint16_t intercoolr_last_pstate(struct intercoolr *ic);

#endif
