/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2010 Carnegie Mellon University.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * This work was supported in part by funding from the Defense Advanced 
 * Research Projects Agency and the National Science Foundation of the 
 * United States of America, and the CMU Sphinx Speech Consortium.
 *
 * THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 *
 */

/**
 * \file garray.h
 * \brief Generic expandable arrays.
 */

#include "garray.h"
#include "err.h"
#include "ckd_alloc.h"

#include <string.h>
#include <assert.h>

struct garray_s {
    int refcount;
    void *ent;
    size_t ent_size;
    size_t n_ent, n_ent_alloc;
    size_t base_idx;
    garray_cmp_t cmp;
    void *udata;
};

garray_t *
garray_init(size_t n_ent, size_t ent_size)
{
    garray_t *gar = ckd_calloc(1, sizeof(*gar));
    gar->refcount = 1;
    if (n_ent == 0)
        gar->n_ent_alloc = 8; /* Arbitrary number. */
    else
        gar->n_ent_alloc = n_ent;
    gar->ent_size = ent_size;
    gar->n_ent = n_ent;
    gar->ent = ckd_malloc(gar->n_ent_alloc * gar->ent_size);
    return gar;
}

garray_t *
garray_retain(garray_t *gar)
{
    ++gar->refcount;
    return gar;
}

int
garray_free(garray_t *gar)
{
    if (gar == NULL)
        return 0;
    if (--gar->refcount > 0)
        return gar->refcount;

    ckd_free(gar->ent);
    ckd_free(gar);
    return 0;
}

size_t
garray_size(garray_t *gar)
{
    return gar->n_ent;
}

size_t
garray_ent_size(garray_t *gar)
{
    return gar->ent_size;
}

size_t
garray_next_idx(garray_t *gar)
{
    return gar->n_ent + gar->base_idx;
}

size_t
garray_alloc_size(garray_t *gar)
{
    return gar->n_ent_alloc;
}

size_t
garray_reserve(garray_t *gar, size_t n_ent)
{
    if (n_ent > gar->n_ent_alloc) {
        assert(gar->n_ent_alloc != 0);
        while (n_ent > gar->n_ent_alloc)
            gar->n_ent_alloc *= 2;
        gar->ent = ckd_realloc(gar->ent, gar->n_ent_alloc * gar->ent_size);
    }
    return gar->n_ent_alloc;
}

size_t
garray_expand(garray_t *gar, size_t n_ent)
{
    garray_reserve(gar, n_ent);
    gar->n_ent = n_ent;
    return gar->n_ent;
}

size_t
garray_expand_to(garray_t *gar, size_t next_idx)
{
    assert(next_idx != 0);
    return garray_expand(gar, next_idx - gar->base_idx);
}

void *
garray_void(garray_t *gar, size_t idx)
{
    if (idx < gar->base_idx)
        return NULL;
    return (char *)gar->ent + (idx - gar->base_idx) * gar->ent_size;
}

void *
garray_append(garray_t *gar, void const *ent)
{
    garray_expand(gar, gar->n_ent + 1);
    memcpy(garray_void(gar, gar->n_ent + gar->base_idx - 1),
           ent, gar->ent_size);
    return garray_void(gar, gar->n_ent + gar->base_idx - 1);
}

size_t
garray_pop(garray_t *gar, size_t n_ent)
{
    if (n_ent > gar->n_ent)
        gar->n_ent = 0;
    else
        gar->n_ent -= n_ent;
    return gar->n_ent;
}

size_t
garray_pop_from(garray_t *gar, size_t first_idx)
{
    if (first_idx >= gar->n_ent + gar->base_idx)
        return gar->n_ent + gar->base_idx;
    return garray_pop(gar, gar->n_ent + gar->base_idx - first_idx)
        + gar->base_idx;
}

void
garray_reset(garray_t *gar)
{
    gar->n_ent = 0;
    gar->base_idx = 0;
}

void
garray_reset_to(garray_t *gar, size_t base_idx)
{
    gar->n_ent = 0;
    gar->base_idx = base_idx;
}

size_t
garray_shift(garray_t *gar, size_t n_ent)
{
    if (n_ent > gar->n_ent)
        gar->n_ent = 0;
    else
        gar->n_ent -= n_ent;
    if (gar->n_ent == 0)
        return 0;
    memmove(gar->ent, garray_void(gar, gar->base_idx + n_ent),
            gar->n_ent * gar->ent_size);
    return gar->n_ent;
}

