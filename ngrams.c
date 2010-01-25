/*
   +----------------------------------------------------------------------+
   | Copyright (c) 2010 The PHP Group                                     |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author:  César Rodas <crodas@member.fsf.org>                         |
   +----------------------------------------------------------------------+
 */

#include "textcat.h"
#include "textcat_internal.h"

/* simple_hash(const uchar *, int) {{{ */
long textcat_simple_hash(const uchar *str, size_t len, size_t max_number)
{
	long hash = len * 13;
	while (--len > 0) {
		hash = (hash<<5)-hash + *str++;
	}
	return (long)hash & max_number;
}
/* }}} */

/* textcat_find_ngram(const ngram_set *, const uchar *, int, ngram **) {{{ */
Bool textcat_ngram_find(const ngram_set * nset, const uchar * key, size_t len, ngram_t ** item)
{
    ngram_t * entry;
   
    for (entry = nset->first; entry!=NULL; entry = entry->next) 
    {
        if (entry->len == len && strncmp(entry->str, key, len) == 0) {
            *item = entry;
            return TC_TRUE;
        }
    }
    return TC_FALSE;
}
/* }}} */
 
/* textcat_ngram_incr(TextCat *, ngram_set *, const uchar *, size_t) {{{ */
Bool textcat_ngram_incr_ex(TextCat * tc, const uchar * key, size_t len, long freq)
{
    ngram_t * item;
    ngram_set * nset;
    int spot;
    spot = textcat_simple_hash(key, len, tc->hash_size - 1);
    nset = &(tc->hash.table[spot]);
    if (textcat_ngram_find(nset, key, len, &item) == TC_TRUE) {
        item->freq += freq;
    } else {
        if (textcat_ngram_create(tc, nset, key, len, &item)  == TC_FALSE) {
            return TC_FALSE;
        }
        item->freq += freq;
    }
    return TC_TRUE;
}

Bool textcat_ngram_incr(TextCat * tc, const uchar * key, size_t len)
{
    return textcat_ngram_incr_ex(tc, key, len, 1);
}
/* }}} */

/* textcat_ngram_create(TextCat *, ngram_set *, const uchar *, int, ngram **) {{{ */
Bool textcat_ngram_create(TextCat * tc, ngram_set * nset, const uchar * key, size_t len, ngram_t ** ritem)
{
    ngram_t * item;
    item = mempool_malloc(tc->temp, sizeof(ngram_t));
    CHECK_MEM(item)

    /* setup the new N-gram */
    item->str  = mempool_strndup(tc->temp, key, len);
    item->freq = 0;
    item->len  = len;
    item->next = NULL;

    CHECK_MEM(item->str);
    
    if (nset->first == NULL) {
        nset->first = item;
    }
    if (nset->last != NULL) {
        nset->last->next = item;
    }
    *ritem = item;
    nset->last = item;
    nset->total++;
    tc->hash.ngrams++;

    return TC_TRUE;
} 
/* }}} */

/* textcat_init_hash(TextCat * tc) {{{ */
Bool textcat_init_hash(TextCat * tc)
{
    ngram_set * table;
    int i;

    table = mempool_calloc(tc->temp, tc->hash_size, sizeof(ngram_set));

    CHECK_MEM(table)

    for (i=0; i < tc->hash_size; i++) {
        table[i].first = NULL;
        table[i].last  = NULL;
        table[i].total = 0;
    }

    tc->hash.table  = table;
    tc->hash.ngrams = 0;
    tc->hash.size   = tc->hash_size;
    return TC_TRUE; 
}
/* }}} */

/* textcat_destroy_hash(TextCat * tc)  {{{ */
void textcat_destroy_hash(TextCat * tc) 
{
    mempool_reset(tc->temp);
}
/* }}} */

/* textcat_copy_result(TextCat * tc, NGrams ** result) {{{ */
static int textcat_hash_sort(const void * a, const void *b)
{
    int diff = 0;
    ngram_t *aa, *bb;
    aa = *(ngram_t **) a;
    bb = *(ngram_t **) b;
    diff = bb->freq - aa->freq;
    /* if they have the same frequency, let's order 
     * by string, in a descendent fashion
     */
    if (diff == 0) {
        diff = strcmp(bb->str, aa->str);
    }
    return diff;
}

