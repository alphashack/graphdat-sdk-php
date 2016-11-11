#include "codeigniter.h"
#include "ext/standard/php_var.h"
#include "zend.h"

// This bit of code is to detect the presence of Codeigniter
// and extract the abstract path

int hasCodeigniter(TSRMLS_D)
{
    if(zend_hash_exists(EG(class_table), "ci_router", sizeof("ci_router")) 
        && zend_hash_exists(EG(class_table), "ci_config", sizeof("ci_config")) )
    {
        return 1;
    }
    return 0;
}

char* getCodeigniterPath(size_t *slen TSRMLS_DC)
{
    char* result;
    zval retval;
    if(zend_eval_string("$this->router->fetch_class() . '::' . $this->router->fetch_method();", &retval, "graphdat codeigniter" TSRMLS_CC) == FAILURE)
    {
        return NULL;
    }
    result = Z_STRVAL_P(&retval);
    *slen = Z_STRLEN_P(&retval);
    return result;
}
