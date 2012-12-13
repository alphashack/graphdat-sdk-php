#include "zendplugin.h"
#include "ext/standard/php_var.h"
#include "zend.h"

// This bit of code is to detect the presence of Zend Framework
// and extract the abstract path

int hasZend(TSRMLS_D)
{
    if(zend_hash_exists(EG(class_table), "zend_controller_front", sizeof("zend_controller_front")))
    {
        return 1;
    }
    return 0;
}

char* getZendPath(size_t *slen TSRMLS_DC)
{
    char* result;
    zval retval;
    if(zend_eval_string("Zend_Controller_Front::getInstance()->getRequest()->getModuleName().'::'.Zend_Controller_Front::getInstance()->getRequest()->getControllerName().'::'.Zend_Controller_Front::getInstance()->getRequest()->getActionName()", &retval, "graphdat zend" TSRMLS_CC) == FAILURE)
    {
        return NULL;
    }
    result = Z_STRVAL_P(&retval);
    *slen = Z_STRLEN_P(&retval);
    return result;
}
