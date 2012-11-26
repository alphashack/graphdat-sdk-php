/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2012 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef PHP_GRAPHDAT_H
#define PHP_GRAPHDAT_H

#include <sys/time.h>
#include "timers.h"

extern zend_module_entry graphdat_module_entry;
#define phpext_graphdat_ptr &graphdat_module_entry

#ifdef PHP_WIN32
#	define PHP_GRAPHDAT_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_GRAPHDAT_API __attribute__ ((visibility("default")))
#else
#	define PHP_GRAPHDAT_API
#endif



PHP_MINIT_FUNCTION(graphdat);
PHP_MSHUTDOWN_FUNCTION(graphdat);
PHP_RINIT_FUNCTION(graphdat);
PHP_RSHUTDOWN_FUNCTION(graphdat);
PHP_MINFO_FUNCTION(graphdat);

PHP_FUNCTION(graphdat_begin);
PHP_FUNCTION(graphdat_end);

/* 
  	Declare any global variables you may need between the BEGIN
	and END macros here:     
 */
ZEND_BEGIN_MODULE_GLOBALS(graphdat)
  // items for settings
	long socketPort;
    char *socketFile;
    int debug;
    // items that are used when running
    int socketFD;
    int isDirty;
    struct timeval requestStart;
    struct graphdat_timer_list timers;
ZEND_END_MODULE_GLOBALS(graphdat)


/* In every utility function you add that needs to use variables 
   in php_graphdat_globals, call TSRMLS_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as GRAPHDAT_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define GRAPHDAT_GLOBALS(v) TSRMG(graphdat_globals_id, zend_graphdat_globals *, v)
#else
#define GRAPHDAT_GLOBALS(v) (graphdat_globals.v)
#endif


#define PRINTDEBUG(str, ...) if(GRAPHDAT_GLOBALS(debug)) zend_error(E_NOTICE, str, ##__VA_ARGS__)
// #define PRINTDEBUG(str, ...) zend_printf(str, ##__VA_ARGS__)

#endif	/* PHP_GRAPHDAT_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
