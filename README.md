# libnetworking
This project is part of the work on a status monitor for the EDSAC machine. Network communication between the measurement nodes and the mothership.

cJSON is used directly in this project (contrib/cJSON.c, include/contrib/cJSON.h). cJSON can be found at https://github.com/DaveGamble/cJSON

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
