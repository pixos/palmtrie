/*_
 * Copyright (c) 2019-2020 Hirochika Asai <asai@jar.jp>
 * All rights reserved.
 */

#include "../palmtrie.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>

/*
 * Get current time at the microsecond granularity
 */
double
getmicrotime(void)
{
    struct timeval tv;
    double microsec;

    if ( 0 != gettimeofday(&tv, NULL) ) {
        return 0.0;
    }

    microsec = (double)tv.tv_sec + (1.0 * tv.tv_usec / 1000000);

    return microsec;
}

/*
 * Xorshift
 */
static __inline__ u32
xor128(void)
{
    static u32 x = 123456789;
    static u32 y = 362436069;
    static u32 z = 521288629;
    static u32 w = 88675123;
    u32 t;

    t = x ^ (x<<11);
    x = y;
    y = z;
    z = w;
    return w = (w ^ (w>>19)) ^ (t ^ (t >> 8));
}

#define NRTRIALS    30
double g_t0;
double g_t1;
int g_nrsigs;
long long g_cnt;
double g_data_timer[NRTRIALS];
long long g_data_cnt[NRTRIALS];
void
sig_handler(int signum)
{
    double t;

    t = getmicrotime();
    g_data_timer[g_nrsigs] = t;
    g_data_cnt[g_nrsigs] = g_cnt;
    g_nrsigs++;

    (void)signum;
}

/*
 * Performance test
 */
static int
test_lpm_perf(enum palmtrie_type type, const char *fname)
{
    struct palmtrie palmtrie;
    FILE *fp;
    char buf[4096];
    int prefix[4];
    int prefixlen;
    int nexthop[4];
    int ret;
    addr_t addr1 = {0, {0, 0, 0, 0, 0, 0, 0, 0}};
    addr_t mask = {0, {0, 0, 0, 0, 0, 0, 0, 0}};
    u64 addr2;
    long long i;
    addr_t tmp = {0, {0, 0, 0, 0, 0, 0, 0, 0}};
    double delta;
    u64 x;

    /* Initialize */
    palmtrie_init(&palmtrie, type);

    /* Load from the linx file */
    fp = fopen(fname, "r");
    if ( NULL == fp ) {
        return -1;
    }

    /* Load the full route */
    while ( !feof(fp) ) {
        if ( !fgets(buf, sizeof(buf), fp) ) {
            continue;
        }
        ret = sscanf(buf, "%d.%d.%d.%d/%d %d.%d.%d.%d", &prefix[0], &prefix[1],
                     &prefix[2], &prefix[3], &prefixlen, &nexthop[0],
                     &nexthop[1], &nexthop[2], &nexthop[3]);
        if ( ret < 0 ) {
            return -1;
        }

        /* Convert to u32 */
        addr1.a[0] = ((u32)prefix[0] << 24) + ((u32)prefix[1] << 16)
            + ((u32)prefix[2] << 8) + (u32)prefix[3];
        addr2 = ((u32)nexthop[0] << 24) + ((u32)nexthop[1] << 16)
            + ((u32)nexthop[2] << 8) + (u32)nexthop[3];

        /* Add an entry */
        mask.a[0] = (1ULL << (32 - prefixlen)) - 1;
        ret = palmtrie_add_data(&palmtrie, addr1, mask, prefixlen, addr2);
        if ( ret < 0 ) {
            return -1;
        }
    }
    ret = palmtrie_commit(&palmtrie);
    if ( ret < 0 ) {
        printf("Failed to commit.\n");
        return -1;
    }

    /* Close */
    fclose(fp);

    struct sigaction act;
    struct sigaction oldact;
    struct itimerval itval;
    sigemptyset(&act.sa_mask);
    act.sa_handler = sig_handler;
    act.sa_flags = 0;
    if ( 0 != sigaction(SIGVTALRM, &act, &oldact) ) {
        return -1;
    }
    itval.it_value.tv_sec = 10;
    itval.it_value.tv_usec = 0;
    itval.it_interval.tv_sec = 10;
    itval.it_interval.tv_usec = 0;
    if ( 0 != setitimer(ITIMER_VIRTUAL, &itval, NULL) ) {
        return -1;
    }

    /* Benchmark */
    x = 0;
    g_t0 = getmicrotime();
    g_nrsigs = 0;
    for ( g_cnt = 0; g_nrsigs < NRTRIALS; g_cnt++ ) {
        tmp.a[0] = xor128();
        x ^= palmtrie_lookup(&palmtrie, tmp);
    }
    g_t1 = getmicrotime();
    sigaction(SIGVTALRM, &oldact, NULL);
    (void)x;
    delta = g_t1 - g_t0;
    printf("# %p %.6lf [sec] / %lf [Mlookup/sec]\n",
           (void *)x, delta, g_cnt / delta / 1000 / 1000);

    double t;
    long long cnt;
    t = g_t0;
    cnt = 0;
    for ( i = 0; i < NRTRIALS; i++ ) {
        delta = g_data_timer[i] - t;
        printf("%lf %lld %lf\n", g_data_timer[i], g_data_cnt[i],
               (g_data_cnt[i] - cnt) / delta / 1000 / 1000 );
        t = g_data_timer[i];
        cnt = g_data_cnt[i];
    }

    return 0;
}

/*
 * Main routine for the basic test
 */
int
main(int argc, const char *const argv[])
{
    const char *fname;
    const char *type;

    if ( argc != 3 ) {
        fprintf(stderr, "Usage: %s <tcam-file> <type>\n", argv[0]);
        return EXIT_FAILURE;
    }
    fname = argv[1];
    type = argv[2];
    if ( 0 == strcmp(type, "sl") ) {
        printf("#SL:\n");
        test_lpm_perf(PALMTRIE_SORTED_LIST, fname);
    } else if ( 0 == strcmp(type, "tpt") ) {
        printf("#TPT(%d):\n", PALMTRIE_PRIORITY_SKIP);
        test_lpm_perf(PALMTRIE_BASIC, fname);
    } else if ( 0 == strcmp(type, "mtpt") ) {
        printf("#MTPT(%d/%d):\n", PALMTRIE_PRIORITY_SKIP, PALMTRIE_MTPT_STRIDE);
        test_lpm_perf(PALMTRIE_DEFAULT, fname);
    } else if ( 0 == strcmp(type, "popmtpt") ) {
        printf("#POPMTPT(%d/%d):\n", PALMTRIE_PRIORITY_SKIP, PALMTRIE_MTPT_STRIDE);
        test_lpm_perf(PALMTRIE_PLUS, fname);
    }

    return 0;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
