/*
  +----------------------------------------------------------------------+
  | Copyright (c) 2010 The PHP Group                                     |
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

#ifndef PHP_TEXTCAT_H
#define PHP_TEXTCAT_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <php.h>
#include <ext/standard/info.h>

extern zend_module_entry textcat_module_entry;
#define phpext_textcat_ptr &textcat_module_entry

#ifdef PHP_WIN32
    #define PHP_TEXTCAT_API __declspec(dllexport)
#else
    #define PHP_TEXTCAT_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#define PHP_TEXTCAT_VERSION "0.1"

PHP_MINIT_FUNCTION(textcat);
PHP_MINFO_FUNCTION(textcat);

#define DEFAULT_FILE_EXTENSION ".tc\0"

#define CALL_METHOD(Class, Method, retval, thisptr)  PHP_FN(Class##_##Method)(0, retval, NULL, thisptr, 0 TSRMLS_CC);

typedef struct {
    zval * ptr;
    zval * language;
    TSRMLS_D;
} textcat_callback_param;

#endif /* PHP_TEXTCAT_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
