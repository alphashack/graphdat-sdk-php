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
  | Author: Sugendran Ganess                                             |
  +----------------------------------------------------------------------+
*/

/*
 * NOTE: a bunch of these variable names are due to what happens in the
 * PHP macros that follow - becareful when renaming things for style
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
#include "timers.h"
#include <string.h>
#include "magento.h"
#include "drupal7.h"

// declare some helpers
char* getRequestPath(size_t *slen TSRMLS_DC);
char* getRequestMethod(size_t *slen, char *fallback TSRMLS_DC);
void onRequestEnd(TSRMLS_D);

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

/* 
 * Every user visible function must have an entry in graphdat_functions[].
 */
const zend_function_entry graphdat_functions[] = {
    PHP_FE(graphdat_begin, NULL)
    PHP_FE(graphdat_end, NULL)
    PHP_FE_END  /* Must be the last line in graphdat_functions[] */
};


/* 
 * this bit lets zend know what's happening
 */
zend_module_entry graphdat_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
  STANDARD_MODULE_HEADER,
#endif
  "graphdat",
  graphdat_functions,
  PHP_MINIT(graphdat),
  PHP_MSHUTDOWN(graphdat),
  PHP_RINIT(graphdat),    /* Replace with NULL if there's nothing to do at request start */
  PHP_RSHUTDOWN(graphdat),  /* Replace with NULL if there's nothing to do at request end */
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

/* 
 * Grab values from the php.ini file
 */
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("graphdat.socketFile", "/tmp/gd.agent.sock", PHP_INI_ALL, OnUpdateString, socketFile, zend_graphdat_globals, graphdat_globals)
    STD_PHP_INI_ENTRY("graphdat.socketPort", "26873", PHP_INI_ALL, OnUpdateLong, socketPort, zend_graphdat_globals, graphdat_globals)
    STD_PHP_INI_ENTRY("graphdat.debug", "false", PHP_INI_ALL, OnUpdateBool, debug, zend_graphdat_globals, graphdat_globals)
PHP_INI_END()

/*
 * Set any global values not set as part of the php.ini
 */
static void php_graphdat_init_globals(zend_graphdat_globals *graphdat_globals TSRMLS_DC)
{
  memset(graphdat_globals, 0, sizeof(zend_graphdat_globals));
  graphdat_globals->socketFD = -1;
}

/* 
 * What to do at module init
 */
PHP_MINIT_FUNCTION(graphdat)
{
    ZEND_INIT_MODULE_GLOBALS(graphdat, php_graphdat_init_globals, NULL);
    REGISTER_INI_ENTRIES();
    return SUCCESS;
}

/* 
 * What to do at module destruct
 */
PHP_MSHUTDOWN_FUNCTION(graphdat)
{
    if(GRAPHDAT_GLOBALS(socketFD) != -1)
    {
        closeSocket(GRAPHDAT_GLOBALS(socketFD));
        GRAPHDAT_GLOBALS(socketFD) = -1;
    }

    UNREGISTER_INI_ENTRIES();
    return SUCCESS;
}

/*
 * what to do at request start
 */
PHP_RINIT_FUNCTION(graphdat)
{
    gettimeofday(&GRAPHDAT_GLOBALS(requestStart), NULL);
    initTimerList(8, &GRAPHDAT_GLOBALS(timers));
    beginTimer(&GRAPHDAT_GLOBALS(timers), "", GRAPHDAT_GLOBALS(requestStart));
    return SUCCESS;
}

/* 
 * What to do at request end
 */
PHP_RSHUTDOWN_FUNCTION(graphdat)
{
    onRequestEnd(TSRMLS_C);
    freeTimerList(&GRAPHDAT_GLOBALS(timers));
    return SUCCESS;
}

/*
 * Called by the phpinfo() func
 */
PHP_MINFO_FUNCTION(graphdat)
{
  php_info_print_table_start();
  php_info_print_table_header(2, "graphdat", "enabled");
  php_info_print_table_end();

  DISPLAY_INI_ENTRIES();
}

PHP_FUNCTION(graphdat_begin)
{
    char *name;
    int name_len;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &name_len) == FAILURE) {
        RETURN_NULL();
    }
    beginTimer(&GRAPHDAT_GLOBALS(timers), name, GRAPHDAT_GLOBALS(requestStart));
    RETURN_TRUE;
}

PHP_FUNCTION(graphdat_end)
{
    char *name;
    int name_len;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &name_len) == FAILURE) {
        RETURN_NULL();
    }
    endTimer(&GRAPHDAT_GLOBALS(timers), name);
    RETURN_TRUE;
}

///// Helpers

