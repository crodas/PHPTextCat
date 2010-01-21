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

#include <string.h>
#include <limits.h>
#include <assert.h>
#include <math.h>
#ifdef HAVE_CONFIG
#   include "config.h"
#endif


#define CHECK_MEM(x)        if (x == NULL) { tc->error = TC_ERR_MEM; return TC_FALSE;  }
#define CHECK_MEM_EX(x,Y)   if (x == NULL) { tc->error = TC_ERR_MEM; Y; return TC_FALSE;  }

                                
#define INIT_MEMORY(x)  \
                        mempool_init(&tc->x, xmalloc, xfree, tc->allocate_size); \
                        CHECK_MEM_EX(tc->x, TextCat_Destroy(tc) )  \

#define LOCK_INSTANCE(tc)   if (tc->status != TC_FREE) {\
                                tc->error = TC_BUSY; \
                                return TC_FALSE; \
                            } \
                            tc->status = TC_BUSY; /* lock it for our thread */ \

#define UNLOCK_INSTANCE(tc)   tc->status = TC_FREE;

typedef struct {
    long dist;
    uchar * name;
} _cands;

/* Backward declarations {{{ */
Bool mempool_init(void ** memory, void * (*xmalloc)(size_t), void (*xfree)(void *), size_t block_size);
void mempool_done(void ** memory);
void * mempool_calloc(void * memory, size_t nmemb, size_t size);
void * mempool_malloc(void * memory, size_t size);
uchar * mempool_strndup(void * memory, uchar * key, size_t len);
void mempool_reset(void * memory);


long textcat_simple_hash(const uchar *p, size_t len, size_t max_number);
Bool textcat_ngram_find(const ngram_set * nset, const uchar * key, size_t len, ngram_t ** item);
Bool textcat_ngram_create(TextCat * tc, ngram_set * nset, const uchar * key, size_t len, ngram_t ** item);
Bool textcat_ngram_incr(TextCat * tc, const uchar * key, size_t len);
Bool textcat_copy_result(TextCat * tc, NGrams ** result);
void textcat_ngram_sort_by_str(NGrams * ngrams);
void textcat_ngram_sort_by_freq(NGrams * ngrams);
Bool textcat_init_hash(TextCat * tc);
void textcat_destroy_hash(TextCat * tc);
Bool textcat_result_merge(TextCat *tc, result_stack * stack, NGrams ** result);
/* */
Bool knowledge_save(void *, const uchar * id, NGrams * ngrams, void *);
Bool knowledge_list(void *, uchar *** list, int * size, void *);
Bool knowledge_load(void * memory, const uchar * id, NGrams * result, int max, void *);
long knowledge_dist(NGrams *a, NGrams *b, void *);
Bool textcat_default_text_parser(TextCat *tc, const uchar * text, size_t length, int * (*set_ngram)(TextCat *, const uchar *, size_t), void *);
/* }}} */

#define mempool_strdup(x,y) mempool_strndup(x, y, strlen(y))

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
