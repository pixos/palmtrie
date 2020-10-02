/*_
 * Copyright (c) 2015-2020 Hirochika Asai <asai@jar.jp>
 * All rights reserved.
 */

#include "palmtrie.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alloca.h>
#include <immintrin.h>

#define _STACK_DEPTH    64

/*
 * Recursively delete the node in a tree
 */
static void
_delete_node(struct palmtrie_mtpt_node_data *n)
{
    int i;

    if ( NULL == n ) {
        return;
    }

    for ( i = 0; i < (1 << PALMTRIE_MTPT_STRIDE); i++ ) {
        if ( NULL != n->children[i] ) {
            if ( n->bit > n->children[i]->bit ) {
                _delete_node(n->children[i]);
            }
        }
    }
    for ( i = 0; i < (1 << PALMTRIE_MTPT_STRIDE) - 1; i++ ) {
        if ( NULL != n->ternaries[i] ) {
            if ( n->bit > n->ternaries[i]->bit ) {
                _delete_node(n->ternaries[i]);
            }
        }
    }

    free(n);

    return;
}

/*
 * Release the instance
 */
int
palmtrie_mtpt_release(struct palmtrie *palmtrie)
{
    /* Check the type */
    if ( PALMTRIE_DEFAULT != palmtrie->type ) {
        /* Type error */
        return -1;
    }
    /* Recursively delete nodes from the root */
    _delete_node(palmtrie->u.mtpt.root);

    return 0;
}

/*
 * Add a leaf
 */