char* getRequestPath(size_t *slen TSRMLS_DC)
{
    char * result;
    zval **requestUriData;
    int found = 1;
    if(hasMagento(TSRMLS_C))
    {
        size_t magentoLen;
        result = getMagentoPath(&magentoLen TSRMLS_CC);
        if(result != NULL)
        {
            *slen = magentoLen;
            return result;
        }
    }
    if(hasDrupal7(TSRMLS_C))
    {
        size_t drupal7len;
        result = getDrupal7Path(&drupal7len TSRMLS_CC);
        if(result != NULL)
        {
            *slen = drupal7len;
            return result;
        }
    }
    // looks like we can't do any magic
    zval *zServerVars = PG(http_globals)[TRACK_VARS_SERVER];
    // the server globals should never be null....
    if(zServerVars == NULL)
    {
      return NULL;
    }
    HashTable *serverVars = Z_ARRVAL_P(zServerVars);
    if(zend_hash_find(serverVars, "REQUEST_URI", sizeof("REQUEST_URI"), (void **)&requestUriData) == FAILURE)
    {
        if(zend_hash_find(serverVars, "SCRIPT_NAME", sizeof("SCRIPT_NAME"), (void **)&requestUriData) == FAILURE)
        {
            found = 0;
            result = NULL;
        }
    }
    if(found == 1)
    {
        result = Z_STRVAL_PP(requestUriData);
        *slen = Z_STRLEN_PP(requestUriData);
    }
    return result;
}

char* getRequestMethod(size_t *slen, char *fallback TSRMLS_DC)
{
    char * result;
    zval **requestMethodData;
    HashTable *serverVars = Z_ARRVAL_P(PG(http_globals)[TRACK_VARS_SERVER]);
    if(zend_hash_find(serverVars, "REQUEST_METHOD", sizeof("REQUEST_METHOD"), (void **)&requestMethodData) == FAILURE)
    {
        result = fallback;
        *slen = strlen(fallback);
    }
    else
    {
        result = Z_STRVAL_PP(requestMethodData);
        *slen = Z_STRLEN_PP(requestMethodData);
    }
    return result;
}


void onRequestEnd(TSRMLS_D)
{
    endTimer(&GRAPHDAT_GLOBALS(timers), "");
    if(GRAPHDAT_GLOBALS(socketFD) == -1)
    {
        GRAPHDAT_GLOBALS(socketFD) = openSocket(GRAPHDAT_GLOBALS(socketFile), (int) GRAPHDAT_GLOBALS(socketPort), GRAPHDAT_GLOBALS(debug));
    }
    if(GRAPHDAT_GLOBALS(socketFD) == -1)
    {
        PRINTDEBUG("Graphdat :: not connected to agent, skipping \n");
        return;
    }
    char* requestUri;
    size_t requestUriLen;
    char* requestMethod;
    size_t requestMethodLen;

    char *requestLineItem;
    int requestLineItemLen;
    struct timeval timeNow;

    double totalTime = totalResponseTime(&GRAPHDAT_GLOBALS(timers));
    
    requestUri = getRequestPath(&requestUriLen TSRMLS_CC);
    if(requestUri == NULL)
    {
        // always bail successfully
        PRINTDEBUG("Graphdat :: failed getting value for request path - skipping\n");
        return;
    }
    requestMethod = getRequestMethod(&requestMethodLen, "CLI" TSRMLS_CC);

    requestLineItemLen = requestUriLen + requestMethodLen + 2;
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
    msgpack_pack_raw(pk, requestLineItemLen - 1);
    msgpack_pack_raw_body(pk, requestLineItem, requestLineItemLen - 1);
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
    outputTimersToMsgPack(pk, &GRAPHDAT_GLOBALS(timers));
    
    unsigned char len[4];
    len[0] = buffer->size >> 24;
    len[1] = buffer->size >> 16;
    len[2] = buffer->size >> 8;
    len[3] = buffer->size;
    
    socketWrite(GRAPHDAT_GLOBALS(socketFD), &len, 4, GRAPHDAT_GLOBALS(debug));
    size_t written = socketWrite(GRAPHDAT_GLOBALS(socketFD), buffer->data, buffer->size, GRAPHDAT_GLOBALS(debug));
    if(written != buffer->size)
    {
        PRINTDEBUG("Mismatch: %d bytes written, %d bytes send to be written.\n", (uint) written, (uint) buffer->size);
        closeSocket(GRAPHDAT_GLOBALS(socketFD));
        GRAPHDAT_GLOBALS(socketFD) = -1;
    }

    if(GRAPHDAT_GLOBALS(debug))
    {
        size_t b64len = 1 + BASE64_LENGTH (buffer->size);
        char *b64str = emalloc(b64len);
        base64_encode(buffer->data, buffer->size, b64str, b64len);
        PRINTDEBUG("Sent %d bytes: %s\n", (int) buffer->size, b64str);
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
}