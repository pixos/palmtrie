/*_
 * Copyright (c) 2018-2020 Hirochika Asai <asai@jar.jp>
 * All rights reserved.
 */

#include "../palmtrie.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

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

/* Macro for testing */
#define TEST_FUNC(str, func, ret)                \
    do {                                         \
        printf("%s: ", str);                     \
        if ( 0 == func() ) {                     \
            printf("passed");                    \
        } else {                                 \
            printf("failed");                    \
            ret = -1;                            \
        }                                        \
        printf("\n");                            \
    } while ( 0 )

#define TEST_PROGRESS()                              \
    do {                                             \
        printf(".");                                 \
        fflush(stdout);                              \
    } while ( 0 )

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

/*
 * Performance test
 */
static int
test_microperf(enum palmtrie_type type, const char *fname, long long nr)
{
    struct palmtrie palmtrie;
    FILE *fp;
    char buf[4096];
    char data0[1024];
    char data1[1024];
    int priority;
    int action;
    int ret;
    addr_t addr = {0, {0, 0, 0, 0, 0, 0, 0, 0}};
    addr_t mask = {0, {0, 0, 0, 0, 0, 0, 0, 0}};
    u64 d;
    long long i;
    ssize_t k;
    addr_t tmp = {0, {0, 0, 0, 0, 0, 0, 0, 0}};
    double t0;
    double t1;
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
        reverse(data0);
        reverse(data1);
        memset(&addr, 0, sizeof(addr_t));
        memset(&mask, 0, sizeof(addr_t));
        for ( k = 0; k < (ssize_t)strlen(data0); k++ ) {
            d = hex2bin(data0[k]);
            addr.a[k >> 4] |= d << ((k & 0xf) << 2);
            d = hex2bin(data1[k]);
            mask.a[k >> 4] |= d << ((k & 0xf) << 2);
        }
        ret = palmtrie_add_data(&palmtrie, addr, mask, priority, action);
        if ( ret < 0 ) {
            return -1;
        }
        if ( 0 == i % 100000 ) {
            TEST_PROGRESS();
        }
        i++;
    }
    ret = palmtrie_commit(&palmtrie);
    if ( ret < 0 ) {
        return -1;
    }

    /* Close */
    fclose(fp);

    /* Benchmark */
    x = 0;
    t0 = getmicrotime();
    for ( i = 0; i < nr; i++ ) {
        if ( 0 == i % (nr / 8) ) {
            TEST_PROGRESS();
        }
        tmp.a[0] = xor128();
        tmp.a[1] = xor128();
        x ^= palmtrie_lookup(&palmtrie, tmp);
    }
    t1 = getmicrotime();
    (void)x;
    delta = t1 - t0;
    printf("(%.6lf sec: %.3lf Mlookup/sec)", delta, i / delta / 1000 / 1000);

#if 0
    if ( type == PALMTRIE_MULTIWAY_TERNARY_PATRICIA ) {
        void *t;
        t = palmtrie_popmtpt_convert(&palmtrie.u.mtpt);

        x = 0;
        t0 = getmicrotime();
        for ( i = 0; i < nr; i++ ) {
            if ( 0 == i % (nr / 8) ) {
                TEST_PROGRESS();
            }
            tmp.a[0] = xor128();
            tmp.a[1] = xor128();
            x ^= (uint64_t)palmtrie_popmtpt_lookup(t, tmp);
        }
        t1 = getmicrotime();
        (void)x;
        delta = t1 - t0;
        printf("(%.6lf sec: %.3lf Mlookup/sec)", delta, i / delta / 1000 / 1000);
    }
#endif
    return 0;
}
static int
test_microperf_sl(void)
{
    return test_microperf(PALMTRIE_SORTED_LIST, "tests/acl-0002.tcam",
                          0x100000ULL);
}
static int
test_microperf_tpt(void)
{
    return test_microperf(PALMTRIE_TERNARY_PATRICIA, "tests/acl-0002.tcam",
                          0x1000000ULL);
}
static int
test_microperf_mtpt(void)
{
    return test_microperf(PALMTRIE_MULTIWAY_TERNARY_PATRICIA,
                          "tests/acl-0002.tcam", 0x1000000ULL);
}
static int
test_microperf_popmtpt(void)
{
    return test_microperf(PALMTRIE_MULTIWAY_TERNARY_PATRICIA_OPT,
                          "tests/acl-0002.tcam", 0x1000000ULL);
}

/*
 * Performance test
 */
