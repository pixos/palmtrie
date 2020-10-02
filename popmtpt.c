/*_
 * Copyright (c) 2019-2020 Hirochika Asai <asai@jar.jp>
 * All rights reserved.
 */

#include "palmtrie.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <immintrin.h>

/* 64-bit popcnt intrinsic.  To use popcnt instruction in x86-64, the "-mpopcnt"
   option must be specified in CFLAGS. */
#define popcnt(v)               __builtin_popcountll(v)

#define PALMTRIE_POPMTPT_NR_NODES      (1 << 23)

#define _STACK_DEPTH    64

/*
 * Check if the  node is compressible or not
 * Return value:
 *   -1: Not compressible
 *    0: Compressible NULL
 *    1: Compressible non-NULL
 */
static struct palmtrie_mtpt_node_data *
_compressible_leaf(struct palmtrie_mtpt_node_data *n)
{
    int i;
    uint64_t cn;
    uint64_t cl;
    uint64_t tn;
    uint64_t tl;
    struct palmtrie_mtpt_node_data *pn;

    pn = NULL;
    cn = 0;
    cl = 0;
    for ( i = 0; i < (1 << PALMTRIE_MTPT_STRIDE); i++ ) {
        if ( NULL != n->children[i] ) {
            if ( n->bit > n->children[i]->bit ) {
                cn++;
            } else {
                pn = n->children[i];
                cl++;
            }
        }
    }

    tn = 0;
    tl = 0;
    for ( i = 0; i < (1 << PALMTRIE_MTPT_STRIDE) - 1; i++ ) {
        if ( NULL != n->ternaries[i] ) {
            if ( n->bit > n->ternaries[i]->bit ) {
                tn++;
            } else {
                pn = n->ternaries[i];
                tl++;
            }
        }
    }

    if ( cn || tn ) {
        return NULL;
    }
    if ( cl == 1 && tl == 0 ) {
        return pn;
    }
    if ( cl == 0 && tl == 1 ) {
        return pn;
    }

    return NULL;
}

/*
 * Traverse the trie to compile the optimized trie
 */
static int
_traverse_node(struct palmtrie_popmtpt *t, struct palmtrie_popmtpt_node *pn,
               struct palmtrie_mtpt_node_data *n)
{
    int i;
    int pos;
    uint64_t bitmap[(1 << PALMTRIE_MTPT_STRIDE) >> 6];
    struct palmtrie_popmtpt_node *c;
    struct palmtrie_mtpt_node_data *cl;

    if ( NULL == n ) {
        return -1;
    }

    pn->bit = n->bit;
#if PALMTRIE_PRIORITY_SKIP
    pn->u.inode.max_priority = n->max_priority;
#endif

