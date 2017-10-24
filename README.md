# libnetworking
This project is part of the work on a status monitor for the EDSAC machine. Network communication between the measurement nodes and the mothership.

** This is not suitable for connection to public networks**

cJSON is used directly in this project (contrib/cJSON.c, include/contrib/cJSON.h). cJSON can be found at https://github.com/DaveGamble/cJSON

GLib2.0 >= 2.32 is required as a dependency so that will need to be installed. On Debian this package is called libglib2.0-dev.

Compile using
```
autoreconf -i
./configure
make
```

Run unit tests using
```
make check
```

Clean up using
```
make distclean
```