static int
test_rossperf(enum palmtrie_type type, const char *fname, const char *tfname,
              long long nr)
{
    struct palmtrie palmtrie;
    FILE *fp;
    char buf[4096];
    char data0[1024];
    char data1[1024];
    int priority;
    int action;
    int ret;
    addr_t addr = {0, {0, 0, 0, 0, 0, 0, 0, 0}};
    addr_t mask = {0, {0, 0, 0, 0, 0, 0, 0, 0}};
    u64 d;
    long long i;
    ssize_t k;
    addr_t tmp = {0, {0, 0, 0, 0, 0, 0, 0, 0}};
    double t0;
    double t1;
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
        reverse(data0);
        reverse(data1);
        memset(&addr, 0, sizeof(addr_t));
        memset(&mask, 0, sizeof(addr_t));
        for ( k = 0; k < (ssize_t)strlen(data0); k++ ) {
            d = hex2bin(data0[k]);
            addr.a[k >> 4] |= d << ((k & 0xf) << 2);
            d = hex2bin(data1[k]);
            mask.a[k >> 4] |= d << ((k & 0xf) << 2);
        }
        ret = palmtrie_add_data(&palmtrie, addr, mask, priority, action);
        if ( ret < 0 ) {
            return -1;
        }
        if ( 0 == i % 100000 ) {
            TEST_PROGRESS();
        }
        i++;
    }
    ret = palmtrie_commit(&palmtrie);
    if ( ret < 0 ) {
        return -1;
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
        reverse(data0);
        if ( strlen(data0) != 32 ) {
            /* Length mismatch */
            return -1;
        }
        memset(&tmp, 0, sizeof(addr_t));
        for ( k = 0; k < (ssize_t)strlen(data0); k++ ) {
            d = hex2bin(data0[k]);
            tmp.a[k >> 4] |= d << ((k & 0xf) << 2);
        }
        (void)memcpy(&pattern[i], &tmp, sizeof(addr_t));
        if ( 0 == i % 1000000 ) {
            TEST_PROGRESS();
        }
        i++;
        if ( i >= 65536 * 256 ) {
            break;
        }
    }
    npkt = i;

    /* Close */
    fclose(fp);

    /* Benchmark */
    x = 0;
    t0 = getmicrotime();
    for ( i = 0, j = 0; i < nr; i++ ) {
        if ( 0 == i % (nr / 8) ) {
            TEST_PROGRESS();
        }
        x ^= palmtrie_lookup(&palmtrie, pattern[j]);
        j++;
        if ( j >= npkt ) {
            j = 0;
        }
    }
    t1 = getmicrotime();
    (void)x;
    delta = t1 - t0;
    printf("(%.6lf sec: %.3lf Mlookup/sec)", delta, i / delta / 1000 / 1000);

#if 0
    if ( type == PALMTRIE_MULTIWAY_TERNARY_PATRICIA ) {
        gv(palmtrie.u.mtpt.root);
    }
#endif
#if 0
    if ( type == PALMTRIE_MULTIWAY_TERNARY_PATRICIA ) {
        void *t;
        t = palmtrie_popmtpt_convert(&palmtrie.u.mtpt);

        x = 0;
        t0 = getmicrotime();
        for ( i = 0, j = 0; i < nr; i++ ) {
            if ( 0 == i % (nr / 8) ) {
                TEST_PROGRESS();
            }
            //x ^= (uint64_t)palmtrie_mtpt_lookup(&palmtrie, pattern[j]);
            x ^= (uint64_t)palmtrie_popmtpt_lookup(t, pattern[j]);
            j++;
            if ( j >= npkt ) {
                j = 0;
            }
        }
        t1 = getmicrotime();
        (void)x;
        delta = t1 - t0;
        printf("(%.6lf sec: %.3lf Mlookup/sec)", delta, i / delta / 1000 / 1000);
    }
