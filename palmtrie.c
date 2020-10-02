/*_
 * Copyright (c) 2015-2020 Hirochika Asai <asai@jar.jp>
 * All rights reserved.
 *
 */

#include "palmtrie.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Initialize an instance
 */
struct palmtrie *
palmtrie_init(struct palmtrie *palmtrie, enum palmtrie_type type)
{
    /* Allocate for the data structure when the argument is not NULL, and then
       clear all the variables */
    if ( NULL == palmtrie ) {
        palmtrie = malloc(sizeof(struct palmtrie));
        if ( NULL == palmtrie ) {
            /* Memory allocation error */
            return NULL;
        }
        (void)memset(palmtrie, 0, sizeof(struct palmtrie));
        palmtrie->allocated = 1;
    } else {
        (void)memset(palmtrie, 0, sizeof(struct palmtrie));
    }

    /* Set the type and initialize the type-specific data structure */
    switch ( type ) {
    case PALMTRIE_SORTED_LIST:
        /* Sorted list */
        palmtrie->u.sl.head = NULL;
        break;
    case PALMTRIE_TERNARY_PATRICIA:
        /* Ternary PATRICIA */
        palmtrie->u.tpt.root = NULL;
        break;
    case PALMTRIE_MULTIWAY_TERNARY_PATRICIA:
        /* Multiway ternary PATRICIA */
        palmtrie->u.mtpt.root = NULL;
        break;
    case PALMTRIE_MULTIWAY_TERNARY_PATRICIA_OPT:
        /* Multiway ternary PATRICIA */
        palmtrie->u.popmtpt.root = 0;
        palmtrie->u.popmtpt.nodes.nr = 0;
        palmtrie->u.popmtpt.nodes.used = 0;
        palmtrie->u.popmtpt.nodes.ptr = NULL;
        palmtrie->u.popmtpt.mtpt.root = NULL;
        break;
    default:
        /* Unsupported type */
        if ( palmtrie->allocated ) {
            free(palmtrie);
        }
        return NULL;
    }
    palmtrie->type = type;

    return palmtrie;
}

/*
 * Release the instance
 */
void
palmtrie_release(struct palmtrie *palmtrie)
{
    int ret;

    /* Release type-specific variables */
    switch ( palmtrie->type ) {
    case PALMTRIE_SORTED_LIST:
        ret = palmtrie_sl_release(palmtrie);
        break;
    case PALMTRIE_TERNARY_PATRICIA:
        ret = palmtrie_tpt_release(palmtrie);
        break;
    case PALMTRIE_MULTIWAY_TERNARY_PATRICIA:
        ret = palmtrie_mtpt_release(palmtrie);
        break;
    case PALMTRIE_MULTIWAY_TERNARY_PATRICIA_OPT:
        //ret = palmtrie_mtpt_release(palmtrie);
        ret = 0;
        break;
    default:
        return;
    }
    if ( 0 != ret ) {
        return;
    }

    if ( palmtrie->allocated ) {
        free(palmtrie);
    }
}

/*
 * palmtrie_add_data -- add an entry with data for a specified address to the
 * trie
 */
int
palmtrie_add_data(struct palmtrie *palmtrie, addr_t addr, addr_t mask,
                  int priority, u64 data)
{
    switch ( palmtrie->type ) {
    case PALMTRIE_SORTED_LIST:
        return palmtrie_sl_add(palmtrie, addr, mask, priority, (void *)data);
    case PALMTRIE_TERNARY_PATRICIA:
        return palmtrie_tpt_add(palmtrie, addr, mask, priority, (void *)data);
    case PALMTRIE_MULTIWAY_TERNARY_PATRICIA:
        return palmtrie_mtpt_add(&palmtrie->u.mtpt, addr, mask, priority,
                                 (void *)data);
    case PALMTRIE_MULTIWAY_TERNARY_PATRICIA_OPT:
        return palmtrie_popmtpt_add(&palmtrie->u.popmtpt, addr, mask, priority,
                                    (void *)data);
    default:
        /* Not supported type */
        return -1;
    }
}

/*
 * palmtrie_lookup -- lookup an entry corresponding to the specified address
 * from the trie
 */
u64
palmtrie_lookup(struct palmtrie *palmtrie, addr_t addr)
{
    switch ( palmtrie->type ) {
    case PALMTRIE_SORTED_LIST:
        return (u64)palmtrie_sl_lookup(palmtrie, addr);
    case PALMTRIE_TERNARY_PATRICIA:
        return (u64)palmtrie_tpt_lookup(palmtrie, addr);
    case PALMTRIE_MULTIWAY_TERNARY_PATRICIA:
        return (u64)palmtrie_mtpt_lookup(palmtrie, addr);
    case PALMTRIE_MULTIWAY_TERNARY_PATRICIA_OPT:
        return (u64)palmtrie_popmtpt_lookup(&palmtrie->u.popmtpt, addr);
    default:
        return 0;
    }

    return 0;
}

/*
 * palmtrie_commit -- compile an optimized trie by applying incremental updates
 */
int
palmtrie_commit(struct palmtrie *palmtrie)
{
    if ( PALMTRIE_MULTIWAY_TERNARY_PATRICIA_OPT == palmtrie->type ) {
        return palmtrie_popmtpt_commit(&palmtrie->u.popmtpt);
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
