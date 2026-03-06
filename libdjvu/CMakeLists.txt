set(DJVULIBRE_LIBRARY_SOURCES
    Arrays.cpp
    BSByteStream.cpp
    BSEncodeByteStream.cpp
    ByteStream.cpp
    DataPool.cpp
    DjVmDir.cpp
    DjVmDir0.cpp
    DjVmDoc.cpp
    DjVmNav.cpp
    DjVuAnno.cpp
    DjVuDocEditor.cpp
    DjVuDocument.cpp
    DjVuDumpHelper.cpp
    DjVuErrorList.cpp
    DjVuFile.cpp
    DjVuFileCache.cpp
    DjVuGlobal.cpp
    DjVuGlobalMemory.cpp
    DjVuImage.cpp
    DjVuInfo.cpp
    DjVuMessage.cpp
    DjVuMessageLite.cpp
    DjVuNavDir.cpp
    DjVuPalette.cpp
    DjVuPort.cpp
    DjVuText.cpp
    DjVuToPS.cpp
    GBitmap.cpp
    GContainer.cpp
    GException.cpp
    GIFFManager.cpp
    GMapAreas.cpp
    GOS.cpp
    GPixmap.cpp
    GRect.cpp
    GScaler.cpp
    GSmartPointer.cpp
    GString.cpp
    GThreads.cpp
    GURL.cpp
    GUnicode.cpp
    IFFByteStream.cpp
    IW44EncodeCodec.cpp
    IW44Image.cpp
    JB2EncodeCodec.cpp
    JB2Image.cpp
    JPEGDecoder.cpp
    MMRDecoder.cpp
    MMX.cpp
    UnicodeByteStream.cpp
    XMLParser.cpp
    XMLTags.cpp
    ZPCodec.cpp
    atomic.cpp
    ddjvuapi.cpp
    debug.cpp
    miniexp.cpp)

set(DJVULIBRE_LIBRARY_HEADERS
    Arrays.h
    BSByteStream.h
    ByteStream.h
    DataPool.h
    DjVmDir.h
    DjVmDir0.h
    DjVmDoc.h
    DjVmNav.h
    DjVuAnno.h
    DjVuDocEditor.h
    DjVuDocument.h
    DjVuDumpHelper.h
    DjVuErrorList.h
    DjVuFile.h
    DjVuFileCache.h
    DjVuGlobal.h
    DjVuImage.h
    DjVuInfo.h
    DjVuMessage.h
    DjVuMessageLite.h
    DjVuNavDir.h
    DjVuPalette.h
    DjVuPort.h
    DjVuText.h
    DjVuToPS.h
    GBitmap.h
    GContainer.h
    GException.h
    GIFFManager.h
    GMapAreas.h
    GOS.h
    GPixmap.h
    GRect.h
    GScaler.h
    GSmartPointer.h
    GString.h
    GThreads.h
    GURL.h
    IFFByteStream.h
    IW44Image.h
    JB2Image.h
    JPEGDecoder.h
    MMRDecoder.h
    MMX.h
    Template.h
    UnicodeByteStream.h
    XMLParser.h
    XMLTags.h
    ZPCodec.h
    atomic.h
    debug.h
    ddjvuapi.h
    miniexp.h)

add_library(djvulibre SHARED
            ${DJVULIBRE_LIBRARY_SOURCES}
            ${DJVULIBRE_LIBRARY_HEADERS})
add_library(DjVuLibre::djvulibre ALIAS djvulibre)

target_include_directories(
  djvulibre
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_BINARY_DIR}/generated")

target_compile_definitions(djvulibre PRIVATE DIR_DATADIR="${CMAKE_INSTALL_FULL_DATADIR}")

if(WIN32)
  target_compile_definitions(
    djvulibre
    PRIVATE
      DJVUAPI_EXPORT
      DDJVUAPI_EXPORT
      MINILISPAPI_EXPORT
      _CRT_SECURE_NO_WARNINGS)
  set_target_properties(djvulibre PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

if(TARGET JPEG::JPEG)
  target_link_libraries(djvulibre PRIVATE JPEG::JPEG)
endif()

if(TARGET ZLIB::ZLIB)
  target_link_libraries(djvulibre PRIVATE ZLIB::ZLIB)
endif()

if(TARGET TIFF::TIFF)
  target_link_libraries(djvulibre PRIVATE TIFF::TIFF)
endif()

if(TARGET Threads::Threads)
  target_link_libraries(djvulibre PRIVATE Threads::Threads)
endif()

if(HAVE_LIBM)
  target_link_libraries(djvulibre PRIVATE m)
endif()

if(APPLE)
  find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)
  if(COREFOUNDATION_FRAMEWORK)
    target_link_libraries(djvulibre PRIVATE "${COREFOUNDATION_FRAMEWORK}")
  endif()
endif()

if(TARGET libjpeg_turbo_ep)
  add_dependencies(djvulibre libjpeg_turbo_ep)
endif()
if(TARGET zlib_ep)
  add_dependencies(djvulibre zlib_ep)
endif()
if(TARGET tiff_ep)
  add_dependencies(djvulibre tiff_ep)
endif()

set_target_properties(
  djvulibre
  PROPERTIES
    OUTPUT_NAME djvulibre
    VERSION "${DJVULIBRE_LIBRARY_VERSION}"
    SOVERSION "${DJVULIBRE_SOVERSION}")

if(WIN32)
  set_target_properties(djvulibre PROPERTIES PREFIX "lib")
endif()

djvulibre_set_output_dirs(djvulibre)

install(TARGETS djvulibre
        EXPORT DjVuLibreTargets
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")

install(FILES ${DJVULIBRE_PUBLIC_HEADERS}
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libdjvu")
        