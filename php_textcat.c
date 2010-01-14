/*
  +----------------------------------------------------------------------+
  | Copyright (c) 2009 The PHP Group                                     |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.0 of the PHP license,       |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_0.txt.                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Cesar Rodas <crodas@member.fsf.org>                         |
  +----------------------------------------------------------------------+
*/

/* $ Id: $ */

#include "php_textcat.h"
#include "textcat.h"

/****************************************
  Helper macros
****************************************/

/* macros {{{ */
#define TEXTCAT_METHOD_INIT_VARS             \
    zval* object = getThis(); \
    php_textcat_t*  obj = NULL;      \

#define TEXTCAT_METHOD_FETCH_OBJECT                                             \
    obj = (php_textcat_t *) zend_object_store_get_object( object TSRMLS_CC );   \
    if (obj->tc == NULL) {    \
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "BaseTextCat constructor was not called");    \
        return;    \
    }

#define TEXTCAT_FETCH_PARAM(x) \
    x = (textcat_callback_param *) obj->tc->param;\
    x->ptr = getThis();

#define NGRAMS_TO_ARRAYP(ngram_p, array_p) \
    MAKE_STD_ZVAL(array_p); \
    NGRAMS_TO_ARRAY(ngram_p, array_p)

#define NGRAMS_TO_ARRAY(ngram_p, array) \
    array_init(array); \
    textcat_ngram_sort_by_freq(ngram_p); \
    long _i; \
    for (_i=0; _i < ngram_p->size; _i++) { \
        add_next_index_stringl(array, ngram_p->ngram[_i].str, result->ngram[_i].size, 1); \
    } \

#define ENDFOREACH(array) \
    zval_dtor(&value); } \
    zend_hash_internal_pointer_reset(Z_ARRVAL_P(array));  }

#define FOREACH(array) FOREACH_EX(array, 1)

#define FOREACH_EX(array, exp) {\
    HashPosition pos; \
    zval **current, value; \
    char *key;\
    uint keylen;\
    ulong idx;\
    int type;\
    for (zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(array), &pos); \
            zend_hash_get_current_data_ex(Z_ARRVAL_P(array), (void**)&current, &pos) == SUCCESS && exp; \
            zend_hash_move_forward_ex(Z_ARRVAL_P(array), &pos) \
    ) { \
        value = **current;\
        zval_copy_ctor(&value);\
        INIT_PZVAL(&value);\
        convert_to_string(&value);\

#define VAR_DUMP(value)  do { \
    zval * args[1], funcname; \
    ZVAL_STRING(&funcname, "var_dump", 0); \ 
    args[0] = value; \
    call_user_function(EG(function_table), NULL, &funcname, &return_value, 1, args TSRMLS_CC); \
    } while(0);

#define FETCH_DIR \
    dir = zend_read_property(textcat_ce, object, "_path", sizeof("_path")-1,1  TSRMLS_CC);\
\
    if (Z_TYPE_P(dir) == IS_NULL) {\
        php_textcat_throw_exception("You must set a directory to save, TextCat::setDirectoy", 1 TSRMLS_CC);\
        RETURN_FALSE;\
    }

#define TEXTCAT_GET_FILENAME(size, filename, path) \
    FETCH_DIR \
    do {\
    int offset = 0;\
    path   = emalloc(size +  Z_STRLEN_P(dir) + 10);\
    strncpy(path, Z_STRVAL_P(dir), Z_STRLEN_P(dir)); \
    offset += Z_STRLEN_P(dir); \
    strncpy(path + offset, "/", 1); \
    offset += 1; \
    strncpy(path + offset, filename, size); \
    offset += size; \
    strncpy(path + offset, DEFAULT_FILE_EXTENSION, strlen(DEFAULT_FILE_EXTENSION) ); \
    offset += strlen(DEFAULT_FILE_EXTENSION); \
    *(path+offset) = '\0'; \
    } while(0);

#if ZTS 
    #define PREPARE_CALLBACK_THREAD TSRMLS_D = p->TSRMLS_C; 
#else
    #define PREPARE_CALLBACK_THREAD
#endif 

