set(DJVULIBRE_JB2CMP_SOURCES
    jb2cmp/classify.cpp
    jb2cmp/cuts.cpp
    jb2cmp/frames.cpp
    jb2cmp/patterns.cpp)

set(DJVULIBRE_JB2CMP_HEADERS
    jb2cmp/classify.h
    jb2cmp/mdjvucfg.h
    jb2cmp/minidjvu.h
    jb2cmp/patterns.h)

add_library(djvulibre_jb2cmp OBJECT
            ${DJVULIBRE_JB2CMP_SOURCES}
            ${DJVULIBRE_JB2CMP_HEADERS})
target_include_directories(
  djvulibre_jb2cmp
  PRIVATE
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/libdjvu"
    "${CMAKE_SOURCE_DIR}/tools"
    "${CMAKE_BINARY_DIR}/generated")

if(TIFF_FOUND)
  target_link_libraries(djvulibre_jb2cmp PRIVATE TIFF::TIFF)
endif()

function(djvulibre_add_tool name)
  add_executable(${name} ${ARGN})
  target_include_directories(
    ${name}
    PRIVATE
      "${CMAKE_SOURCE_DIR}"
      "${CMAKE_SOURCE_DIR}/libdjvu"
      "${CMAKE_SOURCE_DIR}/tools"
      "${CMAKE_BINARY_DIR}/generated")
  target_link_libraries(${name} PRIVATE djvulibre)
  if(Threads_FOUND)
    target_link_libraries(${name} PRIVATE Threads::Threads)
  endif()
  if(TIFF_FOUND AND (${name} STREQUAL "cjb2" OR ${name} STREQUAL "ddjvu"))
    target_link_libraries(${name} PRIVATE TIFF::TIFF)
  endif()
  djvulibre_set_output_dirs(${name})
  install(TARGETS ${name} RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endfunction()

djvulibre_add_tool(bzz bzz.cpp common.h)
djvulibre_add_tool(c44 c44.cpp common.h)
djvulibre_add_tool(cjb2
                   cjb2.cpp
                   jb2tune.cpp
                   jb2tune.h
                   common.h
                   $<TARGET_OBJECTS:djvulibre_jb2cmp>)
djvulibre_add_tool(cpaldjvu
                   cpaldjvu.cpp
                   jb2tune.cpp
                   jb2tune.h
                   common.h
                   $<TARGET_OBJECTS:djvulibre_jb2cmp>)
djvulibre_add_tool(csepdjvu
                   csepdjvu.cpp
                   jb2tune.cpp
                   jb2tune.h
                   common.h
                   $<TARGET_OBJECTS:djvulibre_jb2cmp>)
djvulibre_add_tool(ddjvu ddjvu.cpp tiff2pdf.c tiff2pdf.h)
djvulibre_add_tool(djvm djvm.cpp common.h)
djvulibre_add_tool(djvmcvt djvmcvt.cpp common.h)
djvulibre_add_tool(djvudump djvudump.cpp common.h)
djvulibre_add_tool(djvuextract djvuextract.cpp common.h)
djvulibre_add_tool(djvumake djvumake.cpp common.h)
djvulibre_add_tool(djvups djvups.cpp)
djvulibre_add_tool(djvused djvused.cpp common.h)
djvulibre_add_tool(djvuserve djvuserve.cpp common.h)
djvulibre_add_tool(djvutxt djvutxt.cpp)

install(PROGRAMS any2djvu djvudigital DESTINATION "${CMAKE_INSTALL_BINDIR}")

install(FILES
          any2djvu.1
          bzz.1
          c44.1
          cjb2.1
          cpaldjvu.1
          csepdjvu.1
          ddjvu.1
          djvm.1
          djvmcvt.1
          djvu.1
          djvudigital.1
          djvudump.1
          djvuextract.1
          djvumake.1
          djvups.1
          djvused.1
          djvuserve.1
          djvutxt.1
        DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
