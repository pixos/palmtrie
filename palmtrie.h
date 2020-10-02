/*_
 * Copyright (c) 2015-2020 Hirochika Asai <asai@jar.jp>
 * All rights reserved.
 */

#ifndef _PALMTRIE_H
#define _PALMTRIE_H

#include <stdint.h>
#include <immintrin.h>

typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/* Typical TCAM key-width: 40, 80, 160, 320, 480, 640 bits */

/* 512-bit addressing */
#if defined(PALMTRIE_SHORT) && PALMTRIE_SHORT
typedef struct { u32 g; u64 a[2]; } __attribute__ ((packed)) addr_t;
#define PALMTRIE_ADDR_BITS     480
#define PALMTRIE_ADDR_ZERO     {0, {0, 0}}

#define ADDR_MASK(addr, mask) do {              \
        (addr).a[0] &= ~(mask).a[0];            \
        (addr).a[1] &= ~(mask).a[1];            \
    } while ( 0 )

#define ADDR_MASK_CMP(addr0, mask0, addr1, mask1)                       \
    (((addr0).a[0] & ~(mask0).a[0]) == ((addr1).a[0] & ~(mask1).a[0])   \
     && ((addr0).a[1] & ~(mask0).a[1]) == ((addr1).a[1] & ~(mask1).a[1]))

#define ADDR_MASK_CMP2(addr0, mask0, addr1)                             \
    (((addr0).a[0] & ~(mask0).a[0]) == ((addr1).a[0])                   \
     && ((addr0).a[1] & ~(mask0).a[1]) == ((addr1).a[1]))

#define ADDR_CMP(x, y)                                                  \
    ((x).a[0] == (y).a[0] && (x).a[1] == (y).a[1])

#else

typedef struct { u32 g; u64 a[8]; } __attribute__ ((packed)) addr_t;
#define PALMTRIE_ADDR_BITS     480
#define PALMTRIE_ADDR_ZERO     {0, {0, 0, 0, 0, 0, 0, 0, 0}}

#define ADDR_MASK(addr, mask) do {              \
        (addr).a[0] &= ~(mask).a[0];            \
        (addr).a[1] &= ~(mask).a[1];            \
        (addr).a[2] &= ~(mask).a[2];            \
        (addr).a[3] &= ~(mask).a[3];            \
        (addr).a[4] &= ~(mask).a[4];            \
        (addr).a[5] &= ~(mask).a[5];            \
        (addr).a[6] &= ~(mask).a[6];            \
        (addr).a[7] &= ~(mask).a[7];            \
    } while ( 0 )

#define ADDR_MASK_CMP(addr0, mask0, addr1, mask1)                       \
    (((addr0).a[0] & ~(mask0).a[0]) == ((addr1).a[0] & ~(mask1).a[0])   \
     && ((addr0).a[1] & ~(mask0).a[1]) == ((addr1).a[1] & ~(mask1).a[1]) \
     && ((addr0).a[2] & ~(mask0).a[2]) == ((addr1).a[2] & ~(mask1).a[2]) \
     && ((addr0).a[3] & ~(mask0).a[3]) == ((addr1).a[3] & ~(mask1).a[3]) \
     && ((addr0).a[4] & ~(mask0).a[4]) == ((addr1).a[4] & ~(mask1).a[4]) \
     && ((addr0).a[5] & ~(mask0).a[5]) == ((addr1).a[5] & ~(mask1).a[5]) \
     && ((addr0).a[6] & ~(mask0).a[6]) == ((addr1).a[6] & ~(mask1).a[6]) \
     && ((addr0).a[7] & ~(mask0).a[7]) == ((addr1).a[7] & ~(mask1).a[7]))

#define ADDR_MASK_CMP2(addr0, mask0, addr1)                             \
    (((addr0).a[0] & ~(mask0).a[0]) == ((addr1).a[0])                   \
     && ((addr0).a[1] & ~(mask0).a[1]) == ((addr1).a[1])                \
     && ((addr0).a[2] & ~(mask0).a[2]) == ((addr1).a[2])                \
     && ((addr0).a[3] & ~(mask0).a[3]) == ((addr1).a[3])                \
     && ((addr0).a[4] & ~(mask0).a[4]) == ((addr1).a[4])                \
     && ((addr0).a[5] & ~(mask0).a[5]) == ((addr1).a[5])                \
     && ((addr0).a[6] & ~(mask0).a[6]) == ((addr1).a[6])                \
     && ((addr0).a[7] & ~(mask0).a[7]) == ((addr1).a[7]))

