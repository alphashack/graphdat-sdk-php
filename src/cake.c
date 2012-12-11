#include "cake.h"
#include "ext/standard/php_var.h"
#include "zend.h"

// This bit of code is to detect the presence of CakePHP
// and extract the abstract path

int hasCake(TSRMLS_D)
{
    if(zend_hash_exists(EG(class_table), "cakeroute", sizeof("cakeroute")) 
        && zend_hash_exists(EG(class_table), "router", sizeof("router")) )
    {
        return 1;
    }
    return 0;
}

char* getCakePath(size_t *slen TSRMLS_DC)
{
    char* result;
    zval retval;
    if(zend_eval_string("Router::currentRoute()->template;", &retval, "graphdat cake" TSRMLS_CC) == FAILURE)
    {
        return NULL;
    }
    result = Z_STRVAL_P(&retval);
    *slen = Z_STRLEN_P(&retval);
    return result;
}
