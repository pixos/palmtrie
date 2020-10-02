/*_
 * Copyright (c) 2015-2018,2020 Hirochika Asai <asai@jar.jp>
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "palmtrie.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Release the instance
 */
int
palmtrie_sl_release(struct palmtrie *palmtrie)
{
    struct palmtrie_sorted_list_entry *e;
    struct palmtrie_sorted_list_entry *ne;

    e = palmtrie->u.sl.head;

    while ( NULL != e ) {
        ne = e->next;
        free(e);
        e = ne;
    }

    return 0;
}

/*
 * Add an entry to the sorted list
 */
static int
_add(struct palmtrie_sorted_list_entry **head, addr_t addr, addr_t mask,
     int priority, void *data)
{
    struct palmtrie_sorted_list_entry *ent;
    struct palmtrie_sorted_list_entry *cur;
    struct palmtrie_sorted_list_entry *prev;

    /* Allocate a sorted list entry */
    ent = malloc(sizeof(struct palmtrie_sorted_list_entry));
    if ( NULL == ent ) {
        /* Memory allocation failed. */
        return -1;
    }
    ent->addr = addr;
    ent->mask = mask;
    ent->priority = priority;
    ent->data = data;
    ent->next = NULL;

    /* Search the insertion position */
    prev = NULL;
    cur = *head;
    while ( NULL != cur && cur->priority > priority ) {
        prev = cur;
        cur = cur->next;
    }

    if ( NULL == prev ) {
        *head = ent;
        ent->next = cur;
    } else {
        prev->next = ent;
        ent->next = cur;
    }

    return 0;
}
int
palmtrie_sl_add(struct palmtrie *palmtrie, addr_t addr, addr_t mask,
                int priority, void *data)
{
    return _add(&palmtrie->u.sl.head, addr, mask, priority, data);
}

/*
 * Delete an entry from the sorted list
 */
static void *
_delete(struct palmtrie_sorted_list_entry **ent, addr_t addr, addr_t mask)
{
    void *data;

    while ( NULL != *ent ) {
        if ( ADDR_MASK_CMP((*ent)->addr, (*ent)->mask, addr, mask)
             && ADDR_CMP((*ent)->mask, mask) ) {
            /* Found */
            data = (*ent)->data;
            *ent = (*ent)->next;
            return data;
        }
        ent = &(*ent)->next;
    }

    /* Not found */
    return NULL;
}
void *
palmtrie_sl_delete(struct palmtrie *palmtrie, addr_t addr, addr_t mask)
{
    return _delete(&palmtrie->u.sl.head, addr, mask);
}

/*
 * Lookup an entry corresponding to the specified address key
 */
static void *
_lookup(struct palmtrie_sorted_list_entry *ent, addr_t addr)
{
    while ( NULL != ent ) {
        if ( ADDR_MASK_CMP(ent->addr, ent->mask, addr, ent->mask) ) {
            return ent->data;
        }
        ent = ent->next;
    }

    return NULL;
}
void *
palmtrie_sl_lookup(struct palmtrie *palmtrie, addr_t addr)
{
    return _lookup(palmtrie->u.sl.head, addr);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
