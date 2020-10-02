/*_
 * Copyright (c) 2015-2020 Hirochika Asai <asai@jar.jp>
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
        fflush(stdout);                          \
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
 * Test
 */
static int
test_true(enum palmtrie_type type)
{
    struct palmtrie palmtrie;
    int ret;
    addr_t addrs[] = {
        {0, {0x5, 0, 0, 0, 0, 0, 0, 0}}, {0, {0, 0, 0, 0, 0, 0, 0, 0}},
        {0, {0x3, 0, 0, 0, 0, 0, 0, 0}}, {0, {0, 0, 0, 0, 0, 0, 0, 0}},
        {0, {0x0, 0, 0, 0, 0, 0, 0, 0}}, {0, {1, 0, 0, 0, 0, 0, 0, 0}},
        {0, {0x0, 0, 0, 0, 0, 0, 0, 0}}, {0, {0xffffffffULL, 0, 0, 0, 0, 0, 0, 0}},
        {0, {0xcbb20c00ULL, 0, 0, 0, 0, 0, 0, 0}}, {0, {0xff, 0, 0, 0, 0, 0, 0, 0}},
        {0, {0xcbb22100ULL, 0, 0, 0, 0, 0, 0, 0}}, {0, {0xff, 0, 0, 0, 0, 0, 0, 0}},
        {0, {0xcbb22400ULL, 0, 0, 0, 0, 0, 0, 0}}, {0, {0x3ff, 0, 0, 0, 0, 0, 0, 0}},
        {0, {0xcbb22c00ULL, 0, 0, 0, 0, 0, 0, 0}}, {0, {0xff, 0, 0, 0, 0, 0, 0, 0}},
        {0, {0xcbb24000ULL, 0, 0, 0, 0, 0, 0, 0}}, {0, {0x3fff, 0, 0, 0, 0, 0, 0, 0}},
        {0, {0xcbb24e00ULL, 0, 0, 0, 0, 0, 0, 0}}, {0, {0xff, 0, 0, 0, 0, 0, 0, 0}},
        {0, {0xcbb25600ULL, 0, 0, 0, 0, 0, 0, 0}}, {0, {0xff, 0, 0, 0, 0, 0, 0, 0}},
        {0, {0xcbb25b00ULL, 0, 0, 0, 0, 0, 0, 0}}, {0, {0xff, 0, 0, 0, 0, 0, 0, 0}},
        {0, {0xcbb25c00ULL, 0, 0, 0, 0, 0, 0, 0}}, {0, {0xff, 0, 0, 0, 0, 0, 0, 0}},
        {0, {0xcbb27c00ULL, 0, 0, 0, 0, 0, 0, 0}}, {0, {0xff, 0, 0, 0, 0, 0, 0, 0}},
        {0, {0xcbb27f00ULL, 0, 0, 0, 0, 0, 0, 0}}, {0, {0xff, 0, 0, 0, 0, 0, 0, 0}},
        {0, {0xcbb28000ULL, 0, 0, 0, 0, 0, 0, 0}}, {0, {0x7fff, 0, 0, 0, 0, 0, 0, 0}},
    };
    addr_t tests[] = {
        {0, {0xcbb24001ULL, 0, 0, 0, 0, 0, 0, 0}},
        {0, {0xcbb26001ULL, 0, 0, 0, 0, 0, 0, 0}},
        {0, {0xcbb28701ULL, 0, 0, 0, 0, 0, 0, 0}},
        {0, {0x3, 0, 0, 0, 0, 0, 0, 0}},
        {0, {0x5, 0, 0, 0, 0, 0, 0, 0}},
        {0, {0x2, 0, 0, 0, 0, 0, 0, 0}},
        {0, {0x1, 0, 0, 0, 0, 0, 0, 0}},
        {0, {0x0, 0, 0, 0, 0, 0, 0, 0}},
    };

    palmtrie_init(&palmtrie, type);

    ret = palmtrie_add_data(&palmtrie, addrs[0], addrs[1], 2, 5);
    if ( ret < 0 ) {
        return -1;
    }
    ret = palmtrie_add_data(&palmtrie, addrs[2], addrs[3], 2, 3);
    if ( ret < 0 ) {
        return -1;
    }
    ret = palmtrie_add_data(&palmtrie, addrs[4], addrs[5], 1, 2);
    if ( ret < 0 ) {
        return -1;
    }
    ret = palmtrie_add_data(&palmtrie, addrs[6], addrs[7], 0, 9);
    if ( ret < 0 ) {
        return -1;
    }
    ret = palmtrie_add_data(&palmtrie, addrs[8], addrs[9], 24, 11);
    if ( ret < 0 ) {
        return -1;
    }
    ret = palmtrie_add_data(&palmtrie, addrs[10], addrs[11], 24, 12);
    if ( ret < 0 ) {
        return -1;
    }
    ret = palmtrie_add_data(&palmtrie, addrs[12], addrs[13], 22, 13);
    if ( ret < 0 ) {
        return -1;
    }
    ret = palmtrie_add_data(&palmtrie, addrs[14], addrs[15], 24, 14);
    if ( ret < 0 ) {
        return -1;
    }
    ret = palmtrie_add_data(&palmtrie, addrs[16], addrs[17], 18, 15);
    if ( ret < 0 ) {
        return -1;
    }
    ret = palmtrie_add_data(&palmtrie, addrs[18], addrs[19], 24, 16);
    if ( ret < 0 ) {
        return -1;
    }
    ret = palmtrie_add_data(&palmtrie, addrs[20], addrs[21], 24, 17);
    if ( ret < 0 ) {
        return -1;
    }
    ret = palmtrie_add_data(&palmtrie, addrs[22], addrs[23], 24, 18);
    if ( ret < 0 ) {
        return -1;
    }
    ret = palmtrie_add_data(&palmtrie, addrs[24], addrs[25], 24, 19);
    if ( ret < 0 ) {
        return -1;
    }
    ret = palmtrie_add_data(&palmtrie, addrs[26], addrs[27], 24, 20);
    if ( ret < 0 ) {
        return -1;
    }
    ret = palmtrie_add_data(&palmtrie, addrs[28], addrs[29], 24, 21);
    if ( ret < 0 ) {
        return -1;
    }
    ret = palmtrie_add_data(&palmtrie, addrs[30], addrs[31], 17, 22);
    if ( ret < 0 ) {
        return -1;
    }
    ret = palmtrie_commit(&palmtrie);
    if ( ret < 0 ) {
        return -1;
    }

    if ( 15 != palmtrie_lookup(&palmtrie, tests[0]) ) {
        return -1;
    }
    if ( 15 != palmtrie_lookup(&palmtrie, tests[1]) ) {
        return -1;
    }
    if ( 22 != palmtrie_lookup(&palmtrie, tests[2]) ) {
        return -1;
    }
    if ( 3 != palmtrie_lookup(&palmtrie, tests[3]) ) {
        return -1;
    }
    if ( 5 != palmtrie_lookup(&palmtrie, tests[4]) ) {
        return -1;
    }
    if ( 9 != palmtrie_lookup(&palmtrie, tests[5]) ) {
        return -1;
    }
    if ( 2 != palmtrie_lookup(&palmtrie, tests[6]) ) {
        return -1;
    }
    if ( 2 != palmtrie_lookup(&palmtrie, tests[7]) ) {
        return -1;
    }

    return 0;
}
static int
test_true_sl(void)
{
    return test_true(PALMTRIE_SORTED_LIST);
}
static int
test_true_tpt(void)
{
    return test_true(PALMTRIE_TERNARY_PATRICIA);
}
static int
test_true_mtpt(void)
{
    return test_true(PALMTRIE_MULTIWAY_TERNARY_PATRICIA);
}
static int
test_true_popmtpt(void)
{
    return test_true(PALMTRIE_MULTIWAY_TERNARY_PATRICIA_OPT);
}

