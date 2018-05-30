EXTRA_DIST = CMakeLists.txt

AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/test -I$(top_srcdir)

bin_PROGRAMS =	basic_test

basic_test_SOURCES = basic_test.cpp PJ_geos_test.cc pj_phi2_test.cc geodesic_test.cc
basic_test_LDADD = ../../src/libproj.la ../../test/libtestmain.la

basic_test-check: basic_test
	./basic_test

check-local: basic_test-check
