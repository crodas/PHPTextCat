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

/* TextCat_Init(TextCat ** tcc) {{{ */
Bool TextCat_Init_ex(TextCat ** tcc, void * (*xmalloc)(size_t), void (*xfree)(void *))
{
    TextCat * tc;
    tc = (TextCat *) xmalloc(sizeof(TextCat));
    if (tc == NULL) {
        return TC_FALSE;
    }
    tc->malloc        = xmalloc;
    tc->free          = xfree;
    tc->allocate_size = TC_BUFFER_SIZE;
    tc->hash_size     = TC_HASH_SIZE;
    tc->min_ngram_len = MIN_NGRAM_LEN;
    tc->max_ngram_len = MAX_NGRAM_LEN;
    tc->max_ngrams    = TC_MAX_NGRAMS;
    tc->error         = TC_OK;
    tc->status        = TC_FREE;
    tc->threshold     = TC_THRESHOLD;
    tc->memory        = NULL;
    tc->temp          = NULL;
    tc->results       = NULL;
    tc->klNames       = NULL;
    tc->klContent     = NULL;
    tc->param         = NULL;
    tc->klTotal       = -1;
    TextCat_reset_handlers(tc);
    INIT_MEMORY(memory);
    INIT_MEMORY(temp);
    *tcc = tc;
    return TC_TRUE;
}

Bool TextCat_Init(TextCat ** tcc)
{
    return TextCat_Init_ex(tcc, malloc, free);
}
/* }}} */

/* TextCat_reset(Textcat *) {{{ */
Bool TextCat_reset(TextCat * tc)
{
    LOCK_INSTANCE(tc);
    if (tc->memory != NULL) {
        mempool_reset(tc->memory);
    }
    if (tc->temp != NULL) {
        mempool_reset(tc->temp);
    }
    tc->results   = NULL;
    tc->klNames   = NULL;
    tc->klContent = NULL;
    tc->klTotal   = -1;
    UNLOCK_INSTANCE(tc);
    return TC_TRUE;
}
/* }}}

/* TextCat_reset_handler(TextCat * tc) {{{ */
Bool TextCat_reset_handlers(TextCat * tc)
{
    LOCK_INSTANCE(tc);
    tc->parse_str = &textcat_default_text_parser;
    tc->save      = &knowledge_save;
    tc->list      = &knowledge_list;
    tc->load      = &knowledge_load;
    tc->distance  = &knowledge_dist;
    UNLOCK_INSTANCE(tc);
    return TC_TRUE;
}
/* }}} */

/* TextCat_Destroy(TextCat * tc) {{{ */ 
Bool TextCat_Destroy(TextCat * tc) 
{
    LOCK_INSTANCE(tc);
    void (*xfree)(size_t) = tc->free;
    if (tc->memory != NULL) {
        mempool_done(&tc->memory);
    }
    if (tc->temp != NULL) {
        mempool_done(&tc->temp);
    }
    xfree(tc);
}
/* }}} */

/* TextCat_parse_ex(TextCat * tc, const uchar * text, size_t length,  NGrams ** ngrams, Bool store_stack) {{{ */
Bool TextCat_parse_ex(TextCat * tc, const uchar * text, size_t length,  NGrams ** ngrams, Bool store_stack)
{
    NGrams * result;
    result_stack * stack, *stack_temp;

    LOCK_INSTANCE(tc);

    if (textcat_init_hash(tc) == TC_FALSE) {
        UNLOCK_INSTANCE(tc);
        return TC_FALSE;
    }
    
    if (tc->parse_str(tc, text, length, &textcat_ngram_incr, tc->param) == TC_FALSE) {
        textcat_destroy_hash(tc);
        UNLOCK_INSTANCE(tc);
        return TC_FALSE;
    }
    if (textcat_copy_result(tc, &result) == TC_FALSE) {
        textcat_destroy_hash(tc);
        UNLOCK_INSTANCE(tc);
        return TC_FALSE;
    }

    /* add the result to our Result Stack {{{ */
    if (store_stack == TC_TRUE) {
        stack = mempool_malloc(tc->memory, sizeof(result_stack));
        CHECK_MEM_EX(stack, textcat_destroy_hash(tc); tc->status=TC_FREE; )
        stack->result = result;
        stack->next   = NULL;
        if (tc->results == NULL) {
            tc->results = stack;
        } else {
            stack_temp = tc->results;
            while (stack_temp->next != NULL) {
                stack_temp = stack_temp->next;
            }
            stack_temp->next = stack;
        }
    }
    /* }}} */

    if (ngrams != NULL) {
        *ngrams = result;
    }

    textcat_destroy_hash(tc);
    tc->error  = TC_OK;
    UNLOCK_INSTANCE(tc);
    return TC_TRUE;
}
/* }}} */