static int
_add_leaf(struct palmtrie_mtpt_node_data **node, addr_t addr, addr_t mask,
          int priority, void *data, int cbit)
{
    struct palmtrie_mtpt_node_data *n;
    int i;
    int idx;
    int aidx;
    int midx;
    int b;
    int bit;
    int mbit;

    /* Allocate a node */
    n = malloc(sizeof(struct palmtrie_mtpt_node_data));
    if ( NULL == n ) {
        return -1;
    }
    if ( cbit <= 0 ) {
        n->bit = cbit - PALMTRIE_MTPT_STRIDE;
    } else {
        //n->bit = 1 - PALMTRIE_MTPT_STRIDE;
        n->bit = 0;
    }
    n->addr = addr;
    n->mask = mask;
    n->priority = priority;
#if PALMTRIE_PRIORITY_SKIP
    if ( NULL != *node ) {
        n->max_priority = (*node)->max_priority > priority
            ? (*node)->max_priority : priority;
    } else {
        n->max_priority = priority;
    }
#endif
    n->data = data;
    for ( i = 0; i < (1 << PALMTRIE_MTPT_STRIDE); i++ ) {
        n->children[i] = NULL;
    }
    for ( i = 0; i < (1 << PALMTRIE_MTPT_STRIDE) - 1; i++ ) {
        n->ternaries[i] = NULL;
    }

    /* Replace */
    if ( NULL == *node ) {
        if ( cbit < -PALMTRIE_MTPT_STRIDE ) {
            /* Same key */
            free(n);
            return -1;
        }
        /* Search the least significant dc bit */
        mbit = -PALMTRIE_MTPT_STRIDE - 1;
        for ( b = cbit - 1; b >= -PALMTRIE_MTPT_STRIDE; b-- ) {
            if ( EXTRACT(mask, b) ) {
                mbit = b;
            }
        }
        if ( mbit >= -PALMTRIE_MTPT_STRIDE ) {
            /* dc bit */
            b = mbit - PALMTRIE_MTPT_STRIDE;
            while ( b >= 0 ) {
                b -= PALMTRIE_MTPT_STRIDE;
            }
        } else {
            b = cbit;
            while ( b >= 0 ) {
                b -= PALMTRIE_MTPT_STRIDE;
            }
        }
        n->bit = b;

        aidx = EXTRACTN(addr, n->bit, PALMTRIE_MTPT_STRIDE);
        midx = EXTRACTN(mask, n->bit, PALMTRIE_MTPT_STRIDE);
        if ( midx ) {
            /* There are dc bits in the key */
            for ( i = 1; i <= PALMTRIE_MTPT_STRIDE; i++ ) {
                if ( (midx >> (PALMTRIE_MTPT_STRIDE - i)) & 1 ) {
                    /* A dc bit is found. */
                    idx = ((aidx >> (PALMTRIE_MTPT_STRIDE - i + 1))
                           | (1 << (i - 1))) - 1;
                    n->ternaries[idx] = n;
                    break;
                }
            }
        } else {
            n->children[aidx] = n;
        }
        *node = n;
        return 0;
    } else {
        bit = -PALMTRIE_MTPT_STRIDE - 1;
        mbit = -PALMTRIE_MTPT_STRIDE - 1;
        for ( b = cbit + PALMTRIE_MTPT_STRIDE - 1; b >= -PALMTRIE_MTPT_STRIDE;
              b-- ) {
            if ( EXTRACT((*node)->mask, b) != EXTRACT(mask, b) ) {
                /* Different mask */
                bit = b;
                break;
            } else if ( EXTRACT((*node)->addr, b) != EXTRACT(addr, b)
                        && !EXTRACT((*node)->mask, b) ) {
                /* Different addr */
                bit = b;
                break;
            }
            if ( EXTRACT(mask, b) ) {
                mbit = b;
            }
        }
        if ( bit < -PALMTRIE_MTPT_STRIDE ) {
            /* Same node */
            free(n);
            printf("xxx %llx %llx/%llx %llx , %llx %llx/%llx %llx %d xxx",
                   (*node)->addr.a[0], (*node)->addr.a[1],
                   (*node)->mask.a[0], (*node)->mask.a[1],
                   addr.a[0], addr.a[1], mask.a[0], mask.a[1], cbit);
            return -1;
        }
        /* Calculate the appropriate stride point */
        if ( mbit < -PALMTRIE_MTPT_STRIDE ) {
            b = cbit;
        } else {
            b = mbit;
        }
        while ( b > bit ) {
            b = b - PALMTRIE_MTPT_STRIDE;
        }
        n->bit = b;
        midx = EXTRACTN(mask, b, PALMTRIE_MTPT_STRIDE);
        aidx = EXTRACTN(addr, b, PALMTRIE_MTPT_STRIDE);;
        if ( midx ) {
            /* There are dc bits in the key */
            for ( i = 1; i <= PALMTRIE_MTPT_STRIDE; i++ ) {
                if ( (midx >> (PALMTRIE_MTPT_STRIDE - i)) & 1 ) {
                    /* A dc bit is found. */
                    idx = ((aidx >> (PALMTRIE_MTPT_STRIDE - i + 1))
                           | (1 << (i - 1))) - 1;
                    n->ternaries[idx] = n;
                    break;
                }
            }
        } else {
            n->children[aidx] = n;
        }
        midx = EXTRACTN((*node)->mask, b, PALMTRIE_MTPT_STRIDE);
        aidx = EXTRACTN((*node)->addr, b, PALMTRIE_MTPT_STRIDE);
        if ( midx ) {
            /* There are dc bits in the key */
            for ( i = 1; i <= PALMTRIE_MTPT_STRIDE; i++ ) {
                if ( (midx >> (PALMTRIE_MTPT_STRIDE - i)) & 1 ) {
                    /* A dc bit is found. */
                    idx = ((aidx >> (PALMTRIE_MTPT_STRIDE - i + 1))
                           | (1 << (i - 1))) - 1;
                    n->ternaries[idx] = *node;
                    break;
                }
            }
        } else {
            n->children[aidx] = *node;
        }
        *node = n;

        return 0;
    }
}

/*
 * Add an internal node
 */