#define PREPARE_CALLBACK \
    textcat_callback_param  * param; \
    zval retval; \
    zval * args[2], funcname;\
    \
    param = (textcat_callback_param *) context; \
    PREPARE_CALLBACK_THREAD

#define PREPARE_CALLBACK_FNC(name) \
    PREPARE_CALLBACK \
    ZVAL_STRING(&funcname, name, 0);\
    
#define TEXTCAT_ERROR_TO_EXCEPTION\
    char * errstr;\
    switch (obj->tc->error) {\
    case TC_ERR_MEM:\
        errstr = "TextCat library run out of memory";\
        break;\
    case TC_NO_NGRAM:\
        errstr = "There is no sample, please add it calling method addSample($str)";\
        break;\
    case TC_ERR_CALLBACK:\
        errstr = "Unknown error with callback.";\
        break;\
    default:\
        errstr = "Unknown error.";\
        break;\
    } \
    php_textcat_throw_exception(errstr, obj->tc->error TSRMLS_CC);\
    RETURN_FALSE;

/* }}} */

/****************************************
  Structures and definitions
****************************************/

typedef struct {
    zend_object zo;
    TextCat * tc;
} php_textcat_t;

static zend_class_entry * base_textcat_ce = NULL;
static zend_class_entry * textcat_ce = NULL;
static zend_class_entry * textcat_exception_ce = NULL;


/****************************************
  Forward declarations
****************************************/

static void php_textcat_destroy(php_textcat_t *obj TSRMLS_DC);
static void php_textcat_throw_exception(char *sqlstate, int errorno TSRMLS_DC);

/****************************************
  TextCat callback support 
****************************************/

/* {{{  php_textcat_knowledge_save()  */
static Bool php_textcat_knowledge_save(void * memory, const uchar * id, NGrams * result, void * context)
{
    zval *ngram_array; 
    PREPARE_CALLBACK

    NGRAMS_TO_ARRAYP(result, ngram_array);

    /* Send the result to our PHP method */
    ZVAL_STRING(&funcname, "_save", 0);
    args[0] = param->language;
    args[1] = ngram_array;

    call_user_function(NULL, &param->ptr, &funcname, &retval, 2, args TSRMLS_CC);

    zval_dtor(ngram_array);

    return Z_BVAL(retval) || Z_TYPE(retval) == IS_NULL ? TC_TRUE : TC_FALSE;
}
 /* }}} */

/* {{{ php_textcat_knowledge_load() */
static Bool php_textcat_knowledge_load(void * memory, const uchar * id, NGrams * result, int max, void * context)
{
    zval * name;
    int i;
    PREPARE_CALLBACK_FNC("_load")


    MAKE_STD_ZVAL(name);
    ZVAL_STRING(name, id, 0); 
    
    args[0] = name;
    zval return_value;
    call_user_function(NULL, &param->ptr, &funcname, &retval, 1, args TSRMLS_CC);

    i=0;
    FOREACH_EX(&retval, i < max)
        result->ngram[i].str      = mempool_strndup(memory, Z_STRVAL(value), Z_STRLEN(value));
        result->ngram[i].size     = Z_STRLEN(value);
        result->ngram[i].position = i++;
    ENDFOREACH(&retval)
    result->size = i;
    zval_dtor(&retval);
    efree(name);
}
/* }}} */

/* {{{  php_textcat_knowledge_list()  */
static Bool php_textcat_knowledge_list(void * memory, uchar *** list, int * max, void * context)
{
    PREPARE_CALLBACK_FNC("_list")

    call_user_function(NULL, &param->ptr, &funcname, &retval, 0, NULL TSRMLS_CC);

    if (Z_TYPE(retval) != IS_ARRAY) {
        *list = NULL;
        *max  = 0;
        zval_dtor(&retval);
        php_textcat_throw_exception("_list() method must return an array", 1 TSRMLS_CC);
        return TC_FALSE;
    }
    int i   = 0;
    *max = zend_hash_num_elements(Z_ARRVAL(retval));
    *list = mempool_malloc(memory, *max * sizeof(char *));
    FOREACH(&retval)
        *(*list+i) = mempool_strndup(memory, Z_STRVAL(value), Z_STRLEN(value));
        i++;
    ENDFOREACH(&retval)
    zval_dtor(&retval);
    return TC_TRUE;
}
 /* }}} */