/*
 * Performance test
 */
static int
test_microperf(enum palmtrie_type type, long long nr)
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
    double t0;
    double t1;
    double delta;
    u64 x;

    /* Initialize */
    palmtrie_init(&palmtrie, type);

    /* Load from the linx file */
    fp = fopen("tests/linx-rib.20141217.0000-p46.sorted.txt", "r");
    if ( NULL == fp ) {
        return -1;
    }

    /* Load the full route */
    i = 0;
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
        x ^= palmtrie_lookup(&palmtrie, tmp);
    }
    t1 = getmicrotime();
    (void)x;
    delta = t1 - t0;
    printf("(%.6lf sec: %.3lf Mlookup/sec)", delta, i / delta / 1000 / 1000);

    return 0;
}
static int
test_microperf_sl(void)
{
    return test_microperf(PALMTRIE_SORTED_LIST, 0x1000ULL);
}
static int
test_microperf_tpt(void)
{
    return test_microperf(PALMTRIE_TERNARY_PATRICIA, 0x1000000ULL);
}
static int
test_microperf_mtpt(void)
{
    return test_microperf(PALMTRIE_MULTIWAY_TERNARY_PATRICIA, 0x1000000ULL);
}

/*
 * Performance test
 */
static int
test_lookup_linx(enum palmtrie_type type1, enum palmtrie_type type2)
{
    struct palmtrie palmtrie0;
    struct palmtrie palmtrie1;
    FILE *fp;
    char buf[4096];
    int prefix[4];
    int prefixlen;
    int nexthop[4];
    int ret;
    addr_t addr1 = {0, {0, 0, 0, 0, 0, 0, 0, 0}};
    addr_t mask = {0, {0, 0, 0, 0, 0, 0, 0, 0}};
    u64 addr2;
    u64 i;
    addr_t tmp = {0, {0, 0, 0, 0, 0, 0, 0, 0}};

    /* Initialize */
    palmtrie_init(&palmtrie0, type1);
    palmtrie_init(&palmtrie1, type2);

    /* Load from the linx file */
    fp = fopen("tests/linx-rib.20141217.0000-p46.sorted.txt", "r");
    if ( NULL == fp ) {
        return -1;
    }

    /* Load the full route */
    i = 0;
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
        ret = palmtrie_add_data(&palmtrie0, addr1, mask, prefixlen, addr2);
        if ( ret < 0 ) {
            return -1;
        }
        ret = palmtrie_add_data(&palmtrie1, addr1, mask, prefixlen, addr2);
        if ( ret < 0 ) {
            return -1;
        }
        if ( 0 == i % 10000 ) {
            TEST_PROGRESS();
        }
        i++;
    }

    ret = palmtrie_commit(&palmtrie0);
    if ( ret < 0 ) {
        return -1;
    }
    ret = palmtrie_commit(&palmtrie1);
    if ( ret < 0 ) {
        return -1;
    }

    /* Compare with the sorted list */
    for ( i = 0; i < 0x1000ULL; i++ ) {
        if ( 0 == i % 0x100ULL ) {
            TEST_PROGRESS();
        }
        tmp.a[0] = xor128();
        if ( palmtrie_lookup(&palmtrie0, tmp) != palmtrie_lookup(&palmtrie1, tmp) ) {
            return -1;
        }
    }

    /* Close */
    fclose(fp);

    return 0;
}
static int
test_cross_sl_tpt(void)
{
    return test_lookup_linx(PALMTRIE_SORTED_LIST, PALMTRIE_TERNARY_PATRICIA);
}
static int
test_cross_tpt_mtpt(void)
{
    return test_lookup_linx(PALMTRIE_TERNARY_PATRICIA,
                            PALMTRIE_MULTIWAY_TERNARY_PATRICIA);
}

