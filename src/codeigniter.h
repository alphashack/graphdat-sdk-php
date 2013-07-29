#include "php.h"
#include "zend.h"
#include <stddef.h>

#ifndef graphdat_sdk_php_codeigniter_h
#define graphdat_sdk_php_codeigniter_h

int hasCodeigniter(TSRMLS_D);
char* getCodeigniterPath(size_t *slen TSRMLS_DC);

#endif