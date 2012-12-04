#include "magento.h"
#include "ext/standard/php_var.h"
#include "zend.h"

// This bit of code is to detect the presence of Magento
// and extract the abstract path
//
// The class we're interested in will be
// Mage_Core_Controller_Varien_Action
// the current version of which is found at
//  Mage::app()->getFrontController()
// and then we call getFullActionName()
// to get the abstract path

int hasMagento(TSRMLS_D)
{
	if(zend_hash_exists(EG(class_table), "mage", sizeof("mage")))
	{
	    zval **classVal;
	    zend_hash_find(EG(class_table), "mage", sizeof("mage"),  (void **) &classVal);
	    php_var_dump(classVal, 1, TSRMLS_C);
		return 1;
	}
	return 0;
}

char* getMagentoPath(size_t *slen TSRMLS_DC)
{
    zval **classVal;
    char *result;
    zval funcApp, funcFrontController, funcFullAction, retController, retApp, retAction;

    ZVAL_STRING(&funcApp, "app", 0);

    zend_hash_find(EG(class_table), "mage", sizeof("mage"), (void **) &classVal);

	if(call_user_function(EG(function_table), classVal, &funcApp, &retApp, 0, NULL TSRMLS_CC) == FAILURE)
	{
		return NULL;
	}
	// reference to the app
	zval *pRetApp = &retApp;
	ZVAL_STRING(&funcFrontController, "getfrontcontroller", 0);
	if(call_user_function(EG(function_table), &pRetApp, &funcFrontController, &retController, 0, NULL TSRMLS_CC) == FAILURE)
	{
		return NULL;
	}
	// reference to the controller
	zval *pRetController = &retController;
	ZVAL_STRING(&funcFullAction, "getfullactionname", 0);
	if(call_user_function(EG(function_table), &pRetController, &funcFullAction, &retAction, 0, NULL TSRMLS_CC) == FAILURE)
	{
		return NULL;
	}

    result = Z_STRVAL_P(&retAction);
    *slen = Z_STRLEN_P(&retAction);

	return result;
}
