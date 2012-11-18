graphdat-sdk-php
================

PHP Extension for graphdat

Steps to build
===============
configure your php build with the following two flags

	--enable-maintainer-zts --enable-debug

then run

	phpize
	./configure --enable-graphdat
	make
	sudo make install


once you have done that the graphdat module is installed
to enable it you need to edit your php.ini file and add


References
===========
http://devzone.zend.com/303/extension-writing-part-i-introduction-to-php-and-zend/
http://www.tuxradar.com/practicalphp/20/3/0
http://www.php.net/manual/en/internals2.php