# Graphdat PHP SDK

*PHP Extension for Graphdat*

--

PHP Extension for graphdat. To find out more about graphdat visit http://dashboard.graphdat.com/landing

NOTE: For now only linux and OSX installations of the extension are supported. Watch this space for the windows version.

Steps to build the extension
============================

* Downloaded and extract the latest tarball (https://github.com/alphashack/graphdat-sdk-php/archive/master.zip)
* Run `phpize` - this will create the configure script
* If you are building on an older 32-bit x86 system, e.g. CentOS or Redhat 5, run `CFLAGS=-march=i686 ./configure --enable-graphdat`
* Otherwise, run `./configure --enable-graphdat`
* Run `make`
* Run `sudo make install`

Steps to use PECL to install the extension
==========================================

* Downloaded and extract the latest tarball (https://github.com/alphashack/graphdat-sdk-php/archive/master.zip)
* Update the date in package2.xml to today
* Run `pecl package`
* If you are building on an older 32-bit x86 system, e.g. CentOS or Redhat 5, run `CFLAGS=-march=i686 pecl install graphdat-1.0.3.tgz`
* Otherwise, run `pecl install graphdat-1.0.3.tgz`

The extension is now installed and will need to be enabled. Adding the following line to your php.ini file

```ini
extension=graphdat.so
```

Restart Apache/nginx/lighthttpd and you should now see request counts and response times being reported on your graphdat dashboard.

Instrumenting your code
=======================

The extension adds two methods to PHP, `graphdat_begin` and `graphdat_end`. These methods take 1 parameter which is the name of the timer. For example:

```php
    graphdat_begin("get info");
    phpinfo(INFO_VARIABLES);
    graphdat_end("get info");
```

By putting timers around parts of your codebase you can monitor in realtime how each part is performing and see trends over time, and during load.
