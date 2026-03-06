cmake_minimum_required(VERSION 3.21)

project(DjVuLibre VERSION 3.5.29 LANGUAGES C CXX)

include(CheckCXXSourceCompiles)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckLibraryExists)
include(CheckSymbolExists)
include(CMakePackageConfigHelpers)
include(CTest)
include(GNUInstallDirs)

option(DJVULIBRE_ENABLE_XMLTOOLS "Build DjVu XML command-line tools" ON)
option(DJVULIBRE_ENABLE_DESKTOPFILES "Install desktop integration assets" ON)
option(DJVULIBRE_ENABLE_GTEST "Build GoogleTest-based unit tests" OFF)
option(DJVULIBRE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

# On Windows, default to bundled (ExternalProject) builds of zlib,
# libjpeg-turbo, and libtiff.  On Linux/macOS, default to system packages.
if(WIN32)
  option(DJVULIBRE_USE_BUNDLED_DEPS
         "Build vendored third-party dependencies from source" ON)
else()
  option(DJVULIBRE_USE_BUNDLED_DEPS
         "Build vendored third-party dependencies from source" OFF)
endif()

set(DJVULIBRE_THIRD_PARTY_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party"
    CACHE PATH
    "Root directory for optional third-party dependencies")

set(BUILD_SHARED_LIBS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(cmake/DjVuThirdParty.cmake)
djvulibre_configure_third_party()

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if(MSVC)
  add_compile_options(/W4 /permissive-)
  if(DJVULIBRE_WARNINGS_AS_ERRORS)
    add_compile_options(/WX)
  endif()
else()
  add_compile_options(-Wall -Wextra)
  if(DJVULIBRE_WARNINGS_AS_ERRORS)
    add_compile_options(-Werror)
  endif()
endif()

set(DJVULIBRE_LT_COMPAT 21)
math(EXPR DJVULIBRE_LT_CUR "${PROJECT_VERSION_PATCH}")
set(DJVULIBRE_LT_REV 0)
math(EXPR DJVULIBRE_LT_AGE "${PROJECT_VERSION_PATCH} - ${DJVULIBRE_LT_COMPAT}")
set(DJVULIBRE_SOVERSION 21)
set(DJVULIBRE_LIBRARY_VERSION
    "${DJVULIBRE_SOVERSION}.${DJVULIBRE_LT_REV}.${DJVULIBRE_LT_AGE}")

find_package(Threads)

set(CMAKE_REQUIRED_LIBRARIES "")
if(CMAKE_THREAD_LIBS_INIT)
  list(APPEND CMAKE_REQUIRED_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}")
endif()

check_library_exists(m sqrt "" HAVE_LIBM)
check_include_file("cpuid.h" HAVE_CPUID_H)
check_include_file("dirent.h" HAVE_DIRENT_H)
check_include_file("getopt.h" HAVE_GETOPT_H)
check_include_file("iconv.h" HAVE_ICONV_H)
check_include_file("inttypes.h" HAVE_INTTYPES_H)
check_include_file("langinfo.h" HAVE_LANGINFO_H)
check_include_file("sched.h" HAVE_SCHED_H)
check_include_file("stdint.h" HAVE_STDINT_H)
check_include_file("sys/ipc.h" HAVE_SYS_IPC_H)
check_include_file("sys/mman.h" HAVE_SYS_MMAN_H)
check_include_file("sys/shm.h" HAVE_SYS_SHM_H)
check_include_file("wchar.h" HAVE_WCHAR_H)
check_include_file("wctype.h" HAVE_WCTYPE_H)
check_include_file_cxx("new.h" HAVE_NEW_H)

check_function_exists(fork HAVE_FORK)
check_function_exists(gethostname HAVE_GETHOSTNAME)
check_function_exists(getpwuid HAVE_GETPWUID)
check_function_exists(mkstemp HAVE_MKSTEMP)
check_function_exists(mmap HAVE_MMAP)
check_function_exists(setenv HAVE_SETENV)
check_function_exists(sigaction HAVE_SIGACTION)
check_function_exists(snprintf HAVE_SNPRINTF)
check_function_exists(strerror HAVE_STRERROR)
check_function_exists(strftime HAVE_STRFTIME)
check_function_exists(vsnprintf HAVE_VSNPRINTF)
check_function_exists(wcrtomb HAVE_WCRTOMB)

check_symbol_exists(fseeko "stdio.h" HAVE_FSEEKO)
check_symbol_exists(iconv "iconv.h" HAVE_ICONV)
check_symbol_exists(iswspace "wctype.h" HAVE_ISWSPACE)
check_symbol_exists(nl_langinfo "langinfo.h" HAVE_NL_LANGINFO)
check_symbol_exists(putc_unlocked "stdio.h" HAVE_PUTC_UNLOCKED)
check_symbol_exists(sched_yield "sched.h" HAVE_SCHED_YIELD)

set(CMAKE_EXTRA_INCLUDE_FILES "wchar.h")
check_symbol_exists(mbstate_t "wchar.h" HAVE_MBSTATE_T)
unset(CMAKE_EXTRA_INCLUDE_FILES)

check_cxx_source_compiles(
  "int main() { bool v = true; return v ? 0 : 1; }"
  HAVE_BOOL)
check_cxx_source_compiles(
  "int main() { try { throw 0; } catch (int) { return 0; } return 1; }"
  HAVE_EXCEPTIONS)
check_cxx_source_compiles(
  "namespace A { namespace B { int v = 0; } } int main() { using namespace A::B; return v; }"
  HAVE_NAMESPACES)
check_cxx_source_compiles(
  "template<class T, int N> class A { public: template<int N2> A<T, N> operator=(const A<T, N2>&) { return A<T, N>(); } }; int main() { A<double, 4> x; A<double, 7> y; x = y; return 0; }"
  HAVE_MEMBER_TEMPLATES)
check_cxx_source_compiles(
  "template<typename T> class X { public: X() = default; }; int main() { X<int> x; (void)x; return 0; }"
  HAVE_TYPENAME)
check_cxx_source_compiles(
  "#include <new>\n#include <iostream>\nint main() { return 0; }"
  HAVE_STDINCLUDES)
check_cxx_source_compiles(
  "static int volatile v; int main() { __sync_lock_test_and_set(&v, 1); __sync_lock_release(&v); __sync_add_and_fetch(&v, 1); __sync_synchronize(); return 0; }"
  HAVE_INTEL_ATOMIC_BUILTINS)
check_cxx_source_compiles(
  "#include <wchar.h>\nint main() { wchar_t c = 0; return static_cast<int>(c); }"
  HAVE_WCHAR_T)
check_cxx_source_compiles(
  "int main() { long long int v = 0; return static_cast<int>(v); }"
  HAVE_LONG_LONG_INT)

if(Threads_FOUND AND NOT WIN32)
  set(HAVE_PTHREAD 1)
else()
  set(HAVE_PTHREAD 0)
endif()

if(TARGET JPEG::JPEG)
  set(HAVE_JPEG 1)
else()
  set(HAVE_JPEG 0)
endif()

if(TARGET TIFF::TIFF)
  set(HAVE_TIFF 1)
else()
  set(HAVE_TIFF 0)
endif()

if(HAVE_ICONV_H AND HAVE_ICONV)
  set(HAS_ICONV 1)
else()
  set(HAS_ICONV 0)
endif()

if(HAVE_WCHAR_T)
  set(HAS_WCHAR 1)
  if(HAVE_WCTYPE_H AND HAVE_ISWSPACE)
    set(HAS_WCTYPE 1)
  else()
    set(HAS_WCTYPE 0)
  endif()
  if(HAVE_MBSTATE_T AND HAVE_WCRTOMB)
    set(HAS_MBSTATE 1)
  else()
    set(HAS_MBSTATE 0)
  endif()
else()
  set(HAS_WCHAR 0)
  set(HAS_WCTYPE 0)
  set(HAS_MBSTATE 0)
endif()

if(HAVE_JPEG)
  set(NEED_JPEG_DECODER 1)
else()
  set(NEED_JPEG_DECODER 0)
endif()

if(HAVE_SYS_MMAN_H AND HAVE_MMAP)
  set(HAS_MEMMAP 1)
else()
  set(HAS_MEMMAP 0)
endif()

if(NOT HAVE_MEMBER_TEMPLATES)
  set(GCONTAINER_NO_MEMBER_TEMPLATES 1)
endif()

if(NOT HAVE_TYPENAME)
  set(GCONTAINER_NO_TYPENAME 1)
endif()

set(HAS_CTRL_C_IN_ERR_MSG 1)
set(AUTOCONF 1)
set(PACKAGE "${PROJECT_NAME}")
set(PACKAGE_NAME "${PROJECT_NAME}")
set(PACKAGE_STRING "${PROJECT_NAME} ${PROJECT_VERSION}")
set(PACKAGE_TARNAME "${PROJECT_NAME}")
set(PACKAGE_VERSION "${PROJECT_VERSION}")
set(PACKAGE_URL "")
set(PACKAGE_BUGREPORT "")
set(VERSION "${PROJECT_VERSION}")

if(NOT WIN32 OR CYGWIN)
  set(UNIX 1)
endif()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/generated/config.h")

add_compile_definitions(HAVE_CONFIG_H)

set(DJVULIBRE_PUBLIC_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/libdjvu/ddjvuapi.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/libdjvu/miniexp.h")

set(DJVULIBRE_CMAKE_CONFIG_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/DjVuLibre")

set(DJVULIBRE_PC_LIBS_PRIVATE "")
if(HAVE_LIBM)
  string(APPEND DJVULIBRE_PC_LIBS_PRIVATE " -lm")
endif()
if(CMAKE_THREAD_LIBS_INIT)
  string(APPEND DJVULIBRE_PC_LIBS_PRIVATE " ${CMAKE_THREAD_LIBS_INIT}")
endif()
if(TARGET JPEG::JPEG)
  string(APPEND DJVULIBRE_PC_LIBS_PRIVATE " -ljpeg")
endif()
if(TARGET TIFF::TIFF)
  string(APPEND DJVULIBRE_PC_LIBS_PRIVATE " -ltiff")
endif()
if(TARGET ZLIB::ZLIB)
  string(APPEND DJVULIBRE_PC_LIBS_PRIVATE " -lz")
endif()
string(STRIP "${DJVULIBRE_PC_LIBS_PRIVATE}" DJVULIBRE_PC_LIBS_PRIVATE)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ddjvuapi.pc.in"
  "${CMAKE_CURRENT_BINARY_DIR}/ddjvuapi.pc"
  @ONLY)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/DjVuLibreConfigVersion.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY SameMajorVersion)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/DjVuLibreConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/DjVuLibreConfig.cmake"
  INSTALL_DESTINATION "${DJVULIBRE_CMAKE_CONFIG_DIR}")

function(djvulibre_set_output_dirs target)
  set_target_properties(
    ${target}
    PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
      LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
      ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

  foreach(config Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${config}" config_upper)
    set_target_properties(
      ${target}
      PROPERTIES
        "RUNTIME_OUTPUT_DIRECTORY_${config_upper}" "${CMAKE_BINARY_DIR}/bin/${config}"
        "LIBRARY_OUTPUT_DIRECTORY_${config_upper}" "${CMAKE_BINARY_DIR}/lib/${config}"
        "ARCHIVE_OUTPUT_DIRECTORY_${config_upper}" "${CMAKE_BINARY_DIR}/lib/${config}")
  endforeach()
endfunction()

add_subdirectory(libdjvu)
add_subdirectory(tools)
add_subdirectory(share)
add_subdirectory(tests)

if(DJVULIBRE_ENABLE_XMLTOOLS)
  add_subdirectory(xmltools)
endif()

if(DJVULIBRE_ENABLE_DESKTOPFILES)
  add_subdirectory(desktopfiles)
endif()

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/ddjvuapi.pc"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

install(FILES
          "${CMAKE_CURRENT_BINARY_DIR}/DjVuLibreConfig.cmake"
          "${CMAKE_CURRENT_BINARY_DIR}/DjVuLibreConfigVersion.cmake"
        DESTINATION "${DJVULIBRE_CMAKE_CONFIG_DIR}")

install(EXPORT DjVuLibreTargets
        NAMESPACE DjVuLibre::
        FILE DjVuLibreTargets.cmake
        DESTINATION "${DJVULIBRE_CMAKE_CONFIG_DIR}")

set(CPACK_PACKAGE_NAME "DjVuLibre")
set(CPACK_PACKAGE_VENDOR "DjVuLibre")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "DjVuLibre library and command-line tools")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "DjVuLibre")
set(CPACK_PACKAGE_CONTACT "https://djvu.sourceforge.net/")
set(CPACK_NSIS_DISPLAY_NAME "DjVuLibre")
set(CPACK_NSIS_PACKAGE_NAME "DjVuLibre")
set(CPACK_NSIS_MODIFY_PATH ON)
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
if(WIN32)
  set(CPACK_GENERATOR "ZIP;NSIS")
else()
  set(CPACK_GENERATOR "TGZ")
endif()
set(CPACK_SOURCE_IGNORE_FILES
    "/build/;/build-.*/;/\\\\.git/;/tmp/;/autom4te\\\\.cache/;/third_party/vcpkg/;/third_party/vcpkg_installed/")

include(CPack)
