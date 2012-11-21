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
#include "base64.h"

#define timeValToMs(a) (((double)a.tv_sec * 1000.0f) + ((double)a.tv_usec / 1000.0f))

#ifndef PHP_FE_END
#ifdef ZEND_FE_END
#define PHP_FE_END ZEND_FE_END
#else
#define PHP_FE_END { NULL, NULL, NULL, 0, 0 }
#endif
#endif

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
    graphdat_globals->debug = 1;
    graphdat_globals->socketFD = 0;
}

/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(graphdat)
{
    ZEND_INIT_MODULE_GLOBALS(graphdat, php_graphdat_init_globals, NULL);
	REGISTER_INI_ENTRIES();

    GRAPHDAT_GLOBALS(socketFD) = openSocket(GRAPHDAT_GLOBALS(socketFile), (int) GRAPHDAT_GLOBALS(socketPort), GRAPHDAT_GLOBALS(debug));

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
        GRAPHDAT_GLOBALS(socketFD) = openSocket(GRAPHDAT_GLOBALS(socketFile), (int) GRAPHDAT_GLOBALS(socketPort), GRAPHDAT_GLOBALS(debug));
    }
    if(GRAPHDAT_GLOBALS(socketFD) == -1)
    {
        PRINTDEBUG("Graphdat :: not connected to agent, skipping \n");
       return SUCCESS;
    }
