#include "joomla.h"
#include "ext/standard/php_var.h"
#include "zend.h"

// This bit of code is to detect the presence of Wordpress
// and extract the abstract path

int hasWordpress(TSRMLS_D)
{
    if(zend_hash_exists(EG(class_table), "wp", sizeof("wp")) 
        && zend_hash_exists(EG(class_table), "wp_query", sizeof("wp_query")) )
    {
        return 1;
    }
    return 0;
}

char* getWordpressPath(size_t *slen TSRMLS_DC)
{
    char* result;
    zval retval;
    if(zend_eval_string("implode('::', array_keys($wp->query_vars));", &retval, "graphdat wordpress" TSRMLS_CC) == FAILURE)
    {
        return NULL;
    }
    result = Z_STRVAL_P(&retval);
    *slen = Z_STRLEN_P(&retval);
    return result;
}
