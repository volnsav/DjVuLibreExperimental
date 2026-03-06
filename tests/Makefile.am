noinst_PROGRAMS = libdjvu_smoke_test
check_PROGRAMS =
TESTS =

AM_CPPFLAGS = -I$(top_srcdir)/libdjvu -I$(top_srcdir)
AM_CXXFLAGS = $(PTHREAD_CFLAGS)

libdjvu_smoke_test_SOURCES = libdjvu_smoke_test.cpp
libdjvu_smoke_test_LDADD = $(top_builddir)/libdjvu/libdjvulibre.la $(PTHREAD_LIBS)

if WANT_GTEST
check_PROGRAMS = libdjvu_gtest
TESTS = libdjvu_gtest

libdjvu_gtest_CPPFLAGS = $(AM_CPPFLAGS) $(GTEST_CPPFLAGS)
# Auto-expanded at edit time from tests/gtest/test_*.cpp.
libdjvu_gtest_SOURCES = gtest_main.cpp \
  gtest/test_arrays.cpp \
  gtest/test_atomic.cpp \
  gtest/test_bsencodebytestream.cpp \
  gtest/test_bsbytestream.cpp \
  gtest/test_byte_stream.cpp \
  gtest/test_bytestream_extra.cpp \
  gtest/test_codecs_converters.cpp \
  gtest/test_datapool.cpp \
  gtest/test_datapool_triggers.cpp \
  gtest/test_debug.cpp \
  gtest/test_ddjvuapi.cpp \
  gtest/test_djvmdir.cpp \
  gtest/test_djvmdir0.cpp \
  gtest/test_djvmdoc.cpp \
  gtest/test_djvmnav.cpp \
  gtest/test_djvuanno.cpp \
  gtest/test_djvudoceditor.cpp \
  gtest/test_djvudocument.cpp \
  gtest/test_djvudocument_extra.cpp \
  gtest/test_djvudumphelper.cpp \
  gtest/test_djvuerrorlist.cpp \
  gtest/test_djvufile.cpp \
  gtest/test_djvufilecache.cpp \
  gtest/test_reference_fixtures.cpp \
  gtest/test_djvuglobal.cpp \
  gtest/test_djvuglobalmemory.cpp \
  gtest/test_djvuimage.cpp \
  gtest/test_djvuinfo.cpp \
  gtest/test_djvumessage.cpp \
  gtest/test_djvumessagelite.cpp \
  gtest/test_djvunavdir.cpp \
  gtest/test_djvupalette.cpp \
  gtest/test_djvuport.cpp \
  gtest/test_djvusimpleport.cpp \
  gtest/test_djvutext.cpp \
  gtest/test_djvutops_options.cpp \
  gtest/test_djvutops_print.cpp \
  gtest/test_garray_edge_ops.cpp \
  gtest/test_gbitmap.cpp \
  gtest/test_gcontainer.cpp \
  gtest/test_gcontainer_copy.cpp \
  gtest/test_gcontainer_positions.cpp \
  gtest/test_gcontainer_sort_search.cpp \
  gtest/test_gexception.cpp \
  gtest/test_gexception_behavior.cpp \
  gtest/test_gexception_rethrow.cpp \
  gtest/test_giffmanager.cpp \
  gtest/test_glist_iterators.cpp \
  gtest/test_gmapareas.cpp \
  gtest/test_gnative_string.cpp \
  gtest/test_gnative_string_buffer.cpp \
  gtest/test_gos.cpp \
  gtest/test_gos_basename_edge.cpp \
  gtest/test_gos_cwd_error.cpp \
  gtest/test_gpbuffer_extended.cpp \
  gtest/test_gpixmap.cpp \
  gtest/test_grect.cpp \
  gtest/test_grectmapper.cpp \
  gtest/test_grectmapper_clear.cpp \
  gtest/test_grectmapper_inverse.cpp \
  gtest/test_gscaler.cpp \
  gtest/test_gsmartpointer.cpp \
  gtest/test_gstring.cpp \
  gtest/test_gstring_buffer_ops.cpp \
  gtest/test_gstring_mutation.cpp \
  gtest/test_gstring_search_spaces.cpp \
  gtest/test_gstring_static_parse.cpp \
  gtest/test_gstring_unicode.cpp \
  gtest/test_gsync_runtime.cpp \
  gtest/test_gthreads.cpp \
  gtest/test_gthreads_lowlevel.cpp \
  gtest/test_gunicode.cpp \
  gtest/test_gurl.cpp \
  gtest/test_gurl_arguments_order.cpp \
  gtest/test_gurl_djvuopts.cpp \
  gtest/test_gurl_equality_args.cpp \
  gtest/test_gurl_extra.cpp \
  gtest/test_gurl_filesystem.cpp \
  gtest/test_gurl_hash_pathname.cpp \
  gtest/test_gurl_normalization.cpp \
  gtest/test_gurl_relative.cpp \
  gtest/test_iffbytestream.cpp \
  gtest/test_image_render_extended.cpp \
  gtest/test_iw44image.cpp \
  gtest/test_jb2image.cpp \
  gtest/test_miniexp.cpp \
  gtest/test_mmx_control.cpp \
  gtest/test_mmrdecoder.cpp \
  gtest/test_unicode_bytestream.cpp \
  gtest/test_zpcodec.cpp \
  gtest/test_xmlparser.cpp \
  gtest/test_xmltags.cpp
libdjvu_gtest_LDADD = $(top_builddir)/libdjvu/libdjvulibre.la $(GTEST_LIBS)
endif
