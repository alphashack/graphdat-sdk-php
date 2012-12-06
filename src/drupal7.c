#include "magento.h"
#include "ext/standard/php_var.h"
#include "zend.h"

// This bit of code is to detect the presence of Drupal 7 
// and extract the abstract path - it should work with
// older versions of drupal, but I haven't tested yet
//
// The plan is to run 
//   $menu_item = menu_get_item($_GET['q']);
//   var_dump($menu_item['page_callback']); 
//

int hasDrupal7(TSRMLS_D)
{
	if(zend_hash_exists(EG(function_table), "menu_get_item", sizeof("menu_get_item")) 
		&& zend_hash_exists(EG(function_table), "drupal_bootstrap", sizeof("drupal_bootstrap")) )
	{
		return 1;
	}
	return 0;
}

char* getDrupal7Path(size_t *slen TSRMLS_DC)
{
	char* result;
	zval retval;
	if(zend_eval_string("$graphdat_menu_item = menu_get_item($_GET['q']);", NULL, "graphdat drupal7 p1" TSRMLS_CC) == FAILURE)
	{
		return NULL;
	}
	if(zend_eval_string("$graphdat_menu_item['page_callback'];", &retval, "graphdat drupal7 p2" TSRMLS_CC) == FAILURE)
	{
		return NULL;
	}
	result = Z_STRVAL_P(&retval);
    *slen = Z_STRLEN_P(&retval);
	return result;
}