/* TextCat_parse(TextCat * tc, const uchar * text, size_t length,  NGrams ** ngrams) {{{ */
Bool TextCat_parse(TextCat * tc, const uchar * text, size_t length,  NGrams ** ngrams)
{
    return TextCat_parse_ex(tc, text, length, ngrams, TC_TRUE);
}
/* }}}

/* TextCat_parse_file(TextCat * tc, const uchar * filename, NGrams ** ngrams) {{{ */
Bool TextCat_parse_file(TextCat * tc, const uchar * filename, NGrams ** ngrams)
{
    int fd;
    size_t bytes;
    uchar * buffer;
    NGrams * result;
    result_stack * stack, *stack_temp;

    LOCK_INSTANCE(tc);
    if (textcat_init_hash(tc) == TC_FALSE) {
        UNLOCK_INSTANCE(tc);
        return TC_FALSE;
    }
    

    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        UNLOCK_INSTANCE(tc);
        tc->error = TC_NO_FILE;
        return TC_FALSE;
    }

    buffer = mempool_malloc(tc->temp, 1024);
    CHECK_MEM(buffer);

    do {
        bytes = read(fd, buffer, 1024);
        if (bytes && tc->parse_str(tc, buffer, bytes, &textcat_ngram_incr, tc->param) == TC_FALSE) {
            textcat_destroy_hash(tc);
            UNLOCK_INSTANCE(tc);
            return TC_FALSE;
        }
    } while (bytes > 0);
    close(fd);

    if (textcat_copy_result(tc, &result) == TC_FALSE) {
        textcat_destroy_hash(tc);
        UNLOCK_INSTANCE(tc);
        return TC_FALSE;
    }

    /* add the result to our Result Stack {{{ */
    stack = mempool_malloc(tc->memory, sizeof(result_stack));
    CHECK_MEM_EX(stack, textcat_destroy_hash(tc); tc->status=TC_FREE; )
    stack->result = result;
    stack->next   = NULL;
    if (tc->results == NULL) {
        tc->results = stack;
    } else {
        stack_temp = tc->results;
        while (stack_temp->next != NULL) {
            stack_temp = stack_temp->next;
        }
        stack_temp->next = stack;
    }
    /* }}} */

    if (ngrams != NULL) {
        *ngrams = result;
    }

    textcat_destroy_hash(tc);
    tc->error  = TC_OK;

    UNLOCK_INSTANCE(tc);
    return TC_TRUE;
}
/* }}} */

/* TextCat_save(TextCat *, unsigned uchar *) {{{ */
Bool TextCat_save(TextCat * tc, const uchar * id)
{
    NGrams * results;

    LOCK_INSTANCE(tc);

    if (tc->results == NULL) {
        tc->error = TC_NO_NGRAM;
        return TC_FALSE;
    }
    if (textcat_result_merge(tc, tc->results, &results) == TC_FALSE) {
        return TC_FALSE;
    }
    if (tc->save(tc->temp, id, results, tc->param) == TC_FALSE) {
        tc->error = TC_ERR_CALLBACK;
        return TC_FALSE;
    }
    UNLOCK_INSTANCE(tc);
    TextCat_reset(tc);
    return TC_TRUE;
}
/* }}} */

