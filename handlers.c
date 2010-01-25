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
#include <dirent.h>


#ifndef DIR_NAME
    #define DIR_NAME "./knowledge/"
#endif

#define FILE_BUFFER 1024

TEXTCAT_SAVE(default)
{
    uchar * fname, * content;
    long i, ret, offset;
    int fd;

    fname = tc_malloc(strlen(id) + strlen(DIR_NAME) + 2);
    sprintf(fname, "%s/%s", DIR_NAME, id);
    mkdir(DIR_NAME, 0777);

    fd = open(fname, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (fd == -1) {
        return TC_FALSE;
    }
    content = tc_malloc(FILE_BUFFER); 
    if (content == NULL) {
        return TC_FALSE;
    }

    /* sort by freq */
    textcat_ngram_sort_by_freq(result);

    for(i=0,offset=0; i < result->size; i++) {
        if (offset + result->ngram[i].size >= FILE_BUFFER - 1) {
            ret    = write(fd, content, offset);
            offset = 0;
        }
        strncpy(content+offset, result->ngram[i].str, result->ngram[i].size);
        offset += result->ngram[i].size+1;
        *(content+ offset-1) = '\n';
    }
    ret = write(fd, content, offset);
    close(fd);
    return TC_TRUE;
}

TEXTCAT_LIST(default)
{
    DIR * fd;
    struct dirent * info;
    int len, i;

    fd = opendir(DIR_NAME);
    if (fd == NULL) {
        return TC_FALSE;
    }
    len = -2; /* . and .. aren't files */
    i   = 0;
    while (readdir(fd))  len++;
    rewinddir(fd);
    *list = tc_malloc(len * sizeof(char *));
    if (*list == NULL) {
        return TC_FALSE;
    }
    while (info = readdir(fd)) {
        if (strcmp(info->d_name, ".") == 0 || strcmp(info->d_name, "..") == 0) {
            continue;
        }
        *(*list+i) = tc_strdup(info->d_name);
        if (*(*list+i) == NULL) {
            return TC_FALSE;
        }
        i++;
    }
    *size = len;
    closedir(fd);
    return TC_TRUE;
}

TEXTCAT_LOAD(default)
{
    int fd;
    int bytes, offset, ncount, i,e;
    uchar * fname,  * content;

    fname = tc_malloc(strlen(id) + strlen(DIR_NAME) + 2);
    sprintf(fname, "%s/%s", DIR_NAME, id);

    fd = open(fname, O_RDONLY);
    if (fd == -1) {
        return TC_FALSE;
    }
    
    content = tc_malloc(FILE_BUFFER);
    ncount  = 0;
    offset  = 0;
    do {
        bytes = read(fd, content + offset, FILE_BUFFER - offset) + offset;
        for (i=0; offset < bytes; offset++) {
            if (*(content+offset) == '\n') {
                result->ngram[ncount].str       = tc_strndup(content+i, offset-i);
                result->ngram[ncount].size      = offset-i;
                result->ngram[ncount].position  = ncount;
                i = offset+1;
                ncount++;
                if (ncount >= max) {
                    break;
                }
            }
        }
        if (ncount >= max) {
            break;
        }
        if (offset > i) {
            offset -= i;
            for (e=0; i < bytes; i++,e++) {
                *(content+e) = *(content+i);
            }
            *(content+e) = '\0';
        } else {
            offset = 0;
        }
    } while (bytes > 0);
    result->size = ncount;
    close(fd);
    return TC_TRUE;
}

TEXTCAT_DISTANCE(default)
{
    int ai, bi, diff;
    long dist;
    int max, match;
    dist = 0;
    max  = a->size > b->size ? a->size : b->size;
    max++;
    match = 0;

    for (ai=0,bi=0; ai < a->size && bi < b->size; ) {
         diff = strcmp(a->ngram[ai].str, b->ngram[bi].str);
         if (diff < 0) {
             ai++;
         } else if (diff > 0) {
             dist += max;
             bi++;
         } else {
             dist += abs(a->ngram[ai].position - b->ngram[bi].position);
             match++;
             bi++;
             ai++;
         }
    }
    dist += max * (b->size - bi);
    return (long)dist/max;
}

/* Default Parsing text callback {{{ */
TEXTCAT_PARSER(default)
{
    size_t i,e,x, valid;
    uchar *ntext;
    /* create a copy of the text in order to do a best-effort
     * to clean it, setting everything to lower-case, removing
     * non-alpha and whitespaces.
     */
    ntext = tc_malloc(length+1);
    for (i=0, e=0; i < length; i++) {
        if (isalpha(text[i])) {
            ntext[e++] = tolower(text[i]);
        } else {
            while (i++ < length && !isalpha(text[i]));
            ntext[e++] = ' ';
            i--;
        }
    }
    ntext[e++] = '\0';
    length     = e - 1;
    /* extract the ngrams, and pass-it to the library (with the callback) */
    for (e=0; e < length; e++) {
        for (i=min; i <= max; i++) {
            if (e+i > length) {
                break;
            }


            /* allow spaces only at the beging and end (in order to reduce n-grams quantities) {{{ */
            valid = 1;
            for (x=1; x < i-1; x++) {
                if (isblank(*(ntext+e+x))) {
                    valid = 0;
                    break;
                }
            }
            if (valid==0) {
                continue;
            }
            /* }}} */ 

            if (set_ngram(tc, ntext+e, i) == TC_FALSE) {
                return TC_FALSE;
            }
        }
    }
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