#define ADDR_CMP(x, y)                                                  \
    ((x).a[0] == (y).a[0] && (x).a[1] == (y).a[1] && (x).a[2] == (y).a[2] \
     && (x).a[3] == (y).a[3] && (x).a[4] == (y).a[4] && (x).a[5] == (y).a[5] \
     && (x).a[6] == (y).a[6] && (x).a[7] == (y).a[7])

#endif


#ifndef PALMTRIE_MTPT_STRIDE
#define PALMTRIE_MTPT_STRIDE  8
#endif

#ifndef PALMTRIE_PRIORITY_SKIP
#define PALMTRIE_PRIORITY_SKIP 1
#endif

#ifndef PALMTRIE_EXACTMATCH_FIRST
#define PALMTRIE_EXACTMATCH_FIRST 0
#endif


static __inline__ int
ADDR_PREFIX_CMP(addr_t a0, addr_t m0, addr_t a1, addr_t m1, int plen, int msb)
{
    int i;

    i = (plen >> 6);
    if ( ((a0.a[i] & ~(m0.a[i])) >> (plen & 0x3f))
         != ((a1.a[i] & ~(m1.a[i])) >> (plen & 0x3f)) ) {
        return 0;
    }
    i++;

    for ( ; i <= (msb >> 6); i++ ) {
        if ( (a0.a[i] & ~(m0).a[i]) != (a1.a[i] & ~(m1).a[i]) ) {
            return 0;
        }
    }

    return 1;
}

#define EXTRACT(addr, bit)      (((addr).a[(bit) >> 6] >> ((bit) & 0x3f)) & 1)

#define EXTRACTN(addr, bit, n)                                      \
    ((*(uint32_t *)((void *)&(addr).g + (((bit) + 32) >> 3))        \
      >> (((bit) + 0) & 0x7)) & ((1 << (n)) - 1))


#define BTS(addr, bit) do {                                 \
        (addr).a[(bit) >> 6] |= (1ULL << ((bit) & 0x3f));   \
    } while ( 0 )
#define BTC(addr, bit) do {                             \
        (addr).a[(bit) >> 6] &= ~(1ULL << ((bit) & 0x3f));  \
    } while ( 0 )

#define BACKTRACK_SKIP(n)   ((n).p & 0x7)
#define BACKTRACK_NODE(n)                                       \
    (struct palmtrie_mtpt_node *)((n).p & 0xfffffffffffffff8ULL)


/*
 * Type-packed union pointer
 */
#ifndef __LP64__
#error "Must be LP64."
#endif
#define TPUP_PTR(p)     (void *)((uint64_t)(p) & 0xfffffffffffffff8ULL)
#define TPUP_TYPE(p)    (int)((uint64_t)(p) & 0x7)
#define TPUP_SET(p, t)  (void *)((uint64_t)(p) | (t))


/*
 * Types of the software TCAM implementation
 */
enum palmtrie_type {
    PALMTRIE_SORTED_LIST,
    PALMTRIE_BASIC,
    PALMTRIE_DEFAULT,
    PALMTRIE_PLUS,
};

/*
 * An entry of the sorted list
 */
struct palmtrie_sorted_list_entry {
    addr_t addr;
    addr_t mask;
    int priority;
    void *data;
    struct palmtrie_sorted_list_entry *next;
};

/*
 * Sorted list
 */
struct palmtrie_sorted_list {
    struct palmtrie_sorted_list_entry *head;
};

/*
 * Node of ternary PATRICIA trie
 */
struct palmtrie_tpt_node {
    int bit;

    addr_t addr;
    addr_t mask;

#if PALMTRIE_PRIORITY_SKIP
    int max_priority;
#endif
    int priority;
    void *data;
    struct palmtrie_tpt_node *left;
    struct palmtrie_tpt_node *center;
    struct palmtrie_tpt_node *right;
};

/*
 * Ternary PATRICIA trie
 */
struct palmtrie_tpt {
    struct palmtrie_tpt_node *root;
};

/*
 * Node of multiway ternary PATRICIA trie
 * Types in the least significant 3 bits:
 *   Data node: 0
 *   Backtrack node: 1
 */