/*
 * Cross testing
 */
static int
test_acl_cross(enum palmtrie_type type1, enum palmtrie_type type2)
{
    struct palmtrie palmtrie0;
    struct palmtrie palmtrie1;
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

    /* Initialize */
    palmtrie_init(&palmtrie0, type1);
    palmtrie_init(&palmtrie1, type2);

    /* Load from the linx file */
    fp = fopen("tests/acl-0002.tcam", "r");
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

        /* Add an entry */
        ret = palmtrie_add_data(&palmtrie0, addr, mask, priority, action);
        if ( ret < 0 ) {
            return -1;
        }
        ret = palmtrie_add_data(&palmtrie1, addr, mask, priority, action);
        if ( ret < 0 ) {
            return -1;
        }
        if ( 0 == i % 10000 ) {
            TEST_PROGRESS();
        }
        i++;
    }

    ret = palmtrie_commit(&palmtrie0);
    if ( ret < 0 ) {
        return -1;
    }
    ret = palmtrie_commit(&palmtrie1);
    if ( ret < 0 ) {
        return -1;
    }

    /* Compare with the sorted list */
    for ( i = 0; i < 0x100000; i++ ) {
        if ( 0 == i % 0x10000 ) {
            TEST_PROGRESS();
        }
        tmp.a[0] = xor128();
        tmp.a[1] = xor128();
        if ( palmtrie_lookup(&palmtrie0, tmp) != palmtrie_lookup(&palmtrie1, tmp) ) {
            return -1;
        }
    }

    /* Close */
    fclose(fp);

    return 0;
}
static int
test_acl_cross_sl_tpt(void)
{
    return test_acl_cross(PALMTRIE_SORTED_LIST, PALMTRIE_TERNARY_PATRICIA);
}
static int
test_acl_cross_tpt_mtpt(void)
{
    return test_acl_cross(PALMTRIE_TERNARY_PATRICIA,
                          PALMTRIE_MULTIWAY_TERNARY_PATRICIA);
}

/*
 * ACL test
 */