/****************************************
  Method implementations
****************************************/

/* {{{ BaseTextCat::__construct()
   Creates a new filter with the specified capacity */
static PHP_METHOD(BaseTextCat, __construct)
{
}
/* }}} */

/* {{{ BaseTextCat::add(string item)
   Adds an item to the filter */
static PHP_METHOD(BaseTextCat, add)
{
    char *data = NULL;
    int   data_len;
    TEXTCAT_METHOD_INIT_VARS;
    Bool status;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &data, &data_len) == FAILURE) {
        return;
    }

    TEXTCAT_METHOD_FETCH_OBJECT;

    status = textcat_add(obj->tc, data, data_len);
    if (status != TC_TRUE) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "could not add data to filter");
        RETURN_FALSE;
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ BaseTextCat::getKnowledges()
   Get a list of all saved knowledges */
static PHP_METHOD(BaseTextCat, getKnowledges)
{

    uchar ** knowledges;
    int len, i;

    TEXTCAT_METHOD_INIT_VARS;
    Bool status;
    textcat_callback_param  * param;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "") == FAILURE) {
        return;
    }
    TEXTCAT_METHOD_FETCH_OBJECT;

    TEXTCAT_FETCH_PARAM(param)

    if (TextCat_list(obj->tc, &knowledges, &len) == TC_FALSE) {
        RETURN_FALSE;
    }

    array_init(return_value);
    for (i=0; i < len; i++) {
        add_next_index_string(return_value, knowledges[i], 1);
    }
}
/* }}} */

/* {{{ BaseTextCat::save(string item)
   Checks if the filter has the specified item */
static PHP_METHOD(BaseTextCat, save)
{
    char *data = NULL;
    int   data_len;
    TEXTCAT_METHOD_INIT_VARS;
    Bool status;
    textcat_callback_param  * param;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &data, &data_len) == FAILURE) {
        return;
    }
    TEXTCAT_METHOD_FETCH_OBJECT;

    TEXTCAT_FETCH_PARAM(param)

    MAKE_STD_ZVAL(param->language);
    ZVAL_STRINGL(param->language, data, data_len, 1); 
    status = TextCat_save(obj->tc, data);
    zval_dtor(param->language);

    if (status == TC_TRUE) {
        RETURN_TRUE;
    } else {
        TEXTCAT_ERROR_TO_EXCEPTION
    }

}
/* }}} */

 /* {{{ BaseTextCat::addSample(string)
   Returns array with all the ngrams of a text*/
static PHP_METHOD(BaseTextCat, addSample)
{
    char * text;
    long text_len;

    TEXTCAT_METHOD_INIT_VARS;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &text, &text_len) == FAILURE) {
        return;
    }
    TEXTCAT_METHOD_FETCH_OBJECT;

    if (TextCat_parse_ex(obj->tc, text, (size_t)text_len, NULL, TC_TRUE) != TC_TRUE) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}
/* }}} */

 /* {{{ BaseTextCat::Parse(string)
   Returns array with all the ngrams of a text*/
static PHP_METHOD(BaseTextCat, parse)
{
    char * text;
    long text_len;
    NGrams * result;
    TEXTCAT_METHOD_INIT_VARS;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &text, &text_len) == FAILURE) {
        return;
    }
    TEXTCAT_METHOD_FETCH_OBJECT;

    if (TextCat_parse_ex(obj->tc, text, text_len, &result, TC_FALSE) != TC_TRUE) {
        RETURN_FALSE;
    }
    NGRAMS_TO_ARRAY(result,return_value);
}
/* }}} */

 /* {{{ BaseTextCat::getCategory(string)
   Returns array with all the ngrams of a text*/
static PHP_METHOD(BaseTextCat, getCategory)
{
    char * text;
    long text_len;
    int len, i;
    uchar ** result;
    textcat_callback_param  * param;

    TEXTCAT_METHOD_INIT_VARS;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &text, &text_len) == FAILURE) {
        return;
    }
    TEXTCAT_METHOD_FETCH_OBJECT;

    TEXTCAT_FETCH_PARAM(param)

    if (TextCat_getCategory(obj->tc, text, text_len, &result, &len) != TC_TRUE) {
        TEXTCAT_ERROR_TO_EXCEPTION;
    }
    if (len == 1) {
        RETVAL_STRING(result[0], 1);
    } else {
        array_init(return_value);
        for (i=0; i < len; i++) {
            add_next_index_string(return_value, result[i], 1);
        }
    }
}
/* }}} */

