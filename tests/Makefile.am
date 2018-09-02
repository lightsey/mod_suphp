check_LTLIBRARIES = libgtest.la libgmock.la

libgtest_la_SOURCES = googletest-library.cpp
libgtest_la_CPPFLAGS = -I$(top_srcdir)/googletest/googletest/include -I$(top_srcdir)/googletest/googletest -I$(top_srcdir)/googletest/googlemock/include
libgtest_la_LDFLAGS = -pthread

libgmock_la_SOURCES = googlemock-library.cpp
libgmock_la_CPPFLAGS = -I$(top_srcdir)/googletest/googletest/include -I$(top_srcdir)/googletest/googlemock/include -I$(top_srcdir)/googletest/googlemock
libgmock_la_LDFLAGS = -pthread


check_PROGRAMS = test

test_SOURCES = test.cpp PathMatcher_test.cpp
test_LDADD = libgtest.la libgmock.la ../src/libsuphp.la
test_LDFLAGS = -pthread
test_CPPFLAGS = -I$(top_srcdir)/googletest/googletest/include -I$(top_srcdir)/googletest/googletest -I$(top_srcdir)/googletest/googlemock/include -I$(top_srcdir)/googletest/googlemock

TESTS = test
