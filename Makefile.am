# make static library target
lib_LTLIBRARIES = libedsacnetworking.la
libedsacnetworking_la_SOURCES = src/representation.c src/contrib/cJSON.c include/representation.h include/contrib/cJSON.h src/server.c include/server.h src/sending.c include/sending.h src/timer.c include/timer.h

# package config file
pkgconfig_DATA = libedsacnetworking.pc

# make subdirectories work
ACLOCAL_AMFLAGS = -I m4 --install
AC_LOCAL_AMFLAGS = -I m4 --install

# CFLAGS
AM_CFLAGS = -Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -Wnested-externs -Winline -Wno-long-long -Wuninitialized -Wconversion -Wstrict-prototypes -Werror -O -g -std=c11 -fstack-protector-strong -I include -I$(top_srcdir)/include $(GLIB_CFLAGS) $(PTHREAD_CFLAGS)

# timer.c needs -lrt (see TIMER_CREATE(2))
RT_LIBS = -lrt

# Unit tests
check_PROGRAMS = representation.test system.test server.test sending.test keep_alive_pass.test keep_alive_fail.test
representation_test_SOURCES = src/test/representation.c
representation_test_LDADD = libedsacnetworking.la $(GLIB_LIBS) $(PTHREAD_LIBS) $(RT_LIBS)
system_test_SOURCES = src/test/system.c
system_test_LDADD = libedsacnetworking.la $(GLIB_LIBS) $(PTHREAD_LIBS) $(RT_LIBS)
server_test_SOURCES = src/test/server.c
server_test_LDADD = libedsacnetworking.la $(GLIB_LIBS) $(PTHREAD_LIBS) $(RT_LIBS)
sending_test_SOURCES = src/test/sending.c
sending_test_LDADD = libedsacnetworking.la $(GLIB_LIBS) $(PTHREAD_LIBS) $(RT_LIBS)
keep_alive_pass_test_SOURCES = src/test/keep_alive_pass.c
keep_alive_pass_test_LDADD = libedsacnetworking.la $(GLIB_LIBS) $(PTHREAD_LIBS) $(RT_LIBS)
keep_alive_fail_test_SOURCES = src/test/keep_alive_fail.c
keep_alive_fail_test_LDADD = libedsacnetworking.la $(GLIB_LIBS) $(PTHREAD_LIBS) $(RT_LIBS)
TESTS = representation.test system.test 

# rule for long-check
include Makefile.long-check
