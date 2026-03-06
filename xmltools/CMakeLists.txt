set(DATADIR "${CMAKE_INSTALL_FULL_DATADIR}")

function(djvulibre_add_xmltool name)
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
  djvulibre_set_output_dirs(${name})
  install(TARGETS ${name} RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endfunction()

djvulibre_add_xmltool(djvutoxml djvutoxml.cpp)
djvulibre_add_xmltool(djvuxmlparser djvuxmlparser.cpp)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/djvuxml.1.in"
               "${CMAKE_CURRENT_BINARY_DIR}/djvuxml.1"
               @ONLY)

install(FILES
          djvutoxml.1
          djvuxmlparser.1
          "${CMAKE_CURRENT_BINARY_DIR}/djvuxml.1"
        DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