    /* Binary */
    bitmap[0] = 0;
    for ( i = 0; i < ((1 << PALMTRIE_MTPT_STRIDE) >> 6); i++ ) {
        bitmap[i] = 0;
    }
    for ( i = 0; i < (1 << PALMTRIE_MTPT_STRIDE); i++ ) {
        if ( NULL != n->children[i] ) {
            if ( n->bit > n->children[i]->bit
                 && !_compressible_leaf(n->children[i]) ) {
                /* Node */
                bitmap[i >> 6] |= (1ULL << (i & 0x3f));
            } else {
                /* Leaf */
                bitmap[i >> 6] |= (1ULL << (i & 0x3f));
            }
        }
    }
    c = &t->nodes.ptr[t->nodes.used];
#if PALMTRIE_MTPT_STRIDE == 8 && PALMTRIE_STRIDE_OPT
    pn->u.inode.cbase = t->nodes.used;
    int cbase = 0;
    for ( i = 0; i < ((1 << PALMTRIE_MTPT_STRIDE) >> 6); i++ ) {
        pn->u.inode.bitmap_c[i] = bitmap[i];
        pn->u.inode.children[i] = cbase;
        cbase += popcnt(bitmap[i]);
        t->nodes.used += popcnt(bitmap[i]);
        if ( t->nodes.used > t->nodes.nr ) {
            fprintf(stderr, "Too many nodes\n");
            return -1;
        }
    }
#elif PALMTRIE_MTPT_STRIDE >= 6
    for ( i = 0; i < ((1 << PALMTRIE_MTPT_STRIDE) >> 6); i++ ) {
        pn->u.inode.children[i] = t->nodes.used;
        pn->u.inode.bitmap_c[i] = bitmap[i];
        t->nodes.used += popcnt(bitmap[i]);
        if ( t->nodes.used > t->nodes.nr ) {
            fprintf(stderr, "Too many nodes\n");
            return -1;
        }
    }
#else
    pn->u.inode.children[0] = t->nodes.used;
    pn->u.inode.bitmap_c[0] = bitmap[0];
    t->nodes.used += popcnt(bitmap[0]);
    if ( t->nodes.used > t->nodes.nr ) {
        fprintf(stderr, "Too many nodes\n");
        return -1;
    }
#endif
    pos = 0;
    for ( i = 0; i < (1 << PALMTRIE_MTPT_STRIDE); i++ ) {
        if ( NULL != n->children[i] ) {
            if ( n->bit > n->children[i]->bit ) {
                cl = _compressible_leaf(n->children[i]);
                if ( cl ) {
                    /* Leaf */
                    c[pos].bit = -PALMTRIE_MTPT_STRIDE - 1;
                    c[pos].u.leaf.addr = cl->addr;
                    c[pos].u.leaf.mask = cl->mask;
                    c[pos].u.leaf.data = cl->data;
                    c[pos].u.leaf.priority = cl->priority;
                    pos++;
                } else {
                    /* Traverse */
                    _traverse_node(t, &c[pos], n->children[i]);
                    pos++;
                }
            } else {
                /* Leaf */
                c[pos].bit = -PALMTRIE_MTPT_STRIDE - 1;
                c[pos].u.leaf.addr = n->children[i]->addr;
                c[pos].u.leaf.mask = n->children[i]->mask;
                c[pos].u.leaf.data = n->children[i]->data;
                c[pos].u.leaf.priority = n->children[i]->priority;
                pos++;
            }
        }
    }

    /* Ternary */
    bitmap[0] = 0;
    for ( i = 0; i < ((1 << PALMTRIE_MTPT_STRIDE) >> 6); i++ ) {
        bitmap[i] = 0;
    }
    for ( i = 0; i < (1 << PALMTRIE_MTPT_STRIDE) - 1; i++ ) {
        if ( NULL != n->ternaries[i] ) {
            if ( n->bit > n->ternaries[i]->bit
                 && !_compressible_leaf(n->ternaries[i]) ) {
                bitmap[i >> 6] |= (1ULL << (i & 0x3f));
            } else {
                /* Leaf */
                bitmap[i >> 6] |= (1ULL << (i & 0x3f));
            }
        }
    }
    c = &t->nodes.ptr[t->nodes.used];
#if PALMTRIE_MTPT_STRIDE == 8 && PALMTRIE_STRIDE_OPT
    int tbase = 0;
    pn->u.inode.tbase = t->nodes.used;
    for ( i = 0; i < ((1 << PALMTRIE_MTPT_STRIDE) >> 6); i++ ) {
        pn->u.inode.bitmap_t[i] = bitmap[i];
        pn->u.inode.ternaries[i] = tbase;
        tbase += popcnt(bitmap[i]);
        t->nodes.used += popcnt(bitmap[i]);
        if ( t->nodes.used > t->nodes.nr ) {
            fprintf(stderr, "Too many nodes\n");
            return -1;
        }
    }
#elif PALMTRIE_MTPT_STRIDE >= 6
    for ( i = 0; i < ((1 << PALMTRIE_MTPT_STRIDE) >> 6); i++ ) {
        pn->u.inode.ternaries[i] = t->nodes.used;
        pn->u.inode.bitmap_t[i] = bitmap[i];
        t->nodes.used += popcnt(bitmap[i]);
        if ( t->nodes.used > t->nodes.nr ) {
            fprintf(stderr, "Too many nodes\n");
            return -1;
        }
    }
#else
    pn->u.inode.ternaries[0] = t->nodes.used;
    pn->u.inode.bitmap_t[0] = bitmap[0];
    t->nodes.used += popcnt(bitmap[0]);
    if ( t->nodes.used > t->nodes.nr ) {
        fprintf(stderr, "Too many nodes\n");
        return -1;
    }
#endif
    pos = 0;
    for ( i = 0; i < (1 << PALMTRIE_MTPT_STRIDE) - 1; i++ ) {
        if ( NULL != n->ternaries[i] ) {
            if ( n->bit > n->ternaries[i]->bit ) {
                cl = _compressible_leaf(n->ternaries[i]);
                if ( cl ) {
                    c[pos].bit = -PALMTRIE_MTPT_STRIDE - 1;
                    c[pos].u.leaf.addr = cl->addr;
                    c[pos].u.leaf.mask = cl->mask;
                    c[pos].u.leaf.data = cl->data;
                    c[pos].u.leaf.priority = cl->priority;
                    pos++;
                } else {
                    /* Traverse */
                    _traverse_node(t, &c[pos], n->ternaries[i]);
                    pos++;
                }
            } else {
                /* Terminate */
                c[pos].bit = -PALMTRIE_MTPT_STRIDE - 1;
                c[pos].u.leaf.addr = n->ternaries[i]->addr;
                c[pos].u.leaf.mask = n->ternaries[i]->mask;
                c[pos].u.leaf.data = n->ternaries[i]->data;
                c[pos].u.leaf.priority = n->ternaries[i]->priority;
                pos++;
            }
        }
    }