/* {{{ BaseTextCat::getInfo()
   Returns array with filter information */
static PHP_METHOD(BaseTextCat, getInfo)
{
    TEXTCAT_METHOD_INIT_VARS;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "") == FAILURE) {
        return;
    }

    TEXTCAT_METHOD_FETCH_OBJECT;

    array_init(return_value);
    add_assoc_long_ex(return_value, ZEND_STRS("min_ngram_len"), obj->tc->min_ngram_len);
    add_assoc_long_ex(return_value, ZEND_STRS("max_ngram_len"), obj->tc->max_ngram_len);
    add_assoc_double_ex(return_value, ZEND_STRS("threshold"), obj->tc->threshold);
    add_assoc_long_ex(return_value, ZEND_STRS("hash_size"), obj->tc->hash_size);
    add_assoc_long_ex(return_value, ZEND_STRS("memory_alloc_size"), obj->tc->allocate_size);
}
/* }}} */

/*    {{{ TextCat::_save(string)
    Saves the N-grams into a file */
static PHP_METHOD(TextCat, _save)
{
    TEXTCAT_METHOD_INIT_VARS;
    char * filename;
    long size;
    zval * array;
    php_stream * stream;
    char * path;
    zval * dir;
    int offset;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sa", &filename, &size, &array) == FAILURE) {
        return;
    }
    TEXTCAT_METHOD_FETCH_OBJECT;

    TEXTCAT_GET_FILENAME(size, filename, path);

    stream = php_stream_open_wrapper(path,"wb",REPORT_ERRORS, NULL);
    efree(path);

    if (!stream) {
        RETURN_FALSE;
    }

    #define TEXT_BUFFER_SIZE 4096
    unsigned char * buffer;
    buffer = emalloc(TEXT_BUFFER_SIZE);
    offset = 0;
    FOREACH(array);
        char * str_value;        
        int len;
        len       = Z_STRLEN(value);
        str_value = Z_STRVAL(value);
        if (offset + len >= TEXT_BUFFER_SIZE - 1) {
            php_stream_write(stream, buffer, offset);
        }
        strncpy(buffer+offset, str_value, len);
        offset += len + 1;
        *(buffer+offset-1) = '\n';
    ENDFOREACH(array);
    #undef TEXT_BUFFER_SIZE
    php_stream_write(stream, buffer, offset);
    php_stream_close(stream);
    efree(buffer);
    RETURN_TRUE;
}
/* }}} */

/*    {{{ TextCat::_load()
    Read the N-grams content from a file */
static PHP_METHOD(TextCat, _load)
{
    zval * dir;
    char **namelist;
    long size, i, e;
    char * path, * filename, *contents;
    uchar * ngram_tmp;
    php_stream * stream;

    TEXTCAT_METHOD_INIT_VARS;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &filename, &size) == FAILURE) {
        return;
    }

    TEXTCAT_METHOD_FETCH_OBJECT;

    TEXTCAT_GET_FILENAME(size, filename, path);

    stream = php_stream_open_wrapper(path,"rb",REPORT_ERRORS, NULL);
    efree(path);
    array_init(return_value);
    if (!stream) {
        return;
    }
    if ((size = php_stream_copy_to_mem(stream, &contents, PHP_STREAM_COPY_ALL, 0)) > 0) {
        for (i=0,e=0; i < size; i++) {
            if (*(contents+i) == '\n') {
                *(contents+i) = '\0';
                add_next_index_stringl(return_value, contents+e, i-e, 1);
                e = i+1;
            }
        }
    }
    php_stream_close(stream);
    efree(contents);
}
/* }}} */

/*    {{{ TextCat::_list()
    Return a list of N-grams saved */
