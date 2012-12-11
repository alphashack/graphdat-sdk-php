#include "php.h"
#include "zend.h"
#include <stddef.h>

#ifndef graphdat_sdk_php_wordpress_h
#define graphdat_sdk_php_wordpress_h

int hasWordpress(TSRMLS_D);
char* getWordpressPath(size_t *slen TSRMLS_DC);

#endif