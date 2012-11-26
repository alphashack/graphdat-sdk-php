graphdat-sdk-php
================

PHP Extension for graphdat

Steps to build
===============
configure your php build with the following two flags

	--enable-maintainer-zts --enable-debug

the `--enable-debug` flag will make a debug version of php that spits out things like memory leak warnings

then run

	phpize
	./configure --enable-graphdat CFLAGS="-O0 -g3" LDFLAGS="-O0 -g3"
	make
	sudo make install

the `CFLAGS` and `LDFLAGS` set no optimizations and also compiles debugging symbols

once you have done that the graphdat module is installed
to enable it you need to edit your php.ini file and add


References
===========
http://devzone.zend.com/303/extension-writing-part-i-introduction-to-php-and-zend/
http://www.tuxradar.com/practicalphp/20/3/0
http://www.php.net/manual/en/internals2.php
http://php.net/manual/en/internals2.ze1.zendapi.php
http://engineering.yakaz.com/writing-a-cpp-extension-for-php-5.html