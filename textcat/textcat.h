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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Definitions {{{ */
#define TC_HASH_SIZE    1000
#define TC_BUFFER_SIZE  (16 * 1024) 
#define TC_MAX_NGRAMS   400
#define Bool            char
#define uchar           unsigned char
#define TC_TRUE         1
#define TC_FALSE        0
#define TC_FREE         1
#define TC_BUSY         0
#define MIN_NGRAM_LEN   1
#define MAX_NGRAM_LEN   5
#define TC_THRESHOLD    1.003

#define TC_OK               TC_TRUE
#define TC_ERR              -1
#define TC_ERR_MEM          -2
#define TC_NO_FILE          -3
#define TC_ERR_FILE_SIZE    -4
#define TC_NO_NGRAM         -5
#define TC_ERR_CALLBACK     -6
#define TC_ERR_NO_KNOWLEDGE -7
/* }}} */

/* Data types {{{ */
typedef struct {
    uchar * str;
    long freq;
    int len;
    struct ngram_t * next;
} ngram_t;

typedef struct {
    /* linked list */
    ngram_t * first;
    ngram_t * last;
    long total;
} ngram_set;

typedef struct {
    ngram_set * table;
    long size;
    long ngrams;
} ngram_hash;

typedef struct result_stack {
    struct NGrams * result;
    struct result_stack * next;
} result_stack;

typedef struct TextCat {
    /* pools of memory */
    void * temp;   /* temporary memory */
    void * memory; /* "return" structures and internal stack */

    /* callback */
    void * (*malloc)(size_t);
    void (*free)(void *);
    Bool (*parse_str)(struct TextCat *, uchar *, size_t , int * (*set_ngram)(struct TextCat *, const uchar *, size_t), void *);
    Bool (*save)(void *, const uchar *, struct NGrams *, void *);
    Bool (*list)(void *, uchar ***, int *, void *);
    Bool (*load)(void *, const uchar *, struct NGram *, int, void *);
    long (*distance)(struct NGrams *, struct NGrams *, void *);
    void * param;

    /* config issues */
    size_t allocate_size;
    int hash_size;
    int min_ngram_len;
    int max_ngram_len;
    int max_ngrams;
    float threshold;


    /* internal stuff */
    ngram_hash hash;
    result_stack  * results;
    uchar ** klNames;
    struct NGrams * klContent;
    int klTotal;

    /* status */
    int error;
    int status;
} TextCat;


typedef struct {
    uchar * str;
    int size;
    long freq;
    long position;
} NGram;

typedef struct NGrams {
    NGram * ngram;
    long size;
} NGrams;
/* }}} */


Bool TextCat_Init_ex(TextCat ** tcc, void * (*xmalloc)(size_t), void (*xfree)(void *));
Bool TextCat_Init(TextCat ** tc);
Bool TextCat_Destroy(TextCat * tc);
Bool TextCat_reset(TextCat * tc);
Bool TextCat_reset_handlers(TextCat * tc);
Bool TextCat_load(TextCat *tc);
Bool TextCat_getCategory(TextCat *tc, const uchar * text, size_t length, uchar *** result, int * n);

Bool TextCat_parse_ex(TextCat * tc, const uchar * text, size_t length,  NGrams ** ngrams, Bool store_stack);
Bool TextCat_parse(TextCat * tc, const uchar * text, size_t length, NGrams ** ngram);
Bool TextCat_parse_file(TextCat * tc, const uchar * filename, NGrams ** ngrams);
Bool TextCat_list(TextCat * tc, uchar *** list, int * len);
Bool TextCat_load(TextCat *tc);

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