Bool textcat_copy_result(TextCat * tc, NGrams ** result)
{
    NGrams * ngrams;
    ngram_t * entry, **temp;
    long i, e;
    long length;

    temp = (ngram_t **) mempool_malloc(tc->temp, sizeof(ngram_t *) * tc->hash.ngrams);
    CHECK_MEM(temp);

    for (i=0, e=0; i < tc->hash.size; i++) {
        for (entry = tc->hash.table[i].first; entry ; entry = entry->next) {
            *(temp+e) = entry;
            e++;
        }
    }

    /* simple checking */
    assert(e == tc->hash.ngrams);

    /* sort by the hash by frequency */
    qsort(temp, tc->hash.ngrams, sizeof(ngram_t *), &textcat_hash_sort);

    /* guess the number of desires N-grams */
    length = tc->hash.ngrams > tc->max_ngrams ?  tc->max_ngrams : tc->hash.ngrams;

    /* preparing the result */
    ngrams = (NGrams *) mempool_malloc(tc->memory, sizeof(NGrams));
    CHECK_MEM(ngrams);
    ngrams->ngram = (NGram *) mempool_calloc(tc->memory, length, sizeof(NGram));
    CHECK_MEM(ngrams->ngram);
    ngrams->size = length;

    /* copying the first 'length' ngrams */
    for (i=0; i < length; i++) {
        ngrams->ngram[i].str      = mempool_strndup(tc->memory,temp[i]->str, temp[i]->len);
        ngrams->ngram[i].freq     = temp[i]->freq;
        ngrams->ngram[i].size     = temp[i]->len;
        ngrams->ngram[i].position = i;
        CHECK_MEM(ngrams->ngram[i].str);
    }

    /* sort by string (for fast comparition) */
    textcat_ngram_sort_by_str(ngrams);

    *result = ngrams;

    return TC_TRUE;
}
/* }}} */

/* Sorting {{{ */
static int textcat_qsort_fnc_freq(const void * a, const void * b)
{   
    int diff;
    NGram *aa, *bb;
    aa = (NGram *) a;
    bb = (NGram *) b;
    diff = bb->freq - aa->freq;
    /* if they have the same frequency, let's order 
     * by string, in a descendent fashion
     */
    if (diff == 0) {
        diff = strcmp(bb->str, aa->str);
    }
    return diff;
}

static int textcat_qsort_fnc_str(const void * a, const void * b)
{
    NGram *aa, *bb;
    aa = (NGram *) a;
    bb = (NGram *) b;
    return strcmp(aa->str, bb->str);
}

void textcat_ngram_sort_by_freq(NGrams * ngrams)
{
    qsort(ngrams->ngram, ngrams->size, sizeof(NGram), textcat_qsort_fnc_freq);
}

void textcat_ngram_sort_by_str(NGrams * ngrams)
{
    qsort(ngrams->ngram, ngrams->size, sizeof(NGram), textcat_qsort_fnc_str);
}

/* }}} */

/* textcat_result_merge(TextCat *, result_stack *, NGrams ** ) {{{ */
Bool textcat_result_merge(TextCat *tc, result_stack * stack, NGrams ** result)
{
    NGram * tmp;
    long i;
    if (textcat_init_hash(tc) == TC_FALSE) {
        return TC_FALSE;
    }
    while (stack) {
        for (i=0; i < stack->result->size; i++) {
            tmp = &stack->result->ngram[i];
            if (textcat_ngram_incr_ex(tc, tmp->str, tmp->size, tmp->freq) == TC_FALSE) {
                textcat_destroy_hash(tc);
                return TC_FALSE;
            }
        }
        stack = stack->next;
    }
    if (textcat_copy_result(tc, result) == TC_FALSE) {
        textcat_destroy_hash(tc);
        return TC_FALSE;
    }
    textcat_destroy_hash(tc);
    return TC_TRUE;
}
/* }}} */

void ngrams_print(NGrams * ng)
{
    int i;
    for (i=0; i < ng->size; i++) {
        printf("[%s] %d %d\n", ng->ngram[i].str, ng->ngram[i].position, ng->ngram[i].freq);
    }
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
