check_LTLIBRARIES = libgtest.la

libgtest_la_SOURCES = googletest-library.cc
libgtest_la_CPPFLAGS = -I$(top_srcdir)/googletest/googletest/include -I$(top_srcdir)/googletest/googletest
libgtest_la_LDFLAGS = -pthread

check_PROGRAMS = test

test_SOURCES = test.cpp
test_LDADD = libgtest.la
test_LDFLAGS = -pthread
test_CPPFLAGS = -I$(top_srcdir)/googletest/googletest/include -I$(top_srcdir)/googletest/googletest -pthread

TESTS = test
