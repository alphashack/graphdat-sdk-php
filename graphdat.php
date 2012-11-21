<?php
    if(!isset($_SERVER['REQUEST_URI'])) {
        $_SERVER['REQUEST_URI'] = '/foo';
    }
    if(!isset($_SERVER['REQUEST_METHOD'])) {
        $_SERVER['REQUEST_METHOD'] = 'GET';
    }
        
    $br = (php_sapi_name() == "cli")? "":"<br>";

    if(!extension_loaded('graphdat')) {
        dl('graphdat.' . PHP_SHLIB_SUFFIX);
    }
    $module = 'graphdat';
    $functions = get_extension_funcs($module);
    echo "Functions available in the test extension:$br\n";
    foreach($functions as $func) {
        echo $func."$br\n";
    }
    echo "$br\n";

    // phpinfo();
        
    sleep(1);
?>
