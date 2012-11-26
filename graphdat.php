<?php

    graphdat_begin("validate module");
    $br = (php_sapi_name() == "cli")? "":"<br>";
    if(!extension_loaded('graphdat')) {
        dl('graphdat.' . PHP_SHLIB_SUFFIX);
    }
    graphdat_begin("print module functions");
    $module = 'graphdat';
    $functions = get_extension_funcs($module);
    echo "Functions available in the test extension:$br\n";
    foreach($functions as $func) {
        echo $func."$br\n";
    }
    echo "$br\n";
    graphdat_end("print module functions");
    graphdat_end("validate module");
    //sleep(0.2);
    graphdat_begin("get info");
    phpinfo(INFO_VARIABLES);
    graphdat_end("get info");    
    //sleep(0.5);

    graphdat_begin("fibonaci");

    // slowness :)
    function isFib($n)
    {
        graphdat_begin("isFib");
        $result = false;
        $first = 0;
        $second = 1;
        for($i=1;$i<=$n;$i++)
        {
            $final = $first + $second;
            $first = $second;
            $second = $final;
            if($second == $n)
            {
                $result = true;
                break;
            }
        }
        graphdat_end("isFib");
        return $result;
    }

    echo "checking if the numbers are in the fib sequence $br\n";
    echo "1000 => ", isFib(1000) ? "yep" : "nope", "$br\n";
    echo "500 => ", isFib(500) ? "yep" : "nope", "$br\n";
    echo "2345 => ", isFib(2345) ? "yep" : "nope", "$br\n";
    echo "377 => ", isFib(377) ? "yep" : "nope", "$br\n";
    echo "610 => ", isFib(610) ? "yep" : "nope", "$br\n";
    echo "10946 => ", isFib(10946) ? "yep" : "nope", "$br\n";

    graphdat_end("fibonaci");
?>
