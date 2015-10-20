TARGETS=measure_pstate_latency set_pstate monitor_pstate raplreader_test

CC=gcc
CFLAGS=-O2 -Wall -g -fopenmp
LDFLAGS=-lgomp

all: $(TARGETS)

intercoolr.o : intercoolr.c intercoolr.h

libintercoolr.a : intercoolr.o
	$(RM) $@
	$(AR) cq $@ $^

measure_pstate_latency : measure_pstate_latency.c libintercoolr.a
	$(CC) -o $@ $(CFLAGS) $< -L. -lintercoolr

set_pstate : set_pstate.c libintercoolr.a
	$(CC) -o $@ $(CFLAGS) $< -L. -lintercoolr $(LDFLAGS)

monitor_pstate : monitor_pstate.c libintercoolr.a
	$(CC) -o $@ $(CFLAGS) $< -L. -lintercoolr $(LDFLAGS)

raplreader_test: raplreader.c
	$(CC) -o $@ $(CFLAGS) -D__TEST_MAIN__ $< 

clean:
	rm -f $(TARGETS)
	rm -f *.o *.a
	rm -f *~

