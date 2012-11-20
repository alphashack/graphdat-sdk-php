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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_graphdat.h"
#include "sockets.h"
#include "msgpack.h"

#define timeValToMs(a) ((a.tv_sec * 1000) + (a.tv_usec / 1000))

ZEND_DECLARE_MODULE_GLOBALS(graphdat)

/* True global resources - no need for thread safety here */
static int le_graphdat;

/* {{{ graphdat_functions[]
 *
 * Every user visible function must have an entry in graphdat_functions[].
 */
const zend_function_entry graphdat_functions[] = {
	PHP_FE(confirm_graphdat_compiled,	NULL)		/* For testing, remove later. */
	PHP_FE_END	/* Must be the last line in graphdat_functions[] */
};
/* }}} */

/* {{{ graphdat_module_entry
 */
zend_module_entry graphdat_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"graphdat",
	graphdat_functions,
	PHP_MINIT(graphdat),
	PHP_MSHUTDOWN(graphdat),
	PHP_RINIT(graphdat),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(graphdat),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(graphdat),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_GRAPHDAT
ZEND_GET_MODULE(graphdat)
#endif

/* {{{ PHP_INI
 */

PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("graphdat.socketFile", "/tmp/gd.agent.sock", PHP_INI_ALL, OnUpdateString, socketFile, zend_graphdat_globals, graphdat_globals)
    STD_PHP_INI_ENTRY("graphdat.socketPort", "26873", PHP_INI_ALL, OnUpdateLong, socketPort, zend_graphdat_globals, graphdat_globals)
    STD_PHP_INI_ENTRY("graphdat.debug", "false", PHP_INI_ALL, OnUpdateBool, debug, zend_graphdat_globals, graphdat_globals)
PHP_INI_END()

/* }}} */

/* {{{ php_graphdat_init_globals
 */
static void php_graphdat_init_globals(zend_graphdat_globals *graphdat_globals)
{
    graphdat_globals->debug = true;
    graphdat_globals->socketFD = 0;
}

/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(graphdat)
{
    ZEND_INIT_MODULE_GLOBALS(graphdat, php_graphdat_init_globals, NULL);
	REGISTER_INI_ENTRIES();

    GRAPHDAT_GLOBALS(socketFD) = openSocket(GRAPHDAT_GLOBALS(socketFile), (int) GRAPHDAT_GLOBALS(socketPort));

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(graphdat)
{
	UNREGISTER_INI_ENTRIES();

    closeSocket(GRAPHDAT_GLOBALS(socketFD));

	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(graphdat)
{
    gettimeofday(&GRAPHDAT_GLOBALS(requestStartTime), NULL);

	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(graphdat)
{
    HashTable *serverVars = Z_ARRVAL_P(PG(http_globals)[TRACK_VARS_SERVER]);
    if(GRAPHDAT_GLOBALS(socketFD) == -1)
    {
        DEBUG("Graphdat :: not connected to agent, skipping \n");
       return SUCCESS;
    }
    if(zend_hash_exists(serverVars, "REQUEST_URI", sizeof("REQUEST_URI")) == 0)
    {
        DEBUG("Graphdat :: No value for REQUEST_URI skipping\n");
        return SUCCESS;
    }
    char* requestUri;
    int requestUriLen;
    char* requestMethod;
    int requestMethodLen;
    char* requestLineItem;
    int requestLineItemLen;
    struct timeval timeNow;
    float totalTime;
    zval **requestUriData;
    zval **requestMethodData;
    
    gettimeofday(&timeNow, NULL);
    timeNow = GRAPHDAT_GLOBALS(requestStartTime);
    totalTime = timeValToMs(timeNow) - timeValToMs(timeNow);
    
    if(zend_hash_find(serverVars, "REQUEST_URI", sizeof("REQUEST_URI"), (void **)&requestUriData) == FAILURE)
    {
        // always bail successfully
        DEBUG("Graphdat :: failed getting value for REQUEST_URI skipping\n");
        return SUCCESS;
    }
    requestUri = Z_STRVAL_PP(requestUriData);
    requestUriLen = Z_STRLEN_PP(requestUriData);
    
    if(zend_hash_find(serverVars, "REQUEST_METHOD", sizeof("REQUEST_METHOD"), (void **)&requestMethodData) == FAILURE)
    {
        // always bail successfully
        DEBUG("Graphdat :: failed getting value for REQUEST_METHOD skipping\n");
        return SUCCESS;
    }
    requestMethod = Z_STRVAL_PP(requestMethodData);
    requestMethodLen = Z_STRLEN_PP(requestMethodData);
    
    requestLineItemLen = 1 + requestMethodLen + requestUriLen;
    sprintf(requestLineItem, "%s %s", requestMethod, requestUri);
    
    zend_printf("Request %s took %fms\n", requestLineItem, totalTime);
    
    msgpack_sbuffer* buffer = msgpack_sbuffer_new();
    msgpack_packer* pk = msgpack_packer_new(buffer, msgpack_sbuffer_write);
    // create map with 2 items (route, responsetime)
    msgpack_pack_map(pk, 2);
    // maps are object then key
    // route
    msgpack_pack_raw(pk, requestLineItemLen);
    msgpack_pack_raw_body(pk, requestLineItem, requestLineItemLen);
    msgpack_pack_raw(pk, sizeof("route"));
    msgpack_pack_raw_body(pk, "route", sizeof("route"));
    // response time
    msgpack_pack_double(pk, totalTime);
    msgpack_pack_raw(pk, sizeof("responsetime"));
    msgpack_pack_raw_body(pk, "responsetime", sizeof("responsetime"));
    
    unsigned char len[4];
    len[0] = buffer->size >> 24;
    len[1] = buffer->size >> 16;
    len[2] = buffer->size >> 8;
    len[3] = buffer->size;
    
    socketWrite(GRAPHDAT_GLOBALS(socketFD), &len, 4);
    socketWrite(GRAPHDAT_GLOBALS(socketFD), buffer->data, buffer->size);
    
    msgpack_sbuffer_free(buffer);
    msgpack_packer_free(pk);

    
/*
Need to send the following message to the agent with this info
    var item = {
        type: "Sample",
        source: "HTTP",
        route: sample.Method + ' ' + sample.URL,
        responsetime: sample._ms,
        timestamp: sample._ts,
        cputime: sample['CPU time (ms)'],
        pid: pid,
        context: sample.Context
    };
 */

	return SUCCESS;
}
/* }}} */


/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(graphdat)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "graphdat", "enabled");
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}
/* }}} */


/* Remove the following function when you have succesfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_graphdat_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(confirm_graphdat_compiled)
{
	char *arg = NULL;
	int arg_len, len;
	char *strg;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	len = spprintf(&strg, 0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "graphdat", arg);
	RETURN_STRINGL(strg, len, 0);
}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