#endif
    return 0;
}
static int
test_rossperf_sl(void)
{
    return test_rossperf(PALMTRIE_SORTED_LIST, "tests/acl-0001.tcam",
                         "tests/acl-0001.ross", 0x100000ULL);
}
static int
test_rossperf_tpt(void)
{
    return test_rossperf(PALMTRIE_TERNARY_PATRICIA, "tests/acl-0001.tcam",
                         "tests/acl-0001.ross",
                         0x1000000ULL);
}
static int
test_rossperf_mtpt(void)
{
    return test_rossperf(PALMTRIE_MULTIWAY_TERNARY_PATRICIA, "tests/acl-0001.tcam",
                         "tests/acl-0001.ross", 0x1000000ULL);
}
static int
test_rossperf_popmtpt(void)
{
    return test_rossperf(PALMTRIE_MULTIWAY_TERNARY_PATRICIA_OPT,
                         "tests/acl-0001.tcam", "tests/acl-0001.ross",
                         0x1000000ULL);
}
static int
test_rossperf2_sl(void)
{
    return test_rossperf(PALMTRIE_SORTED_LIST, "tests/acl-0002.tcam",
                         "tests/acl-0001.ross", 0x100000ULL);
}
static int
test_rossperf2_tpt(void)
{
    return test_rossperf(PALMTRIE_TERNARY_PATRICIA, "tests/acl-0002.tcam",
                         "tests/acl-0001.ross", 0x1000000ULL);
}
static int
test_rossperf2_mtpt(void)
{
    return test_rossperf(PALMTRIE_MULTIWAY_TERNARY_PATRICIA, "tests/acl-0002.tcam",
                         "tests/acl-0001.ross", 0x1000000ULL);
}
static int
test_rossperf2_popmtpt(void)
{
    return test_rossperf(PALMTRIE_MULTIWAY_TERNARY_PATRICIA_OPT,
                         "tests/acl-0002.tcam", "tests/acl-0001.ross",
                         0x1000000ULL);
}
static int
test_rossperf3_sl(void)
{
    return test_rossperf(PALMTRIE_SORTED_LIST, "tests/acl-1000.tcam",
                         "tests/acl-1000.ross", 0x100000ULL);
}
static int
test_rossperf3_tpt(void)
{
    return test_rossperf(PALMTRIE_TERNARY_PATRICIA, "tests/acl-1000.tcam",
                         "tests/acl-1000.ross", 0x1000000ULL);
}
static int
test_rossperf3_mtpt(void)
{
    return test_rossperf(PALMTRIE_MULTIWAY_TERNARY_PATRICIA, "tests/acl-1000.tcam",
                         "tests/acl-1000.ross", 0x1000000ULL);
}
static int
test_rossperf3_popmtpt(void)
{
    return test_rossperf(PALMTRIE_MULTIWAY_TERNARY_PATRICIA_OPT,
                         "tests/acl-1000.tcam", "tests/acl-1000.ross",
                         0x1000000ULL);
}

/*
 * Main routine for the basic test
 */
int
main(int argc, const char *const argv[])
{
    int ret;
    int flags;
    int i;

    ret = 0;

    flags = 0;
    if ( argc < 2 ) {
        /* Test all */
        flags = 0xff;
    } else {
        for ( i = 1; i < argc; i++ ) {
            if ( 0 == strcmp("microperf", argv[i]) ) {
                flags |= 1 << 1;
            } else if ( 0 == strcmp("rossperf1", argv[i]) ) {
                flags |= 1 << 2;
            } else if ( 0 == strcmp("rossperf2", argv[i]) ) {
                flags |= 1 << 3;
            } else if ( 0 == strcmp("rossperf3", argv[i]) ) {
                flags |= 1 << 4;
            }
        }
    }

    /* Run tests */
    if ( flags & (1 << 1) ) {
        /* Micro performance measurement */
        TEST_FUNC("microperf (sl)", test_microperf_sl, ret);
        TEST_FUNC("microperf (tpt)", test_microperf_tpt, ret);
        TEST_FUNC("microperf (mtpt)", test_microperf_mtpt, ret);
        TEST_FUNC("microperf (popmtpt)", test_microperf_popmtpt, ret);
    }
    if ( flags & (1 << 2) ) {
        /* Micro performance measurement */
        TEST_FUNC("rossperf1 (sl)", test_rossperf_sl, ret);
        TEST_FUNC("rossperf1 (tpt)", test_rossperf_tpt, ret);
        TEST_FUNC("rossperf1 (mtpt)", test_rossperf_mtpt, ret);
        TEST_FUNC("rossperf1 (popmtpt)", test_rossperf_popmtpt, ret);
    }
    if ( flags & (1 << 3) ) {
        /* Micro performance measurement */
        TEST_FUNC("rossperf2 (sl)", test_rossperf2_sl, ret);
        TEST_FUNC("rossperf2 (tpt)", test_rossperf2_tpt, ret);
        TEST_FUNC("rossperf2 (mtpt)", test_rossperf2_mtpt, ret);
        TEST_FUNC("rossperf2 (popmtpt)", test_rossperf2_popmtpt, ret);
    }
    if ( flags & (1 << 4) ) {
        /* Micro performance measurement */
        //TEST_FUNC("rossperf3 (sl)", test_rossperf3_sl, ret);
        TEST_FUNC("rossperf3 (tpt)", test_rossperf3_tpt, ret);
        TEST_FUNC("rossperf3 (mtpt)", test_rossperf3_mtpt, ret);
        TEST_FUNC("rossperf3 (popmtpt)", test_rossperf3_popmtpt, ret);
    }

    return ret;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
