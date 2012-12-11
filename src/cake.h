#include "php.h"
#include "zend.h"
#include <stddef.h>

#ifndef graphdat_sdk_php_cake_h
#define graphdat_sdk_php_cake_h

int hasCake(TSRMLS_D);
char* getCakePath(size_t *slen TSRMLS_DC);

#endif