//    if(zend_hash_exists(serverVars, "REQUEST_URI", sizeof("REQUEST_URI")) == 0)
//    {
//        PRINTDEBUG("Graphdat :: No value for REQUEST_URI skipping\n");
//        return SUCCESS;
//    }
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
    totalTime = timeValToMs(timeNow) - timeValToMs(GRAPHDAT_GLOBALS(requestStartTime));
    
    if(zend_hash_find(serverVars, "REQUEST_URI", sizeof("REQUEST_URI"), (void **)&requestUriData) == FAILURE)
    {
        // always bail successfully
        PRINTDEBUG("Graphdat :: failed getting value for REQUEST_URI skipping\n");
        return SUCCESS;
    }
    requestUri = Z_STRVAL_PP(requestUriData);
    requestUriLen = Z_STRLEN_PP(requestUriData);
        
    if(zend_hash_find(serverVars, "REQUEST_METHOD", sizeof("REQUEST_METHOD"), (void **)&requestMethodData) == FAILURE)
    {
        // always bail successfully
        PRINTDEBUG("Graphdat :: failed getting value for REQUEST_METHOD skipping\n");
        return SUCCESS;
    }
    requestMethod = Z_STRVAL_PP(requestMethodData);
    requestMethodLen = Z_STRLEN_PP(requestMethodData);
    
    requestLineItemLen = 1 + requestMethodLen + requestUriLen;
    requestLineItem = emalloc(requestLineItemLen);
    sprintf(requestLineItem, "%s %s", requestMethod, requestUri);
    
    PRINTDEBUG("Request %s took %fms\n", requestLineItem, totalTime);
    
    msgpack_sbuffer* buffer = msgpack_sbuffer_new();
    msgpack_sbuffer_init(buffer);
    msgpack_packer* pk = msgpack_packer_new(buffer, msgpack_sbuffer_write);
    // create map with 8 items (type, route, responsetime, timestamp, source, cputime, pid, context)
    msgpack_pack_map(pk, 8);
    // maps are key then object
    // type == Sample
    msgpack_pack_raw(pk, sizeof("type") - 1);
    msgpack_pack_raw_body(pk, "type", sizeof("type") - 1);
    msgpack_pack_raw(pk, sizeof("Sample") - 1);
    msgpack_pack_raw_body(pk, "Sample", sizeof("Sample") - 1);
    // route
    msgpack_pack_raw(pk, sizeof("route") - 1);
    msgpack_pack_raw_body(pk, "route", sizeof("route") - 1);
    msgpack_pack_raw(pk, requestLineItemLen);
    msgpack_pack_raw_body(pk, requestLineItem, requestLineItemLen);
    // response time
    msgpack_pack_raw(pk, sizeof("responsetime") - 1);
    msgpack_pack_raw_body(pk, "responsetime", sizeof("responsetime") - 1);
    msgpack_pack_double(pk, totalTime);
    // timestamp
    msgpack_pack_raw(pk, sizeof("timestamp") - 1);
    msgpack_pack_raw_body(pk, "timestamp", sizeof("timestamp") - 1);
    msgpack_pack_double(pk, timeNow.tv_sec * 1000);
    // source == HTTP
    msgpack_pack_raw(pk, sizeof("source") - 1);
    msgpack_pack_raw_body(pk, "source", sizeof("source") - 1);
    msgpack_pack_raw(pk, sizeof("HTTP") - 1);
    msgpack_pack_raw_body(pk, "HTTP", sizeof("HTTP") - 1);
    // cputime
    msgpack_pack_raw(pk, sizeof("cputime") - 1);
    msgpack_pack_raw_body(pk, "cputime", sizeof("cputime") - 1);
    msgpack_pack_double(pk, 0.0);
    //pid
    msgpack_pack_raw(pk, sizeof("pid") - 1);
    msgpack_pack_raw_body(pk, "pid", sizeof("pid") - 1);
    msgpack_pack_double(pk, getpid());
    // context is an array of maps
    msgpack_pack_raw(pk, sizeof("context") - 1);
    msgpack_pack_raw_body(pk, "context", sizeof("context") - 1);
    // right now there is only 1 context item the root level of "/"
    msgpack_pack_array(pk, 1);
    // context has 5 items (firsttimestampoffset, responsetime, callcount, cputime, name)
    msgpack_pack_map(pk, 5);
    // firsttimestampoffset
    msgpack_pack_raw(pk, sizeof("firsttimestampoffset") - 1);
    msgpack_pack_raw_body(pk, "firsttimestampoffset", sizeof("firsttimestampoffset") - 1);
    msgpack_pack_double(pk, 0.0);
    // responsetime
    msgpack_pack_raw(pk, sizeof("responsetime") - 1);
    msgpack_pack_raw_body(pk, "responsetime", sizeof("responsetime") - 1);
    msgpack_pack_double(pk, totalTime);
    // callcount
    msgpack_pack_raw(pk, sizeof("callcount") - 1);
    msgpack_pack_raw_body(pk, "callcount", sizeof("callcount") - 1);
    msgpack_pack_int(pk, 1);
    // cputime
    msgpack_pack_raw(pk, sizeof("cputime") - 1);
    msgpack_pack_raw_body(pk, "cputime", sizeof("cputime") - 1);
    msgpack_pack_double(pk, 0.0);
    // name
    msgpack_pack_raw(pk, sizeof("name") - 1);
    msgpack_pack_raw_body(pk, "name", sizeof("name") - 1);
    msgpack_pack_raw(pk, sizeof("/") - 1);
    msgpack_pack_raw_body(pk, "/", sizeof("/") - 1);
    
    unsigned char len[4];
    len[0] = buffer->size >> 24;
    len[1] = buffer->size >> 16;
    len[2] = buffer->size >> 8;
    len[3] = buffer->size;
    
    socketWrite(GRAPHDAT_GLOBALS(socketFD), &len, 4, GRAPHDAT_GLOBALS(debug));
    socketWrite(GRAPHDAT_GLOBALS(socketFD), buffer->data, buffer->size, GRAPHDAT_GLOBALS(debug));
    
    if(GRAPHDAT_GLOBALS(debug))
    {
        size_t b64len = 1 + BASE64_LENGTH (buffer->size);
        char *b64str = emalloc(b64len);
        base64_encode(buffer->data, buffer->size, b64str, b64len);
        PRINTDEBUG("sending %d bytes: %s\n", (int) buffer->size, b64str);
        efree(b64str);
    }
    
    msgpack_sbuffer_free(buffer);
    msgpack_packer_free(pk);

    efree(requestLineItem);

    
/*
Need to send the following message to the agent with this info
 {
     "type": "Sample",
     "source": "HTTP",
     "route": "GET /",
     "responsetime": 49.414,
     "timestamp": 1353535694666.753,
     "cputime": 49.635,
     "pid": "90904",
     "context": [{
         "firsttimestampoffset": 0.09912109375,
         "responsetime": 49.414,
         "callcount": 1,
         "cputime": 49.635,
         "name": "/"
     }, {
         "firsttimestampoffset": 3.8701171875,
         "responsetime": 45.502,
         "callcount": 1,
         "cputime": 45.623,
         "name": "/render"
     }]
 }
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
