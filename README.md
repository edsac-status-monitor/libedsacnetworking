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

## You should know before contributing
Unit tests are done in a weird-ish way. Inside each .c file there should be unit tests wrapped in #ifdef TEST. These unit tests should include a main() function.

The contents of this repository are free software: distributed under MIT licencing.