static int
_add_internal(struct palmtrie_mtpt_node_data **node, addr_t addr, addr_t mask,
              int priority, void *data, int dbit)
{
    struct palmtrie_mtpt_node_data *n;
    int i;
    int idx;
    int aidx;
    int midx;

    /* *node should not be NULL. */
    if ( NULL == *node ) {
        return -1;
    }

    /* Allocate a node */
    n = malloc(sizeof(struct palmtrie_mtpt_node_data));
    if ( NULL == n ) {
        return -1;
    }
    n->bit = dbit;
    n->addr = addr;
    n->mask = mask;
    n->priority = priority;
#if PALMTRIE_PRIORITY_SKIP
    n->max_priority = (*node)->max_priority > priority
        ? (*node)->max_priority : priority;
#endif
    n->data = data;
    for ( i = 0; i < (1 << PALMTRIE_MTPT_STRIDE); i++ ) {
        n->children[i] = NULL;
    }
    for ( i = 0; i < (1 << PALMTRIE_MTPT_STRIDE) - 1; i++ ) {
        n->ternaries[i] = NULL;
    }
    aidx = EXTRACTN(addr, dbit, PALMTRIE_MTPT_STRIDE);
    midx = EXTRACTN(mask, dbit, PALMTRIE_MTPT_STRIDE);
    if ( midx ) {
        /* There are dc bits in the key */
        for ( i = 1; i <= PALMTRIE_MTPT_STRIDE; i++ ) {
            if ( (midx >> (PALMTRIE_MTPT_STRIDE - i)) & 1 ) {
                /* A dc bit is found. */
                idx = ((aidx >> (PALMTRIE_MTPT_STRIDE - i + 1))
                       | (1 << (i - 1))) - 1;
                n->ternaries[idx] = n;
                break;
            }
        }
    } else {
        n->children[aidx] = n;
    }
    aidx = EXTRACTN((*node)->addr, dbit, PALMTRIE_MTPT_STRIDE);
    midx = EXTRACTN((*node)->mask, dbit, PALMTRIE_MTPT_STRIDE);
    if ( midx ) {
        /* There are dc bits in the key */
        for ( i = 1; i <= PALMTRIE_MTPT_STRIDE; i++ ) {
            if ( (midx >> (PALMTRIE_MTPT_STRIDE - i)) & 1 ) {
                /* A dc bit is found. */
                idx = ((aidx >> (PALMTRIE_MTPT_STRIDE - i + 1))
                       | (1 << (i - 1))) - 1;
                n->ternaries[idx] = *node;
                break;
            }
        }
    } else {
        n->children[aidx] = *node;
    }
    if ( n->bit == (*node)->bit ) {
        printf("Error in _add_internal()\n");
        return -1;
    }

    /* Replace */
    *node = n;

    return 0;
}

/*
 * Add an entry
 */
static int
_add(struct palmtrie_mtpt_node_data **node, addr_t addr, addr_t mask,
     int priority, void *data, int cbit)
{
    int bit;
    int b;
    int mbit;
    int nbit;
    struct palmtrie_mtpt_node_data **next;
    int aidx;
    int midx;
    int idx;
    int i;

    if ( NULL == *node ) {
        /* Reaches at a null node */
        return _add_leaf(node, addr, mask, priority, data,
                         PALMTRIE_ADDR_BITS - PALMTRIE_MTPT_STRIDE);
    } else {
#if PALMTRIE_PRIORITY_SKIP
        if ( priority > (*node)->max_priority ) {
            (*node)->max_priority = priority;
        }
#endif

        /* Compare the prefix */
        bit = -PALMTRIE_MTPT_STRIDE - 1;
        mbit = -PALMTRIE_MTPT_STRIDE - 1;
        for ( b = cbit; b > (*node)->bit; b-- ) {
            if ( EXTRACT((*node)->mask, b) != EXTRACT(mask, b) ) {
                /* Different mask */
                bit = b;
                break;
            } else if ( EXTRACT((*node)->addr, b) != EXTRACT(addr, b)
                        && !EXTRACT((*node)->mask, b) ) {
                /* Different addr */
                bit = b;
                break;
            }
            /* Same addr and mask bit */
            if ( EXTRACT(mask, b) ) {
                /* Mask bit is set */
                mbit = b;
            }
        }

        if ( bit >= -PALMTRIE_MTPT_STRIDE ) {
            /* Different prefix, then calculate the appropriate stride point */
            if ( mbit < -PALMTRIE_MTPT_STRIDE ) {
                b = cbit;
            } else {
                /* Calculate the bit index from the position of masked bit */
                b = mbit;
            }
            while ( b > bit ) {
                b = b - PALMTRIE_MTPT_STRIDE;
            }
            if ( (*node)->bit < b ) {
                /* Different prefix */
                return _add_internal(node, addr, mask, priority, data, b);
            }

            /* Same prefix */
        }

        /* Same prefix, then try to traverse */
        aidx = EXTRACTN(addr, (*node)->bit, PALMTRIE_MTPT_STRIDE);
        midx = EXTRACTN(mask, (*node)->bit, PALMTRIE_MTPT_STRIDE);
        if ( midx ) {
            /* There are dc bits in the key */
            next = NULL;
            for ( i = 1; i <= PALMTRIE_MTPT_STRIDE; i++ ) {
                if ( (midx >> (PALMTRIE_MTPT_STRIDE - i)) & 1 ) {
                    /* A dc bit is found. */
                    idx = ((aidx >> (PALMTRIE_MTPT_STRIDE - i + 1))
                           | (1 << (i - 1))) - 1;
                    next = &(*node)->ternaries[idx];
                    nbit = (*node)->bit + (PALMTRIE_MTPT_STRIDE - i);
                    //nbit = (*node)->bit;
                    break;
                }
            }
            if ( NULL == next ) {
                return -1;
            }
        } else {
            next = &(*node)->children[aidx];
            nbit = (*node)->bit;
        }
        if ( NULL == *next ) {
            /* Leaf */
            return _add_leaf(next, addr, mask, priority, data, nbit);
        } else if ( (*node)->bit <= (*next)->bit ) {
            /* Backtrack */
            return _add_leaf(next, addr, mask, priority, data, nbit);
        } else {
            /* Traverse to a descendent node */
            return _add(next, addr, mask, priority, data, nbit);
        }
    }

    return -1;
}
int
palmtrie_mtpt_add(struct palmtrie_mtpt *mtpt, addr_t addr, addr_t mask,
               int priority, void *data)
{
    return _add(&mtpt->root, addr, mask, priority, data,
                PALMTRIE_ADDR_BITS - PALMTRIE_MTPT_STRIDE);
}

