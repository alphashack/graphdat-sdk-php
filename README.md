graphdat-sdk-php
================

PHP Extension for graphdat. To find out more about graphdat visit http://dashboard.graphdat.com/landing

NOTE: For now only linux and OSX installations of the extension are supported. Watch this space for the windows version.

Steps to build the extension
============================

* Downloaded and extract the latest tarball (https://github.com/alphashack/graphdat-sdk-php/archive/master.zip)
* Run `phpize` - this will create the configure script
* Run `./configure --enable-graphdat`
* Run `make`
* Run `sudo make install`

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
