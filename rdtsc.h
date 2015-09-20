#ifndef __RDTSC_H_DEFINE__
#define __RDTSC_H_DEFINE__

#include <stdint.h>

#if defined(__i386__)

static inline  uint64_t rdtsc(void)
{
	uint64_t x;

	__asm__ volatile ("rdtsc" : "=A" (x));
	return x;
}
#elif defined(__x86_64__)

static inline uint64_t rdtsc(void)
{
	unsigned hi, lo;

	__asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
	return ((uint64_t)lo)|(((uint64_t)hi)<<32);
}

#elif defined(__powerpc__)

#if defined(__powerpc64__)

static inline uint64_t rdtsc(void)
{
	uint64_t x;

	/* read 64-bit timebase */
	asm volatile ("mfspr %0,%1" :
		      "=&r"(x)
		      : "i"(0x10C));
	return x;
}

#else

static inline uint64_t rdtsc(void)
{
	uint64_t result = 0;
	unsigned long int upper, lower, tmp;

	__asm__ volatile("0:\n"
			"\tmftbu   %0\n"
			"\tmftb    %1\n"
			"\tmftbu   %2\n"
			"\tcmpw    %2,%0\n"
			"\tbne     0b\n"
			: "=r"(upper), "=r"(lower), "=r"(tmp)
			);
	result = upper;
	result = result<<32;
	result = result|lower;

	return result;
}
#endif

#endif /* __powerpc__ */

#endif
