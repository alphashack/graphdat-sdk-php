#include "php.h"
#include "zend.h"
#include <stddef.h>

#ifndef graphdat_sdk_php_drupal7_h
#define graphdat_sdk_php_drupal7_h

int hasDrupal7(TSRMLS_D);
char* getDrupal7Path(size_t *slen TSRMLS_DC);

#endif