struct cache {
    struct palmtrie_mtpt_node_data *node;
    int bit;
    int idx;
};
static __m128i bitindices[8] = {
    {0x0000000000000001, 0x0000000300000007},
    {0x0000000000000001, 0x0000000300000008},
    {0x0000000000000001, 0x0000000400000009},
    {0x0000000000000001, 0x000000040000000a},
    {0x0000000000000002, 0x000000050000000b},
    {0x0000000000000002, 0x000000050000000c},
    {0x0000000000000002, 0x000000060000000d},
    {0x0000000000000002, 0x000000060000000e},
};

static struct palmtrie_mtpt_node_data *
_lookup_pfs(struct palmtrie_mtpt_node_data *node, addr_t addr, int bit,
            struct palmtrie_mtpt_node_data *res)
{
    int idx;
    int i;
    int32_t bits[_STACK_DEPTH];
    int nr;
    struct palmtrie_mtpt_node_data *ptrs[_STACK_DEPTH];

    if ( NULL == node ) {
        return res;
    }

    nr = 0;
    ptrs[nr] = node;
    bits[nr] = bit;
    nr++;
    while ( nr > 0 ) {
        nr--;
        node = ptrs[nr];

#if PALMTRIE_PRIORITY_SKIP
        if ( res->priority >= node->max_priority ) {
            continue;
        }
#endif
        bit = bits[nr];
        /* Check the current node */
        if ( bit <= node->bit ) {
            /* Backtracked */
            if ( ADDR_MASK_CMP2(addr, node->mask, node->addr)
                 && node->priority > res->priority ) {
                res = node;
            }
            continue;
        }

        /* To search */
        idx = EXTRACTN(addr, node->bit, PALMTRIE_MTPT_STRIDE);
        if ( NULL != node->children[idx] ) {
            ptrs[nr] = node->children[idx];
            bits[nr] = node->bit;
            nr++;
        }

        idx = (idx >> 1) | (1 << (PALMTRIE_MTPT_STRIDE - 1));
        for ( i = 0; i < PALMTRIE_MTPT_STRIDE; i++ ) {
            if ( NULL != node->ternaries[(idx >> i) - 1] ) {
                ptrs[nr] = node->ternaries[(idx >> i) - 1];
                bits[nr] = node->bit;
                nr++;
            }
        }

        if ( nr >= _STACK_DEPTH ) {
            fprintf(stderr, "Fatal error: Stack overflow\n");
        }
    }

    return res;
}

/*
 * Lookup an entry corresponding to the specified address
 */
void *
palmtrie_mtpt_lookup(struct palmtrie *palmtrie, addr_t addr)
{
    struct palmtrie_mtpt_node_data *r;
    struct palmtrie_mtpt_node_data sentinel;

    sentinel.priority = -1;
    sentinel.data = NULL;
    r = _lookup_pfs(palmtrie->u.mtpt.root, addr,
                    PALMTRIE_ADDR_BITS - PALMTRIE_MTPT_STRIDE, &sentinel);

    return r->data;
}

/*
 * Delete an entry corresponding to the specified addr/mask
 */
void *
palmtrie_mtpt_delete(struct palmtrie *palmtrie, addr_t addr, addr_t mask)
{
    /* To be implemented */
    return NULL;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
