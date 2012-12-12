#include "php.h"
#include "zend.h"
#include <stddef.h>

#ifndef graphdat_sdk_php_zend_h
#define graphdat_sdk_php_zend_h

int hasZend(TSRMLS_D);
char* getZendPath(size_t *slen TSRMLS_DC);

#endif