#include "php.h"
#include "zend.h"
#include <stddef.h>

#ifndef graphdat_sdk_php_magento_h
#define graphdat_sdk_php_magento_h

int hasMagento(TSRMLS_D);
char* getMagentoPath(size_t *slen TSRMLS_DC);

#endif