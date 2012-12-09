#include "php.h"
#include "zend.h"
#include <stddef.h>

#ifndef graphdat_sdk_php_joomla_h
#define graphdat_sdk_php_joomla_h

int hasJoomla(TSRMLS_D);
char* getJoomlaPath(size_t *slen TSRMLS_DC);

#endif