static int
test_acl_cross_ross(enum palmtrie_type type1, enum palmtrie_type type2)
{
    struct palmtrie palmtrie0;
    struct palmtrie palmtrie1;
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
    palmtrie_init(&palmtrie0, type1);
    palmtrie_init(&palmtrie1, type2);

    /* Load TCAM file */
    fp = fopen("tests/acl-0001.tcam", "r");
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
        ret = palmtrie_add_data(&palmtrie0, addr, mask, priority, action);
        if ( ret < 0 ) {
            return -1;
        }
        ret = palmtrie_add_data(&palmtrie1, addr, mask, priority, action);
        if ( ret < 0 ) {
            return -1;
        }
        if ( 0 == i % 100000 ) {
            TEST_PROGRESS();
        }
        i++;
    }

    ret = palmtrie_commit(&palmtrie0);
    if ( ret < 0 ) {
        return -1;
    }
    ret = palmtrie_commit(&palmtrie1);
    if ( ret < 0 ) {
        return -1;
    }

    /* Close */
    fclose(fp);

    /* Traffic pattern */
    fp = fopen("./tests/acl-0001.ross", "r");
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
        reverse(data0);
        memset(&tmp, 0, sizeof(addr_t));
        for ( k = 0; k < (ssize_t)strlen(data0); k++ ) {
            d = hex2bin(data0[k]);
            tmp.a[k >> 4] |= d << ((k & 0xf) << 2);
        }
        (void)memcpy(&pattern[i], &tmp, sizeof(addr_t));
        if ( 0 == i % 100000 ) {
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
    for ( i = 0, j = 0; i < npkt; i++ ) {
        if ( 0 == i % (npkt / 8) ) {
            TEST_PROGRESS();
        }
        if ( palmtrie_lookup(&palmtrie0, pattern[j])
             != palmtrie_lookup(&palmtrie1, pattern[j]) ) {
            return -1;
        }
        j++;
        if ( j >= npkt ) {
            j = 0;
        }
    }
    t1 = getmicrotime();
    (void)x;
    delta = t1 - t0;

    return 0;
}
static int
test_acl_cross_ross_tpt_mtpt(void)
{
    return test_acl_cross_ross(PALMTRIE_TERNARY_PATRICIA,
                               PALMTRIE_MULTIWAY_TERNARY_PATRICIA);
}
static int
test_acl_cross_ross_tpt_popmtpt(void)
{
    return test_acl_cross_ross(PALMTRIE_TERNARY_PATRICIA,
                               PALMTRIE_MULTIWAY_TERNARY_PATRICIA_OPT);
}
static int
test_acl_cross_ross_sl_tpt(void)
{
    return test_acl_cross_ross(PALMTRIE_SORTED_LIST,
                               PALMTRIE_TERNARY_PATRICIA);
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
            if ( 0 == strcmp("basic", argv[i]) ) {
                flags |= 1 << 0;
            } else if ( 0 == strcmp("microperf", argv[i]) ) {
                flags |= 1 << 1;
            } else if ( 0 == strcmp("cross", argv[i]) ) {
                flags |= 1 << 2;
            } else if ( 0 == strcmp("cross-acl", argv[i]) ) {
                flags |= 1 << 3;
            }
        }
    }

    /* Run tests */
    if ( flags & (1 << 0) ) {
        /* Basic test */
        TEST_FUNC("basic (sl)", test_true_sl, ret);
        TEST_FUNC("basic (tpt)", test_true_tpt, ret);
        TEST_FUNC("basic (mtpt)", test_true_mtpt, ret);
        TEST_FUNC("basic (popmtpt)", test_true_popmtpt, ret);
    }
    if ( flags & (1 << 1) ) {
        /* Micro performance measurement */
        TEST_FUNC("microperf (sl)", test_microperf_sl, ret);
        TEST_FUNC("microperf (tpt)", test_microperf_tpt, ret);
        TEST_FUNC("microperf (mtpt)", test_microperf_mtpt, ret);
    }
    if ( flags & (1 << 2) ) {
        /* Cross validation */
        TEST_FUNC("cross (tpt,mtpt)", test_cross_tpt_mtpt, ret);
        TEST_FUNC("cross (sl,tpt)", test_cross_sl_tpt, ret);
    }
    if ( flags & (1 << 3) ) {
        /* Cross validation */
        TEST_FUNC("cross-acl (tpt,mtpt)", test_acl_cross_tpt_mtpt, ret);
        TEST_FUNC("cross-acl-ross (tpt,mtpt)", test_acl_cross_ross_tpt_mtpt,
                  ret);
        TEST_FUNC("cross-acl-ross (tpt,popmtpt)",
                  test_acl_cross_ross_tpt_popmtpt, ret);
        TEST_FUNC("cross-acl (sl,tpt)", test_acl_cross_sl_tpt, ret);
        TEST_FUNC("cross-acl-ross (sl,tpt)", test_acl_cross_ross_sl_tpt,
                  ret);
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