    return 0;
}

/*
 * Convert the multiway trie to the optimized trie
 */
static int
_convert(struct palmtrie_popmtpt *popmtpt)
{
    struct palmtrie_popmtpt_node *c;
    int ret;

    popmtpt->nodes.nr = PALMTRIE_POPMTPT_NR_NODES;
    popmtpt->nodes.ptr = malloc(sizeof(struct palmtrie_popmtpt_node)
                                * popmtpt->nodes.nr);
    if ( NULL == popmtpt->nodes.ptr ) {
        fprintf(stderr, "Memory allocation error\n");
        return -1;
    }
    popmtpt->nodes.used = 0;

    c = &popmtpt->nodes.ptr[popmtpt->nodes.used];
    popmtpt->root = popmtpt->nodes.used;
    popmtpt->nodes.used++;
    ret = _traverse_node(popmtpt, c, popmtpt->mtpt.root);
    if ( ret < 0 ) {
        return -1;
    }

    return 0;
}

/*
 * Lookup an entry corresponding to the specified address
 */
static struct palmtrie_popmtpt_node *
_lookup(struct palmtrie_popmtpt *t, struct palmtrie_popmtpt_node *node,
        addr_t addr, struct palmtrie_popmtpt_node *res)
{
    int sidx;
    int idx;
    int i;
    struct palmtrie_popmtpt_node *c;
    int nr;
    int tmp;
    int pidx;
    struct palmtrie_popmtpt_node **ptrs;

    ptrs = alloca(sizeof(struct palmtrie_popmtpt_node *) * _STACK_DEPTH);

    if ( __builtin_expect(!!(NULL == node), 0) ) {
        return res;
    }

    nr = 0;
    ptrs[nr++] = node;
    while ( nr > 0 ) {
        nr--;
        node = ptrs[nr];

        if ( node->bit < -PALMTRIE_MTPT_STRIDE ) {
            /* Leaf */
            if ( node->u.leaf.priority > res->u.leaf.priority &&
                 ADDR_MASK_CMP2(addr, node->u.leaf.mask, node->u.leaf.addr) ) {
                res = node;
            }
            continue;
        }

#if PALMTRIE_PRIORITY_SKIP
        if ( res->u.leaf.priority >= node->u.inode.max_priority ) {
            continue;
        }
#endif

#if PALMTRIE_MTPT_STRIDE == 8 && PALMTRIE_STRIDE_OPT
        uint32_t base;
        sidx = EXTRACTN(addr, node->bit, PALMTRIE_MTPT_STRIDE);
        idx = (sidx >> 1) | (1 << (PALMTRIE_MTPT_STRIDE - 1));

        if ( node->u.inode.bitmap_t[0] ) {
              for ( i = 7; i >= 2; i-- ) {
                tmp = (idx >> i) - 1;
                if ( (1ULL << (tmp & 0x3f))
                     & node->u.inode.bitmap_t[tmp >> 6] ) {
                    pidx = popcnt(((1ULL << (tmp & 0x3f)) - 1)
                                  & node->u.inode.bitmap_t[tmp >> 6]);
                    base = node->u.inode.tbase
                        + node->u.inode.ternaries[tmp >> 6];
                    c = &t->nodes.ptr[base];
                    __builtin_prefetch(&c[pidx], 0, 3);
                    ptrs[nr] = &c[pidx];
                    nr++;
                }
            }
        }
        tmp = (idx >> 1) - 1;
        if ( node->u.inode.bitmap_t[tmp >> 6] &&
             (1ULL << (tmp & 0x3f)) & node->u.inode.bitmap_t[tmp >> 6] ) {
            pidx = popcnt(((1ULL << (tmp & 0x3f)) - 1)
                          & node->u.inode.bitmap_t[tmp >> 6]);
            base = node->u.inode.tbase + node->u.inode.ternaries[tmp >> 6];
            c = &t->nodes.ptr[base];
            __builtin_prefetch(&c[pidx], 0, 3);
            ptrs[nr] = &c[pidx];
            nr++;
        }
        tmp = (idx >> 0) - 1;
        if ( node->u.inode.bitmap_t[tmp >> 6] &&
             (1ULL << (tmp & 0x3f)) & node->u.inode.bitmap_t[tmp >> 6] ) {
            pidx = popcnt(((1ULL << (tmp & 0x3f)) - 1)
                          & node->u.inode.bitmap_t[tmp >> 6]);
            base = node->u.inode.tbase + node->u.inode.ternaries[tmp >> 6];
            c = &t->nodes.ptr[base];
            __builtin_prefetch(&c[pidx], 0, 3);
            ptrs[nr] = &c[pidx];
            nr++;
        }

        idx = sidx;
        if ( (1ULL << (idx & 0x3f)) & node->u.inode.bitmap_c[idx >> 6] ) {
            /* There's a child node */
            pidx = popcnt(((1ULL << (idx & 0x3f)) - 1)
                          & node->u.inode.bitmap_c[idx >> 6]);
            base = node->u.inode.cbase + node->u.inode.children[idx >> 6];
            c = &t->nodes.ptr[base];
            __builtin_prefetch(&c[pidx], 0, 3);
            ptrs[nr] = &c[pidx];
            nr++;
        }
#else

        /* Sort by priority (roughly) */
        sidx = EXTRACTN(addr, node->bit, PALMTRIE_MTPT_STRIDE);
#if PALMTRIE_EXACTMATCH_FIRST
        idx = sidx;
        if ( (1ULL << (idx & 0x3f)) & node->u.inode.bitmap_c[idx >> 6] ) {
            /* There's a child node */
            pidx = popcnt(((1ULL << (idx & 0x3f)) - 1)
                          & node->u.inode.bitmap_c[idx >> 6]);
            c = &t->nodes.ptr[node->u.inode.children[idx >> 6]];
            __builtin_prefetch(&c[pidx], 0, 3);
            ptrs[nr] = &c[pidx];
            nr++;
        }
#endif
        idx = (sidx >> 1) | (1 << (PALMTRIE_MTPT_STRIDE - 1));
#if PALMTRIE_MTPT_STRIDE <= 6
#define TERNARY_CONDITION_BEGIN if ( node->u.inode.bitmap_t[0] ) {
#define TERNARY_CONDITION_END }
#elif PALMTRIE_MTPT_STRIDE == 7
#define TERNARY_CONDITION_BEGIN                                         \
        if ( node->u.inode.bitmap_t[0] || node->u.inode.bitmap_t[1] ) {
#define TERNARY_CONDITION_END }
#elif PALMTRIE_MTPT_STRIDE == 8
#define TERNARY_CONDITION_BEGIN                                         \
        if ( node->u.inode.bitmap_t[0] || node->u.inode.bitmap_t[1]     \
             || node->u.inode.bitmap_t[2] || node->u.inode.bitmap_t[3] ) {
#define TERNARY_CONDITION_END }
#else
#define TERNARY_CONDITION_BEGIN
#define TERNARY_CONDITION_END
#endif

#if PALMTRIE_MTPT_STRIDE == 8
        if ( node->u.inode.bitmap_t[0]) {
              for ( i = 7; i >= 2; i-- ) {
                tmp = (idx >> i) - 1;
                if ( (1ULL << (tmp & 0x3f))
                     & node->u.inode.bitmap_t[tmp >> 6] ) {
                    pidx = popcnt(((1ULL << (tmp & 0x3f)) - 1)
                                  & node->u.inode.bitmap_t[tmp >> 6]);
                    c = &t->nodes.ptr[node->u.inode.ternaries[tmp >> 6]];
                    __builtin_prefetch(&c[pidx], 0, 3);
                    ptrs[nr] = &c[pidx];
                    nr++;
                }
            }
        }
        tmp = (idx >> 1) - 1;
        if ( node->u.inode.bitmap_t[tmp >> 6] &&
             (1ULL << (tmp & 0x3f)) & node->u.inode.bitmap_t[tmp >> 6] ) {
            pidx = popcnt(((1ULL << (tmp & 0x3f)) - 1)
                          & node->u.inode.bitmap_t[tmp >> 6]);
            c = &t->nodes.ptr[node->u.inode.ternaries[tmp >> 6]];
            __builtin_prefetch(&c[pidx], 0, 3);
            ptrs[nr] = &c[pidx];
            nr++;
        }
        tmp = (idx >> 0) - 1;
        if ( node->u.inode.bitmap_t[tmp >> 6] &&
             (1ULL << (tmp & 0x3f)) & node->u.inode.bitmap_t[tmp >> 6] ) {
            pidx = popcnt(((1ULL << (tmp & 0x3f)) - 1)
                          & node->u.inode.bitmap_t[tmp >> 6]);
            c = &t->nodes.ptr[node->u.inode.ternaries[tmp >> 6]];
            __builtin_prefetch(&c[pidx], 0, 3);
            ptrs[nr] = &c[pidx];
            nr++;
        }
#else
        TERNARY_CONDITION_BEGIN
        for ( i = PALMTRIE_MTPT_STRIDE - 1; i >= 0; i-- ) {
            tmp = (idx >> i) - 1;
            if ( (1ULL << (tmp & 0x3f)) & node->u.inode.bitmap_t[tmp >> 6] ) {
                pidx = popcnt(((1ULL << (tmp & 0x3f)) - 1)
                              & node->u.inode.bitmap_t[tmp >> 6]);
                c = &t->nodes.ptr[node->u.inode.ternaries[tmp >> 6]];
                __builtin_prefetch(&c[pidx], 0, 3);
                ptrs[nr] = &c[pidx];
                nr++;
            }
        }
        TERNARY_CONDITION_END
#endif
#if !PALMTRIE_EXACTMATCH_FIRST
        idx = sidx;
        if ( (1ULL << (idx & 0x3f)) & node->u.inode.bitmap_c[idx >> 6] ) {
            /* There's a child node */
            pidx = popcnt(((1ULL << (idx & 0x3f)) - 1)
                          & node->u.inode.bitmap_c[idx >> 6]);
            c = &t->nodes.ptr[node->u.inode.children[idx >> 6]];
            __builtin_prefetch(&c[pidx], 0, 3);
            ptrs[nr] = &c[pidx];
            nr++;
        }
#endif

#endif
        if ( __builtin_expect(!!(nr >= _STACK_DEPTH), 0) ) {
            fprintf(stderr, "Fatal error: Stack overflow\n");
        }
    }

    return res;
}
void *
palmtrie_popmtpt_lookup(struct palmtrie_popmtpt *t, addr_t addr)
{
    struct palmtrie_popmtpt_node *n;
    struct palmtrie_popmtpt_node sentinel;

    sentinel.u.leaf.data = NULL;
    sentinel.u.leaf.priority = -1;
    n = &t->nodes.ptr[t->root];
    return _lookup(t, n, addr, &sentinel)->u.leaf.data;
}

/*
 * Add an entry to the trie
 */
int
palmtrie_popmtpt_add(struct palmtrie_popmtpt *mtpt, addr_t addr, addr_t mask,
                     int priority, void *data)
{
    int ret;

    ret = palmtrie_mtpt_add(&mtpt->mtpt, addr, mask, priority, data);
    if ( ret < 0 ){
        return -1;
    }

    return 0;
}

/*
 * Compile the optimized trie
 */
int
palmtrie_popmtpt_commit(struct palmtrie_popmtpt *mtpt)
{
    int ret;

    if ( NULL != mtpt->nodes.ptr ) {
        free(mtpt->nodes.ptr);
        mtpt->nodes.used = 0;
        mtpt->nodes.nr = 0;
        mtpt->root = 0;
    }
    ret = _convert(mtpt);
    if ( ret < 0 ) {
        return -1;
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
