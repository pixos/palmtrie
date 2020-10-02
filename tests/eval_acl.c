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
#include <alloca.h>

void
reverse(char *s)
{
    char *t;
    int l;
    int i;
    l = strlen(s);
    t = alloca(l + 1);
    for ( i = 0; i < l; i++ ) {
        t[i] = s[l - i - 1];
    }
    t[l] = 0;

    strcpy(s, t);
}

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

static __inline__ int
hex2bin(char c)
{
    if ( c >= '0' && c <= '9' ) {
        return c - '0';
    } else if ( c >= 'a' && c <= 'f' ) {
        return c - 'a' + 10;
    } else if ( c >= 'A' && c <= 'F' ) {
        return c - 'A' + 10;
    } else {
        return 0;
    }
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
test_acl_perf(enum palmtrie_type type, const char *fname)
{
    struct palmtrie palmtrie;
    FILE *fp;
    char buf[4096];
    char data0[1024];
    char data1[1024];
    int priority;
    int action;
    int ret;
    addr_t addr = PALMTRIE_ADDR_ZERO;
    addr_t mask = PALMTRIE_ADDR_ZERO;
    u64 d;
    long long i;
    ssize_t k;
    addr_t tmp = PALMTRIE_ADDR_ZERO;
    double delta;
    u64 x;

    /* Initialize */
    palmtrie_init(&palmtrie, type);

    /* Load from the linx file */
    fp = fopen(fname, "r");
    if ( NULL == fp ) {
        return -1;
    }

    double t0, t1, t2;
    t0 = getmicrotime();
    while ( !feof(fp) ) {
        if ( !fgets(buf, sizeof(buf), fp) ) {
            continue;
        }
        ret = sscanf(buf, "%1000s %1000s %d %d", data0, data1, &priority,
                     &action);
        if ( ret < 0 ) {
            return -1;
        }
        if ( strlen(data0) != 32 || strlen(data1) != 32 ) {
            /* Length mismatch */
            return -1;
        }
        //reverse(data0);
        //reverse(data1);
        memset(&addr, 0, sizeof(addr_t));
        memset(&mask, 0, sizeof(addr_t));
        for ( k = 0; k < (ssize_t)strlen(data0); k++ ) {
            d = hex2bin(data0[k]);
            k ^= 1;
            addr.a[k >> 4] |= d << ((k & 0xf) << 2);
            k ^= 1;
            d = hex2bin(data1[k]);
            k ^= 1;
            mask.a[k >> 4] |= d << ((k & 0xf) << 2);
            k ^= 1;
        }
        ret = palmtrie_add_data(&palmtrie, addr, mask, priority, action);
        if ( ret < 0 ) {
            return -1;
        }
    }
    t1 = getmicrotime();
    ret = palmtrie_commit(&palmtrie);
    if ( ret < 0 ) {
        return -1;
    }
    t2 = getmicrotime();
    printf("#build %lf %lf\n", t1 - t0, t2 - t1);

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
    for ( g_cnt = 0; g_nrsigs < NRTRIALS; g_cnt++ ) {
        uint32_t *a;
        uint32_t rv;
        rv = xor128();
        tmp.a[0] = 0x01;
        a = (void *)tmp.a + 1;
        *(a + 0) = xor128();
        *(a + 1) = (rv & 0xffffff00) | 0x0a;
        *(a + 2) = xor128();
        *((uint8_t *)tmp.a + 14) = 0x02;
        //tmp.a[0] = xor128();
        //tmp.a[1] = xor128();
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
 * Performance test
 */
static int
test_acl_ross(enum palmtrie_type type, const char *fname, const char *tfname)
{
    struct palmtrie palmtrie;
    FILE *fp;
    char buf[4096];
    char data0[1024];
    char data1[1024];
    int priority;
    int action;
    int ret;
    addr_t addr = PALMTRIE_ADDR_ZERO;
    addr_t mask = PALMTRIE_ADDR_ZERO;
    u64 d;
    long long i;
    ssize_t k;
    addr_t tmp = PALMTRIE_ADDR_ZERO;
    double delta;
    u64 x;
    addr_t *pattern;
    int npkt;
    long long j;

    /* Initialize */
    palmtrie_init(&palmtrie, type);

    /* Load TCAM file */
    fp = fopen(fname, "r");
    if ( NULL == fp ) {
        return -1;
    }

    /* Load the full route */
    double t0, t1, t2;
    t0 = getmicrotime();
    i = 0;
    while ( !feof(fp) ) {
        if ( !fgets(buf, sizeof(buf), fp) ) {
            continue;
        }
        ret = sscanf(buf, "%1000s %1000s %d %d", data0, data1, &priority,
                     &action);
        if ( ret < 0 ) {
            return -1;
        }
        if ( strlen(data0) != 32 || strlen(data1) != 32 ) {
            /* Length mismatch */
            return -1;
        }
        //reverse(data0);
        //reverse(data1);
        memset(&addr, 0, sizeof(addr_t));
        memset(&mask, 0, sizeof(addr_t));
        for ( k = 0; k < (ssize_t)strlen(data0); k++ ) {
            d = hex2bin(data0[k]);
            k ^= 1;
            addr.a[k >> 4] |= d << ((k & 0xf) << 2);
            k ^= 1;
            d = hex2bin(data1[k]);
            k ^= 1;
            mask.a[k >> 4] |= d << ((k & 0xf) << 2);
            k ^= 1;
        }
        ret = palmtrie_add_data(&palmtrie, addr, mask, priority, action);
        if ( ret < 0 ) {
            return -1;
        }
    }
    t1 = getmicrotime();
    ret = palmtrie_commit(&palmtrie);
    if ( ret < 0 ) {
        return -1;
    }
    t2 = getmicrotime();
    printf("#build %lf %lf\n", t1 - t0, t2 - t1);
    if ( type == PALMTRIE_PLUS ) {
        printf("#nodes = %d\n", palmtrie.u.popmtpt.nodes.used);
    }

    /* Close */
    fclose(fp);

    /* Traffic pattern */
    fp = fopen(tfname, "r");
    if ( NULL == fp ) {
        return -1;
    }

    /* Load the full route */
    i = 0;
    pattern = malloc(sizeof(addr_t) * 65536 * 256);
    while ( !feof(fp) ) {
        if ( !fgets(buf, sizeof(buf), fp) ) {
            continue;
        }
        ret = sscanf(buf, "%1000s", data0);
        if ( ret < 0 ) {
            return -1;
        }
        if ( strlen(data0) != 32 ) {
            /* Length mismatch */
            return -1;
        }
        //reverse(data0);
        memset(&tmp, 0, sizeof(addr_t));
        for ( k = 0; k < (ssize_t)strlen(data0); k++ ) {
            d = hex2bin(data0[k]);
            k ^= 1;
            tmp.a[k >> 4] |= d << ((k & 0xf) << 2);
            k ^= 1;
        }
        (void)memcpy(&pattern[i], &tmp, sizeof(addr_t));
        i++;
        if ( i >= 65536 * 256 ) {
            break;
        }
    }
    npkt = i;

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
    j = 0;
    for ( g_cnt = 0; g_nrsigs < NRTRIALS; g_cnt++ ) {
        x ^= palmtrie_lookup(&palmtrie, pattern[j]);
        j++;
        if ( j >= npkt ) {
            j = 0;
        }
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
 * Performance test
 */
struct tmpent {
    addr_t addr;
    addr_t mask;
    int priority;
    int action;
};
static int
test_acl_build(enum palmtrie_type type, const char *fname)
{
    struct palmtrie palmtrie;
    FILE *fp;
    char buf[4096];
    char data0[1024];
    char data1[1024];
    int priority;
    int action;
    int ret;
    addr_t addr = PALMTRIE_ADDR_ZERO;
    addr_t mask = PALMTRIE_ADDR_ZERO;
    u64 d;
    long long i;
    long long n;
    ssize_t k;
    struct tmpent *ents = malloc(sizeof(struct tmpent) * 1024 * 1024 * 128);

    /* Initialize */
    palmtrie_init(&palmtrie, type);

    /* Load TCAM file */
    fp = fopen(fname, "r");
    if ( NULL == fp ) {
        return -1;
    }

    /* Load the full route */
    double t0, t1, t2;
    i = 0;
    while ( !feof(fp) ) {
        if ( !fgets(buf, sizeof(buf), fp) ) {
            continue;
        }
        ret = sscanf(buf, "%1000s %1000s %d %d", data0, data1, &priority,
                     &action);
        if ( ret < 0 ) {
            return -1;
        }
        if ( strlen(data0) != 32 || strlen(data1) != 32 ) {
            /* Length mismatch */
            return -1;
        }
        //reverse(data0);
        //reverse(data1);
        memset(&addr, 0, sizeof(addr_t));
        memset(&mask, 0, sizeof(addr_t));
        for ( k = 0; k < (ssize_t)strlen(data0); k++ ) {
            d = hex2bin(data0[k]);
            k ^= 1;
            addr.a[k >> 4] |= d << ((k & 0xf) << 2);
            k ^= 1;
            d = hex2bin(data1[k]);
            k ^= 1;
            mask.a[k >> 4] |= d << ((k & 0xf) << 2);
            k ^= 1;
        }

        memcpy(&ents[i].addr, &addr, sizeof(addr_t));
        memcpy(&ents[i].mask, &mask, sizeof(addr_t));
        ents[i].priority = priority;
        ents[i].action = action;
        i++;
    }

    t0 = getmicrotime();
    n = i;
    for ( i = 0; i < n; i++ ) {
        ret = palmtrie_add_data(&palmtrie, ents[i].addr, ents[i].mask,
                             ents[i].priority, ents[i].action);
        if ( ret < 0 ) {
            return -1;
        }
    }
    t1 = getmicrotime();
    ret = palmtrie_commit(&palmtrie);
    if ( ret < 0 ) {
        return -1;
    }
    t2 = getmicrotime();
    printf("#build %lf %lf\n", t1 - t0, t2 - t1);

    /* Close */
    fclose(fp);

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
    if ( 0 == strcmp(type, "sl-rand") ) {
        printf("#SL:\n");
        test_acl_perf(PALMTRIE_SORTED_LIST, fname);
    } else if ( 0 == strcmp(type, "tpt-rand") ) {
        printf("#TPT(%d):\n", PALMTRIE_PRIORITY_SKIP);
        test_acl_perf(PALMTRIE_BASIC, fname);
    } else if ( 0 == strcmp(type, "mtpt-rand") ) {
        printf("#MTPT(%d/%d):\n", PALMTRIE_PRIORITY_SKIP, PALMTRIE_MTPT_STRIDE);
        test_acl_perf(PALMTRIE_DEFAULT, fname);
    } else if ( 0 == strcmp(type, "popmtpt-rand") ) {
        printf("#POPMTPT(%d/%d):\n", PALMTRIE_PRIORITY_SKIP, PALMTRIE_MTPT_STRIDE);
        test_acl_perf(PALMTRIE_PLUS, fname);
    } else if ( 0 == strcmp(type, "sl-ross") ) {
        printf("#SL:\n");
        test_acl_ross(PALMTRIE_SORTED_LIST, fname, "tests/acl-1000.ross");
    } else if ( 0 == strcmp(type, "tpt-ross") ) {
        printf("#TPT(%d):\n", PALMTRIE_PRIORITY_SKIP);
        test_acl_ross(PALMTRIE_BASIC, fname, "tests/acl-1000.ross");
    } else if ( 0 == strcmp(type, "mtpt-ross") ) {
        printf("#MTPT(%d/%d):\n", PALMTRIE_PRIORITY_SKIP, PALMTRIE_MTPT_STRIDE);
        test_acl_ross(PALMTRIE_DEFAULT, fname,
                      "tests/acl-1000.ross");
    } else if ( 0 == strcmp(type, "popmtpt-ross") ) {
        printf("#POPMTPT(%d/%d):\n", PALMTRIE_PRIORITY_SKIP, PALMTRIE_MTPT_STRIDE);
        test_acl_ross(PALMTRIE_PLUS, fname,
                      "tests/acl-1000.ross");
    } else if ( 0 == strcmp(type, "sl-sfl") ) {
        printf("#SL:\n");
        test_acl_ross(PALMTRIE_SORTED_LIST, fname, "tests/traffic.sfl2");
    } else if ( 0 == strcmp(type, "tpt-sfl") ) {
        printf("#TPT(%d):\n", PALMTRIE_PRIORITY_SKIP);
        test_acl_ross(PALMTRIE_BASIC, fname, "tests/traffic.sfl2");
    } else if ( 0 == strcmp(type, "mtpt-sfl") ) {
        printf("#MTPT(%d/%d):\n", PALMTRIE_PRIORITY_SKIP, PALMTRIE_MTPT_STRIDE);
        test_acl_ross(PALMTRIE_DEFAULT, fname,
                      "tests/traffic.sfl2");
    } else if ( 0 == strcmp(type, "popmtpt-sfl") ) {
        printf("#POPMTPT(%d/%d):\n", PALMTRIE_PRIORITY_SKIP, PALMTRIE_MTPT_STRIDE);
        test_acl_ross(PALMTRIE_PLUS, fname,
                      "tests/traffic.sfl2");
    } else if ( 0 == strcmp(type, "sl-traffic") ) {
        printf("#SL:\n");
        test_acl_ross(PALMTRIE_SORTED_LIST, fname, "tests/traffic.tmp");
    } else if ( 0 == strcmp(type, "tpt-traffic") ) {
        printf("#TPT(%d):\n", PALMTRIE_PRIORITY_SKIP);
        test_acl_ross(PALMTRIE_BASIC, fname, "tests/traffic.tmp");
    } else if ( 0 == strcmp(type, "mtpt-traffic") ) {
        printf("#MTPT(%d/%d):\n", PALMTRIE_PRIORITY_SKIP, PALMTRIE_MTPT_STRIDE);
        test_acl_ross(PALMTRIE_DEFAULT, fname,
                      "tests/traffic.tmp");
    } else if ( 0 == strcmp(type, "popmtpt-traffic") ) {
        printf("#POPMTPT(%d/%d):\n", PALMTRIE_PRIORITY_SKIP, PALMTRIE_MTPT_STRIDE);
        test_acl_ross(PALMTRIE_PLUS, fname,
                      "tests/traffic.tmp");
    } else if ( 0 == strcmp(type, "tpt-build") ) {
        printf("#TPT(%d):\n", PALMTRIE_PRIORITY_SKIP);
        test_acl_build(PALMTRIE_BASIC, fname);
    } else if ( 0 == strcmp(type, "mtpt-build") ) {
        printf("#MTPT(%d/%d):\n", PALMTRIE_PRIORITY_SKIP, PALMTRIE_MTPT_STRIDE);
        test_acl_build(PALMTRIE_DEFAULT, fname);
    } else if ( 0 == strcmp(type, "popmtpt-build") ) {
        printf("#POPMTPT(%d/%d):\n", PALMTRIE_PRIORITY_SKIP, PALMTRIE_MTPT_STRIDE);
        test_acl_build(PALMTRIE_PLUS, fname);
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
