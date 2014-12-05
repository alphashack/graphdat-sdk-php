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
#include "ext/standard/base64.h"
#include "timers.h"
#include <string.h>
#include "magento.h"
#include "drupal7.h"
#include "joomla.h"
//#include "wordpress.h"
#include "cake.h"
#include "zendplugin.h"

// declare some helpers
static char* getRequestPath(size_t *slen TSRMLS_DC);
static char* getRequestMethod(size_t *slen, char *fallback TSRMLS_DC);
static void onRequestEnd(TSRMLS_D);

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
    STD_PHP_INI_ENTRY("graphdat.socketFile", "/tmp/gd.agent.sock", PHP_INI_PERDIR|PHP_INI_SYSTEM, OnUpdateString, socketFile, zend_graphdat_globals, graphdat_globals)
    STD_PHP_INI_ENTRY("graphdat.socketPort", "26873", PHP_INI_PERDIR|PHP_INI_SYSTEM, OnUpdateLong, socketPort, zend_graphdat_globals, graphdat_globals)
    STD_PHP_INI_ENTRY("graphdat.debug", "false", PHP_INI_PERDIR|PHP_INI_SYSTEM, OnUpdateBool, debug, zend_graphdat_globals, graphdat_globals)
    STD_PHP_INI_ENTRY("graphdat.enable_joomla", "false", PHP_INI_PERDIR|PHP_INI_SYSTEM, OnUpdateBool, enable_joomla, zend_graphdat_globals, graphdat_globals)
    STD_PHP_INI_ENTRY("graphdat.enable_drupal", "false", PHP_INI_PERDIR|PHP_INI_SYSTEM, OnUpdateBool, enable_drupal, zend_graphdat_globals, graphdat_globals)
    STD_PHP_INI_ENTRY("graphdat.enable_magento", "false", PHP_INI_PERDIR|PHP_INI_SYSTEM, OnUpdateBool, enable_magento, zend_graphdat_globals, graphdat_globals)
    STD_PHP_INI_ENTRY("graphdat.enable_cakephp", "false", PHP_INI_PERDIR|PHP_INI_SYSTEM, OnUpdateBool, enable_cakephp, zend_graphdat_globals, graphdat_globals)
PHP_INI_END()

/*
 * Set any global values not set as part of the php.ini
 */
static void php_graphdat_init_globals(zend_graphdat_globals *graphdat_globals TSRMLS_DC)
{
  memset(graphdat_globals, 0, sizeof(zend_graphdat_globals));
  graphdat_globals->socketFD = -1;
}

void setPlugins(TSRMLS_D)
{
    // work out what plugins are needed
    if(!GRAPHDAT_GLOBALS(enable_joomla) && !GRAPHDAT_GLOBALS(enable_drupal)
        && !GRAPHDAT_GLOBALS(enable_magento) && !GRAPHDAT_GLOBALS(enable_cakephp)
        && !GRAPHDAT_GLOBALS(enable_zend))
    {
      // if none are enabled then we enable them all
      GRAPHDAT_GLOBALS(enable_joomla) = 1;
      GRAPHDAT_GLOBALS(enable_drupal) = 1;
      GRAPHDAT_GLOBALS(enable_magento) = 1;
      GRAPHDAT_GLOBALS(enable_cakephp) = 1;
      GRAPHDAT_GLOBALS(enable_zend) = 1;
      GRAPHDAT_GLOBALS(all_plugins_enabled) = 1;
    }

    GRAPHDAT_GLOBALS(plugins).count = GRAPHDAT_GLOBALS(enable_joomla) + GRAPHDAT_GLOBALS(enable_drupal)
                                      + GRAPHDAT_GLOBALS(enable_magento) + GRAPHDAT_GLOBALS(enable_cakephp)
                                      + GRAPHDAT_GLOBALS(enable_zend);
    GRAPHDAT_GLOBALS(plugins).array = malloc(sizeof(struct graphdat_plugin) * GRAPHDAT_GLOBALS(plugins).count);
    int index = 0;
    if(GRAPHDAT_GLOBALS(enable_joomla))
    {
      struct graphdat_plugin *plugin = &GRAPHDAT_GLOBALS(plugins).array[index++];
      plugin->isAvailable = hasJoomla;
      plugin->getPath = getJoomlaPath;
    }
    if(GRAPHDAT_GLOBALS(enable_drupal))
    {
      struct graphdat_plugin *plugin = &GRAPHDAT_GLOBALS(plugins).array[index++];
      plugin->isAvailable = hasDrupal7;
      plugin->getPath = getDrupal7Path;
    }
    if(GRAPHDAT_GLOBALS(enable_magento))
    {
      struct graphdat_plugin *plugin = &GRAPHDAT_GLOBALS(plugins).array[index++];
      plugin->isAvailable = hasMagento;
      plugin->getPath = getMagentoPath;
    }
    if(GRAPHDAT_GLOBALS(enable_cakephp))
    {
      struct graphdat_plugin *plugin = &GRAPHDAT_GLOBALS(plugins).array[index++];
      plugin->isAvailable = hasCake;
      plugin->getPath = getCakePath;
    }
    if(GRAPHDAT_GLOBALS(enable_zend))
    {
      struct graphdat_plugin *plugin = &GRAPHDAT_GLOBALS(plugins).array[index++];
      plugin->isAvailable = hasZend;
      plugin->getPath = getZendPath;
    }
}