static PHP_METHOD(TextCat, _list)
{
    zval * dir;
    char **namelist;
    int i, n;

    TEXTCAT_METHOD_INIT_VARS;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "") == FAILURE) {
        return;
    }

    TEXTCAT_METHOD_FETCH_OBJECT;

    FETCH_DIR

    n = php_stream_scandir(Z_STRVAL_P(dir), &namelist, NULL, (void *) php_stream_dirent_alphasort);
    array_init(return_value);
    for (i = 0; i < n; i++) {
        int offset = strlen(namelist[i]) - strlen(DEFAULT_FILE_EXTENSION);
        if (i > 1 && strcmp(namelist[i] + offset, DEFAULT_FILE_EXTENSION) == 0) {
            namelist[i][ offset ] = '\0';
            add_next_index_stringl(return_value, namelist[i], offset, 0); 
        } else {
            efree(namelist[i]);
        }
    }
    if (n > 0) {
        efree(namelist);
    }
}
/* }}} */

/*    {{{ TextCat::setDirectory($path)
    Set the base directory where the n-grams output are going to be saved */
static PHP_METHOD(TextCat, setDirectory)
{
    char * path;
    int path_len;
    int did;

    TEXTCAT_METHOD_INIT_VARS;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &path_len) == FAILURE) {
        return;
    }
    *(path+path_len) = '\0';

    if ((did=opendir(path)) == -1) {
        RETURN_FALSE;
    }
    closedir(did);
    zend_update_property_stringl(textcat_ce, object, "_path", sizeof("_path")-1, path, path_len  TSRMLS_CC);
    RETURN_TRUE;
}
/* }}} */

/****************************************
  Internal support code
****************************************/

/* {{{ php_textcat_throw_exception(char * message, int errno TSRMLS_DC)
    Throws an Exception  */
static void php_textcat_throw_exception(char * message, int errorno TSRMLS_DC)
{
    zval * tc_ex;

    MAKE_STD_ZVAL(tc_ex);
    object_init_ex(tc_ex, textcat_exception_ce);

    zend_update_property_string(textcat_exception_ce, tc_ex, "message", sizeof("message") - 1,
            message TSRMLS_CC);
    zend_throw_exception_object(tc_ex TSRMLS_CC);
}
/* }}} */

/* {{{ constructor/destructor */
static void php_textcat_destroy(php_textcat_t *obj TSRMLS_DC)
{
    if (obj->tc) {
        efree(obj->tc->param);
        TextCat_Destroy(obj->tc);
    }
}

static void php_textcat_free_storage(php_textcat_t *obj TSRMLS_DC)
{
    zend_object_std_dtor(&obj->zo TSRMLS_CC);

    php_textcat_destroy(obj TSRMLS_CC);
    efree(obj);
}

zend_object_value php_textcat_new(zend_class_entry *ce TSRMLS_DC)
{
    zend_object_value retval;
    php_textcat_t *obj;
    zval *tmp;

    obj = (php_textcat_t *) emalloc(sizeof(php_textcat_t));
    memset(obj, 0, sizeof(php_textcat_t));
    zend_object_std_init(&obj->zo, ce TSRMLS_CC);
    zend_hash_copy(obj->zo.properties, &ce->default_properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));

    retval.handle = zend_objects_store_put(obj, (zend_objects_store_dtor_t)zend_objects_destroy_object, (zend_objects_free_object_storage_t)php_textcat_free_storage, NULL TSRMLS_CC);
    retval.handlers = zend_get_std_object_handlers();

    /* Start textcat object  {{{*/
    if (TextCat_Init(&obj->tc) == TC_FALSE) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "could not create textcat object");
        return;
    }

    /* tie this class instance to the TextCat instance (for callback) */
    textcat_callback_param * param = emalloc(sizeof(textcat_callback_param));
    #ifdef ZTS
    param->TSRMLS_C = TSRMLS_C;
    #endif
    obj->tc->param  = param;

    /* preparing callback */
    obj->tc->save = php_textcat_knowledge_save;
    obj->tc->list = php_textcat_knowledge_list;
    obj->tc->load = php_textcat_knowledge_load;
    /* }}} */

    return retval;
}
/* }}} */
 