size_t
garray_shift_from(garray_t *gar, size_t first_idx)
{
    return garray_shift(gar, first_idx - gar->base_idx);
}

size_t
garray_move(garray_t *gar, size_t dest, size_t src, size_t n_ent)
{
    if ((src - gar->base_idx) + n_ent > gar->n_ent)
        return 0;
    if ((dest - gar->base_idx) + n_ent > gar->n_ent)
        return 0;
    memmove(garray_void(gar, dest),
            garray_void(gar, src), n_ent * gar->ent_size);
    return n_ent;
}

void
garray_clear(garray_t *gar, size_t start, size_t n_ent)
{
    memset(garray_void(gar, start), 0, n_ent * gar->ent_size);
}

garray_t *
garray_slice(garray_t *gar, size_t start, size_t n_ent)
{
    garray_t *gar2;

    if ((start - gar->base_idx) + n_ent > gar->n_ent)
        return NULL;
    gar2 = garray_init(n_ent, gar->ent_size);
    memcpy(gar2->ent, garray_void(gar, start), n_ent * gar->ent_size);
    return gar2;
}

size_t
garray_set_base(garray_t *gar, size_t base_idx)
{
    size_t old_base = gar->base_idx;
    gar->base_idx = base_idx;
    return old_base;
}

size_t
garray_base(garray_t *gar)
{
    return gar->base_idx;
}

void *
garray_put(garray_t *gar, size_t idx, void const *ent)
{
    if (idx < gar->base_idx)
        return NULL;
    if ((idx - gar->base_idx) >= gar->n_ent)
        return NULL;
    memcpy(garray_void(gar, idx), ent, gar->ent_size);
    return garray_void(gar, idx);
}

void *
garray_insert(garray_t *gar, size_t idx, void const *ent)
{
    size_t n_move, rv;

    if (idx < gar->base_idx)
        return NULL;
    if ((idx - gar->base_idx) >= gar->n_ent)
        return NULL;

    n_move = gar->n_ent + gar->base_idx - idx;
    garray_expand(gar, gar->n_ent + 1);
    rv = garray_move(gar, idx + 1, idx, n_move);
    assert(rv == n_move);
    memcpy(garray_void(gar, idx), ent, gar->ent_size);
    return garray_void(gar, idx);
}

void
garray_set_cmp(garray_t *gar, garray_cmp_t cmp, void *udata)
{
    gar->cmp = cmp;
    gar->udata = udata;
}

size_t
garray_bisect_left(garray_t *gar, void *ent)
{
    size_t lo, hi;

    lo = gar->base_idx;
    hi = gar->n_ent + gar->base_idx;
    while (lo < hi) {
        size_t mid = (lo + hi)/2;
        int c = gar->cmp(gar, garray_void(gar, mid), ent, gar->udata);
        if (c < 0)
            lo = mid + 1;
        else
            hi = mid;
    }
    return lo;
}   

size_t
garray_bisect_right(garray_t *gar, void *ent)
{
    size_t lo, hi;

    lo = gar->base_idx;
    hi = gar->n_ent + gar->base_idx;
    while (lo < hi) {
        size_t mid = (lo + hi)/2;
        int c = gar->cmp(gar, ent, garray_void(gar, mid), gar->udata);
        if (c < 0)
            hi = mid;
        else
            lo = mid + 1;
    }
    return lo;
}

size_t
garray_find_first(garray_t *gar, void *ent)
{
    size_t pos, next_idx;

    next_idx = gar->n_ent + gar->base_idx;
    pos = garray_bisect_left(gar, ent);
    if (pos == next_idx
        || (*gar->cmp)(gar, ent, garray_void(gar, pos), gar->udata) != 0)
        return next_idx;
    return pos;
}

int
garray_cmp_int32(garray_t *gar, void const *a, void const *b, void *udata)
{
    return *(int32 *)a - *(int32 *)b;
}

int
garray_cmp_str(garray_t *gar, void const *a, void const *b, void *udata)
{
    return strcmp(*(char const **)a, *(char const **)b);
}

static void
garray_swap(garray_t *gar, size_t a, size_t b)
{
    char *tmp = ckd_malloc(gar->ent_size);

    memcpy(tmp, garray_void(gar, a), gar->ent_size);
    memcpy(garray_void(gar, a), garray_void(gar, b), gar->ent_size);
    memcpy(garray_void(gar, b), tmp, gar->ent_size);
    ckd_free(tmp);
}