struct palmtrie_mtpt_node_backtrack {
    struct palmtrie_mtpt_node_data *backtrack;
    void *searchback;
};
struct palmtrie_mtpt_node_data {
    int bit;
    addr_t addr;
    addr_t mask;
    int priority;
#if PALMTRIE_PRIORITY_SKIP
    int max_priority;
#endif
    void *data;

    /* Descendent nodes */
    struct palmtrie_mtpt_node_data *children[1 << PALMTRIE_MTPT_STRIDE];
    /* Searchback nodes */
    struct palmtrie_mtpt_node_data *ternaries[(1 << PALMTRIE_MTPT_STRIDE) - 1];
};

/*
 * Multiway ternary PATRICIA trie
 */
struct palmtrie_mtpt {
    struct palmtrie_mtpt_node_data *root;
};

struct palmtrie_popmtpt_leaf
{
    int32_t priority;
    addr_t addr;
    addr_t mask;
    void *data;
};
#define PALMTRIE_STRIDE_OPT 1
struct palmtrie_popmtpt_node {
    int16_t bit;
    union {
        struct {
#if PALMTRIE_PRIORITY_SKIP
            int32_t max_priority;
#endif
#if PALMTRIE_MTPT_STRIDE == 8 && PALMTRIE_STRIDE_OPT
            uint32_t cbase;
            uint32_t tbase;
            uint8_t children[4];
            uint8_t ternaries[4];
            uint64_t bitmap_t[4];
            uint64_t bitmap_c[4];
#elif PALMTRIE_MTPT_STRIDE >= 6
            uint32_t children[((1 << PALMTRIE_MTPT_STRIDE) >> 6)];
            uint32_t ternaries[((1 << PALMTRIE_MTPT_STRIDE) >> 6)];
            uint64_t bitmap_t[((1 << PALMTRIE_MTPT_STRIDE) >> 6)];
            uint64_t bitmap_c[((1 << PALMTRIE_MTPT_STRIDE) >> 6)];
#else
            uint32_t children[1];
            uint32_t ternaries[1];
            uint64_t bitmap_t[1];
            uint64_t bitmap_c[1];
#endif
        } inode;
        struct {
            int32_t priority;
            addr_t addr;
            addr_t mask;
            void *data;
        } leaf;
    } u;
};
struct palmtrie_popmtpt {
    uint32_t root;
    struct {
        int nr;
        int used;
        struct palmtrie_popmtpt_node *ptr;
    } nodes;
    struct palmtrie_mtpt mtpt;
};

/*
 * Software TCAM
 */
struct palmtrie {
    union {
        struct palmtrie_sorted_list sl;
        struct palmtrie_tpt tpt;
        struct palmtrie_mtpt mtpt;
        struct palmtrie_popmtpt popmtpt;
    } u;
    enum palmtrie_type type;
    int allocated;
};

/* Prototype declarations */
struct palmtrie * palmtrie_init(struct palmtrie *, enum palmtrie_type);
int palmtrie_add_data(struct palmtrie *, addr_t, addr_t, int, u64);
u64 palmtrie_lookup(struct palmtrie *, addr_t);
int palmtrie_commit(struct palmtrie *);

/* in sl.c */
int palmtrie_sl_add(struct palmtrie *, addr_t, addr_t, int, void *);
void * palmtrie_sl_lookup(struct palmtrie *, addr_t);
int palmtrie_sl_release(struct palmtrie *);

/* in tpt.c */
int palmtrie_tpt_add(struct palmtrie *, addr_t, addr_t, int, void *);
void * palmtrie_tpt_lookup(struct palmtrie *, addr_t);
int palmtrie_tpt_release(struct palmtrie *);

/* in mtpt.c */
int palmtrie_mtpt_add(struct palmtrie_mtpt *, addr_t, addr_t, int, void *);
void * palmtrie_mtpt_lookup(struct palmtrie *, addr_t);
int palmtrie_mtpt_release(struct palmtrie *);

/* in popmtpt.c */
void * palmtrie_popmtpt_lookup(struct palmtrie_popmtpt *, addr_t);
int
palmtrie_popmtpt_add(struct palmtrie_popmtpt *, addr_t, addr_t, int, void *);
int palmtrie_popmtpt_commit(struct palmtrie_popmtpt *);

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