/*
 * What to do at module init
 */
PHP_MINIT_FUNCTION(graphdat)
{
    ZEND_INIT_MODULE_GLOBALS(graphdat, php_graphdat_init_globals, NULL);
    REGISTER_INI_ENTRIES();

    setPlugins(TSRMLS_C);

    return SUCCESS;
}

/*
 * What to do at module destruct
 */
PHP_MSHUTDOWN_FUNCTION(graphdat)
{
    if(GRAPHDAT_GLOBALS(socketFD) != -1)
    {
        closeSocket(GRAPHDAT_GLOBALS(socketFD), GRAPHDAT_GLOBALS(debug));
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
    freeTimerList(&GRAPHDAT_GLOBALS(timers));
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

  if(GRAPHDAT_GLOBALS(all_plugins_enabled))
  {
    php_info_print_table_start();
    php_info_print_table_header(1, "All plugins are enabled because none where chosen");
    php_info_print_table_end();
  }
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

static char* getRequestPath(size_t *slen TSRMLS_DC)
{
    char * result = NULL;
    size_t pluginLen;
    zval **requestUriData;
    int count = GRAPHDAT_GLOBALS(plugins).count;
    int i;
    for(i=0; i < count && result == NULL; i++)
    {
      struct graphdat_plugin *plugin = &GRAPHDAT_GLOBALS(plugins).array[i];
      if(plugin->isAvailable(TSRMLS_C))
      {
        result = plugin->getPath(&pluginLen TSRMLS_CC);
      }
    }
    if(result != NULL)
    {
        *slen = pluginLen;
        return result;
    }
    // looks like we can't do any magic
    int found = 1;
    if (PG(auto_globals_jit)) {
      zend_is_auto_global("_SERVER", sizeof("_SERVER")-1 TSRMLS_CC);
    }
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

static char* getRequestMethod(size_t *slen, char *fallback TSRMLS_DC)
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


static void onRequestEnd(TSRMLS_D)
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
    size_t written;
    written = socketWrite(GRAPHDAT_GLOBALS(socketFD), &len, 4, GRAPHDAT_GLOBALS(debug));
    if(written != 4)
    {
        // close and reopen in case there is a broken pipe
        closeSocket(GRAPHDAT_GLOBALS(socketFD), GRAPHDAT_GLOBALS(debug));
        GRAPHDAT_GLOBALS(socketFD) = -1;
        PRINTDEBUG("Retrying the write");
        GRAPHDAT_GLOBALS(socketFD) = openSocket(GRAPHDAT_GLOBALS(socketFile), (int) GRAPHDAT_GLOBALS(socketPort), GRAPHDAT_GLOBALS(debug));
        written = socketWrite(GRAPHDAT_GLOBALS(socketFD), &len, 4, GRAPHDAT_GLOBALS(debug));
    }
    if(written == 4)
    {
        written = socketWrite(GRAPHDAT_GLOBALS(socketFD), buffer->data, buffer->size, GRAPHDAT_GLOBALS(debug));
        if(written != buffer->size)
        {
          PRINTDEBUG("Mismatch: %d bytes written, %d bytes send to be written.\n", (uint) written, (uint) buffer->size);
        }
        else if(GRAPHDAT_GLOBALS(debug))
        {
          char *b64str = php_base64_encode(buffer->data, buffer->size, NULL);
          PRINTDEBUG("Sent %d bytes: %s\n", (int) buffer->size, b64str);
          efree(b64str);
        }
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