static void
garray_siftdown(garray_t *gar, size_t startpos, size_t endpos)
{
    size_t rootpos = startpos;

    while (rootpos * 2 <= endpos) {
        size_t childpos = rootpos * 2;
        if (childpos < endpos
            && (*gar->cmp)(gar,
                           garray_void(gar, childpos + gar->base_idx),
                           garray_void(gar, childpos + 1 + gar->base_idx),
                           gar->udata) < 0)
            ++childpos;
        if ((*gar->cmp)(gar, garray_void(gar, rootpos + gar->base_idx),
                        garray_void(gar, childpos + gar->base_idx),
                        gar->udata) < 0) {
            garray_swap(gar,
                        rootpos + gar->base_idx,
                        childpos + gar->base_idx);
            rootpos = childpos;
        }
        else
            return;
    }
}

void
garray_heapify(garray_t *gar)
{
    size_t startpos;

    if (gar->n_ent < 2)
        return;

    for (startpos = gar->n_ent / 2;; --startpos) {
        garray_siftdown(gar, startpos, gar->n_ent - 1);
        /* startpos is unsigned */
        if (startpos == 0)
            break;
    }
}

void
garray_sort(garray_t *gar)
{
    size_t endpos;

    if (gar->n_ent < 2)
        return;
    garray_heapify(gar);
    for (endpos = gar->n_ent - 1; endpos > 0; --endpos) {
        garray_swap(gar, endpos + gar->base_idx, gar->base_idx);
        garray_siftdown(gar, gar->base_idx, gar->base_idx + endpos - 1);
    }
}

static void
garray_merge(garray_t *dest, size_t outpos,
             garray_t *left, size_t ls, size_t le,
             garray_t *right, size_t rs, size_t re)
{
    void *lp, *rp, *lpe, *rpe;

    lp = garray_void(left, ls + left->base_idx);
    lpe = garray_void(left, le + left->base_idx);
    rp = garray_void(right, rs + right->base_idx);
    rpe = garray_void(right, re + right->base_idx);

    while (lp < lpe && rp < rpe) {
        int cmp = (*dest->cmp)(left, lp, rp, left->udata);
        if (cmp < 0) {
            garray_put(dest, outpos, lp);
            lp = (char *)lp + left->ent_size;
        }
        else {
            garray_put(dest, outpos, rp);
            rp = (char *)rp + right->ent_size;
        }
        ++outpos;
    }
    while (lp < lpe) {
        garray_put(dest, outpos, lp);
        lp = (char *)lp + left->ent_size;
        ++outpos;
    }
    while (rp < rpe) {
        garray_put(dest, outpos, rp);
        rp = (char *)rp + right->ent_size;
        ++outpos;
    }
}

static void
garray_merge_sort(garray_t *dest, size_t outpos,
                  garray_t *scratch, garray_t *src,
                  size_t startpos, size_t endpos)
{
    size_t middle = (startpos + endpos) / 2;

    assert(endpos >= startpos);
    if (endpos - startpos < 2) {
        memcpy(garray_void(dest, outpos + dest->base_idx),
               garray_void(src, startpos + src->base_idx),
               (endpos - startpos) * src->ent_size);
        return;
    }
    garray_merge_sort(scratch, startpos,
                      dest, src, startpos, middle);
    garray_merge_sort(scratch, middle,
                      dest, src, middle, endpos);
    garray_merge(dest, startpos,
                 scratch, startpos, middle,
                 scratch, middle, endpos);
}

garray_t *
garray_sorted(garray_t *gar)
{
    garray_t *newgar = garray_init(gar->n_ent, gar->ent_size);
    /* Not actually necessary, we hope */
    garray_t *scratch;

    newgar->base_idx = gar->base_idx;
    newgar->cmp = gar->cmp;
    newgar->udata = gar->udata;
    if (gar->n_ent < 2) {
        memcpy(newgar->ent, gar->ent, newgar->n_ent * newgar->ent_size);
        return newgar;
    }

    scratch = garray_init(gar->n_ent, gar->ent_size);
    scratch->base_idx = gar->base_idx;
    scratch->cmp = gar->cmp;
    scratch->udata = gar->udata;

    garray_merge_sort(newgar, 0, scratch, gar, 0, gar->n_ent);
    garray_free(scratch);

    return newgar;
}
