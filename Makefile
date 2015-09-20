TARGETS=measure_pstate_latency

CC=gcc
CFLAGS=-O2 -Wall -g

all: $(TARGETS)

libintercoolr.a : intercoolr.o
	$(AR) cq $@ $^

measure_pstate_latency : measure_pstate_latency.c libintercoolr.a
	$(CC) -o $@ $(CFLAGS) $< -L. -lintercoolr

clean:
	rm -f $(TARGETS)
	rm -f *.o *.a
	rm -f *~
