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
	if(zend_hash_exists(EG(class_table), "mage", sizeof("mage")) 
		&& zend_hash_exists(EG(class_table), "mage_core_controller_varien_action", sizeof("mage_core_controller_varien_action")) )
	{
		return 1;
	}
	return 0;
}

char* getMagentoPath(size_t *slen TSRMLS_DC)
{
	char* result;
	zval retval;
	//if(zend_eval_string("Mage::app()->getFrontController()->getFullActionName();", &retval, "graphdat magento") == FAILURE)
	if(zend_eval_string("Mage::app()->getRequest()->getRequestedRouteName().'::'.Mage::app()->getRequest()->getRequestedControllerName().'::'.Mage::app()->getRequest()->getRequestedActionName()", &retval, "graphdat magento") == FAILURE)
	{
		return NULL;
	}
	result = Z_STRVAL_P(&retval);
    *slen = Z_STRLEN_P(&retval);
	return result;
}