/* {{{ methods arginfo */
ZEND_BEGIN_ARG_INFO_EX(arginfo___construct, 0, 0, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_add, 0)
    ZEND_ARG_INFO(0, data)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_save, 0)
    ZEND_ARG_INFO(0, language)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_parse, 0)
    ZEND_ARG_INFO(0, text)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo__save, 0)
    ZEND_ARG_INFO(0, language)
    ZEND_ARG_ARRAY_INFO(0, ngrams, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_getInfo, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo__load, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_setDir, 0)
    ZEND_ARG_INFO(0, path)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ textcat_class_methods */
static zend_function_entry base_textcat_class_methods[] = {
    PHP_ME(BaseTextCat, __construct,  arginfo___construct, ZEND_ACC_PUBLIC)
    PHP_ME(BaseTextCat, add,     arginfo_add,         ZEND_ACC_PUBLIC)
    PHP_ME(BaseTextCat, getInfo, arginfo_getInfo,     ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    PHP_ME(BaseTextCat, save,    arginfo_save,     ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    PHP_ME(BaseTextCat, parse,  arginfo_parse, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    PHP_ME(BaseTextCat, addSample,  arginfo_parse, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    PHP_ME(BaseTextCat, getKnowledges,  arginfo_getInfo, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    PHP_ME(BaseTextCat, getCategory,  arginfo_parse, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    /* abstract methods */
    ZEND_FENTRY(_save, NULL, arginfo__save, ZEND_ACC_PROTECTED | ZEND_ACC_ABSTRACT)
    ZEND_FENTRY(_load, NULL, arginfo__load, ZEND_ACC_PROTECTED | ZEND_ACC_ABSTRACT)
    ZEND_FENTRY(_list, NULL, arginfo__load, ZEND_ACC_PROTECTED | ZEND_ACC_ABSTRACT)
    { NULL, NULL, NULL }
};

static zend_function_entry textcat_class_methods[] = {
    PHP_ME(TextCat, _save, arginfo__save, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    PHP_ME(TextCat, _load, arginfo__load, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    PHP_ME(TextCat, _list, arginfo__load, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    PHP_ME(TextCat, setDirectory, arginfo_setDir, ZEND_ACC_PUBLIC | ZEND_ACC_FINAL)
    { NULL, NULL, NULL }
};
static zend_function_entry textcat_exception_methods[] = {
    {NULL, NULL, NULL}
};
/* }}} * /

/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(textcat)
{
    zend_class_entry _base_textcat, _textcat;
    zend_class_entry tc_exception;

    /* BaseTextCat */
    INIT_CLASS_ENTRY(_base_textcat, "BaseTextCat", base_textcat_class_methods);
    base_textcat_ce = zend_register_internal_class(&_base_textcat TSRMLS_CC);
    base_textcat_ce->create_object = php_textcat_new;

    /* TextCat */
    INIT_CLASS_ENTRY(_textcat, "TextCat", textcat_class_methods);

    textcat_ce = zend_register_internal_class_ex(&_textcat, base_textcat_ce, NULL TSRMLS_CC);
    zend_declare_property_null(textcat_ce, "_path", sizeof("_path")-1, ZEND_ACC_PROTECTED  TSRMLS_CC);

    INIT_CLASS_ENTRY(tc_exception, "TextCatException", textcat_exception_methods);
    textcat_exception_ce = zend_register_internal_class_ex(&tc_exception,  zend_exception_get_default(TSRMLS_C), NULL TSRMLS_CC);

    return SUCCESS;
}
/* }}} */

/* {{{ textcat_module_entry
 */
zend_module_entry textcat_module_entry = {
    STANDARD_MODULE_HEADER,
    "textcat",
    NULL,
    PHP_MINIT(textcat),
    NULL,
    NULL,
    NULL,
    PHP_MINFO(textcat),
    PHP_TEXTCAT_VERSION,
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

/* {{{ PHP_MINFO_FUNCTION */
PHP_MINFO_FUNCTION(textcat)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "textcat support", "enabled");
    php_info_print_table_row(2, "Version", PHP_TEXTCAT_VERSION);
    php_info_print_table_end();
}
/* }}} */

#ifdef COMPILE_DL_TEXTCAT
ZEND_GET_MODULE(textcat)
#endif


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