/* TextCat_list(TextCat * tc, uchar *** list, int * len) {{{ */
Bool TextCat_list(TextCat * tc, uchar *** list, int * len)
{
    Bool ret;
    LOCK_INSTANCE(tc);
    if (tc->klNames ==  NULL) {
        if (tc->list(tc->memory, &tc->klNames, &tc->klTotal, tc->param) == TC_FALSE) {
            tc->error = TC_ERR_CALLBACK;
            UNLOCK_INSTANCE(tc);
            return TC_FALSE;
        }
    }
    if (list != NULL) {
        *list = tc->klNames;
        *len  = tc->klTotal;
    }
    UNLOCK_INSTANCE(tc);
    return TC_TRUE;
}
/* }}} */

/* TextCat_load(TextCat *tc)  {{{  */
Bool TextCat_load(TextCat *tc) 
{
    uchar ** list;
    int len;
    int i;

    if (tc->klContent != NULL) {
        return TC_TRUE;
    }

    if (TextCat_list(tc, NULL, NULL) == TC_FALSE) {
        tc->error = TC_ERR_CALLBACK;
        return TC_FALSE;
    }
    if (tc->klTotal == 0) {
        tc->error = TC_ERR_NO_KNOWLEDGE;
        return TC_FALSE;
    }

    LOCK_INSTANCE(tc);
    tc->klContent = mempool_calloc(tc->memory, tc->klTotal, sizeof(NGrams));

    for (i=0; i < tc->klTotal; i++) {
        tc->klContent[i].ngram = mempool_calloc(tc->memory, tc->max_ngrams, sizeof(NGram));
        CHECK_MEM(tc->klContent[i].ngram);
        tc->klContent[i].ngram->size = tc->max_ngrams;
        if (tc->load(tc->memory, tc->klNames[i], &tc->klContent[i], tc->max_ngrams, tc->param) == TC_FALSE) {
            tc->klContent = NULL;
            tc->error = TC_ERR_CALLBACK;
            UNLOCK_INSTANCE(tc);
            return TC_FALSE;
        }
        /* sort back from String, for fast comparition */
        textcat_ngram_sort_by_str(&tc->klContent[i]);
    }
    UNLOCK_INSTANCE(tc);
    return TC_TRUE;
}
/* }}} */

/* TextCat_getCategory(TextCat *, const uchar *, size_t, uchar **, int *) {{{ */
static int _ranking_sort(void * a, void *b)
{
    _cands * aa, * bb;
    aa = a;
    bb = b;
    return aa->dist - bb->dist;
}

Bool TextCat_getCategory(TextCat *tc, const uchar * text, size_t length, uchar *** result, int * n)
{
    NGrams * ptext;
    int i;
    _cands * dists;
    long threshold;

    if (TextCat_load(tc) == TC_FALSE) {
        return TC_FALSE;
    }

    if (TextCat_parse_ex(tc, text, length, &ptext, TC_FALSE) == TC_FALSE) {
        return TC_FALSE;
    }

    LOCK_INSTANCE(tc);
    dists = mempool_calloc(tc->memory, tc->klTotal, sizeof(_cands));
    for (i=0; i  < tc->klTotal; i++) {
        dists[i].dist = tc->distance(ptext, &tc->klContent[i], tc->param);
        dists[i].name = tc->klNames[i];
    }
    qsort(dists, tc->klTotal, sizeof(_cands),_ranking_sort);

    threshold = (long)(dists[0].dist * tc->threshold);
    for (i=0; i  < tc->klTotal; i++) {
        if (threshold < dists[i].dist) {
            break;
        }
    }
    *n = i;
    *result = mempool_calloc(tc->memory,i, sizeof(uchar *));
    for (i=0; i < *n;i++) {
        *(*result+i) = dists[i].name;
    }
    UNLOCK_INSTANCE(tc);
    return TC_TRUE;
}
/* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
