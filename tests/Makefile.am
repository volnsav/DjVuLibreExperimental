check_PROGRAMS = libdjvu_smoke_test
TESTS = $(check_PROGRAMS)

AM_CPPFLAGS = -I$(top_srcdir)/libdjvu -I$(top_srcdir)
AM_CXXFLAGS = $(PTHREAD_CFLAGS)

libdjvu_smoke_test_SOURCES = libdjvu_smoke_test.cpp
libdjvu_smoke_test_LDADD = $(top_builddir)/libdjvu/libdjvulibre.la $(PTHREAD_LIBS)

if WANT_GTEST
check_PROGRAMS += libdjvu_gtest
TESTS += libdjvu_gtest

libdjvu_gtest_CPPFLAGS = $(AM_CPPFLAGS) $(GTEST_CPPFLAGS)
libdjvu_gtest_SOURCES = libdjvu_gtest.cpp
libdjvu_gtest_LDADD = $(top_builddir)/libdjvu/libdjvulibre.la $(GTEST_LIBS)
endif
