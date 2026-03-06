# --------------------------------------------------------------------------
# DjVuThirdParty.cmake — resolve zlib, libjpeg-turbo, libtiff
#
# When DJVULIBRE_USE_BUNDLED_DEPS is ON (default on Windows), source
# archives are downloaded into third_party/, optional patches are applied,
# and each library is built as a static library via ExternalProject.
#
# When DJVULIBRE_USE_BUNDLED_DEPS is OFF (default on Linux/macOS), the
# standard find_package() mechanism locates system-installed libraries.
# --------------------------------------------------------------------------

include(ExternalProject)

# Directories -----------------------------------------------------------
set(DJVU_THIRD_PARTY_ROOT     "${CMAKE_SOURCE_DIR}/third_party")
set(DJVU_PATCHES_DIR          "${CMAKE_SOURCE_DIR}/patches")
set(DJVU_THIRD_PARTY_BUILD    "${CMAKE_BINARY_DIR}/third_party_build")
set(DJVU_THIRD_PARTY_INSTALL  "${CMAKE_BINARY_DIR}/third_party_install")
set(DJVU_THIRD_PARTY_DL       "${CMAKE_BINARY_DIR}/_deps")

# Platform-aware static library filenames --------------------------------
if(MSVC)
  set(_DJVU_ZLIB_LIB  "zlib.lib")
  set(_DJVU_JPEG_LIB  "jpeg-static.lib")
  set(_DJVU_TIFF_LIB  "tiff.lib")
else()
  set(_DJVU_ZLIB_LIB  "libz.a")
  set(_DJVU_JPEG_LIB  "libjpeg.a")
  set(_DJVU_TIFF_LIB  "libtiff.a")
endif()

# =======================================================================
# Helper: apply git-format patches
# =======================================================================
function(_djvu_apply_patches name source_dir patch_dir)
  if(NOT EXISTS "${patch_dir}")
    return()
  endif()

  file(GLOB _patch_files "${patch_dir}/*.patch")
  list(SORT _patch_files)
  if(NOT _patch_files)
    return()
  endif()

  set(_stamp "${source_dir}/.djvu_patched.stamp")
  if(EXISTS "${_stamp}")
    message(STATUS "${name}: patches already applied")
    return()
  endif()

  find_package(Git QUIET)
  if(NOT GIT_FOUND)
    message(WARNING "${name}: git not found — skipping patches")
    return()
  endif()

  foreach(_pf IN LISTS _patch_files)
    message(STATUS "${name}: applying ${_pf}")
    set(_applied FALSE)

    foreach(_p 0 1 2 3)
      execute_process(
        COMMAND "${GIT_EXECUTABLE}" apply -p${_p}
                --check --ignore-space-change --ignore-whitespace "${_pf}"
        WORKING_DIRECTORY "${source_dir}"
        RESULT_VARIABLE _rc OUTPUT_QUIET ERROR_QUIET)

      if(_rc EQUAL 0)
        execute_process(
          COMMAND "${GIT_EXECUTABLE}" apply -p${_p}
                  --ignore-space-change --ignore-whitespace "${_pf}"
          WORKING_DIRECTORY "${source_dir}"
          RESULT_VARIABLE _rc2 OUTPUT_VARIABLE _out ERROR_VARIABLE _err)

        if(NOT _rc2 EQUAL 0)
          message(FATAL_ERROR
            "${name}: failed to apply ${_pf} (-p${_p})\n${_err}")
        endif()
        set(_applied TRUE)
        break()
      endif()
    endforeach()

    if(NOT _applied)
      message(FATAL_ERROR
        "${name}: no valid strip-level for ${_pf}")
    endif()
  endforeach()

  file(WRITE "${_stamp}" "patched\n")
endfunction()

# =======================================================================
# Helper: download + extract an archive into third_party/<name>
# =======================================================================
function(_djvu_prepare_archive name url source_subdir)
  set(_dst "${DJVU_THIRD_PARTY_ROOT}/${name}")
  set(_patch_dir "${DJVU_PATCHES_DIR}/${name}")

  if(NOT EXISTS "${_dst}/CMakeLists.txt")
    set(_archive "${DJVU_THIRD_PARTY_DL}/${name}.archive")

    message(STATUS "${name}: downloading ${url}")
    file(DOWNLOAD "${url}" "${_archive}"
         STATUS _st SHOW_PROGRESS TLS_VERIFY ON TIMEOUT 300)
    list(GET _st 0 _code)
    if(NOT _code EQUAL 0)
      list(GET _st 1 _msg)
      message(FATAL_ERROR "${name}: download failed — ${_msg}")
    endif()

    set(_tmp "${DJVU_THIRD_PARTY_DL}/extract_${name}")
    file(REMOVE_RECURSE "${_tmp}")
    file(MAKE_DIRECTORY "${_tmp}")
    message(STATUS "${name}: extracting")
    file(ARCHIVE_EXTRACT INPUT "${_archive}" DESTINATION "${_tmp}")

    if(source_subdir)
      set(_extracted "${_tmp}/${source_subdir}")
    else()
      file(GLOB _children RELATIVE "${_tmp}" "${_tmp}/*")
      list(LENGTH _children _n)
      if(_n EQUAL 1)
        list(GET _children 0 _only)
        set(_extracted "${_tmp}/${_only}")
      else()
        message(FATAL_ERROR "${name}: cannot auto-detect source subdir")
      endif()
    endif()

    if(NOT EXISTS "${_extracted}")
      message(FATAL_ERROR "${name}: extracted dir not found: ${_extracted}")
    endif()

    file(REMOVE_RECURSE "${_dst}")
    file(RENAME "${_extracted}" "${_dst}")
  else()
    message(STATUS "${name}: using existing source at ${_dst}")
  endif()

  _djvu_apply_patches("${name}" "${_dst}" "${_patch_dir}")
  set(${name}_SOURCE_DIR "${_dst}" PARENT_SCOPE)
endfunction()

# =======================================================================
# ExternalProject builders (bundled mode)
# =======================================================================

# ---- zlib -------------------------------------------------------------
function(_djvu_build_zlib source_dir)
  set(_install "${DJVU_THIRD_PARTY_INSTALL}/zlib")
  file(MAKE_DIRECTORY "${_install}/include")

  ExternalProject_Add(zlib_ep
    SOURCE_DIR  "${source_dir}"
    BINARY_DIR  "${DJVU_THIRD_PARTY_BUILD}/zlib"
    INSTALL_DIR "${_install}"
    CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
      -DCMAKE_INSTALL_LIBDIR=lib
      -DCMAKE_POSITION_INDEPENDENT_CODE=ON
      -DCMAKE_BUILD_TYPE=Release
      -DBUILD_SHARED_LIBS=OFF
      -DZLIB_BUILD_EXAMPLES=OFF
      -DCMAKE_DEBUG_POSTFIX=
    BUILD_COMMAND
      ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release
    INSTALL_COMMAND
      ${CMAKE_COMMAND} --install <BINARY_DIR> --config Release
    BUILD_BYPRODUCTS
      "${_install}/lib/${_DJVU_ZLIB_LIB}"
  )

  if(NOT TARGET ZLIB::ZLIB)
    add_library(ZLIB::ZLIB STATIC IMPORTED GLOBAL)
  endif()
  set_target_properties(ZLIB::ZLIB PROPERTIES
    IMPORTED_LOCATION "${_install}/lib/${_DJVU_ZLIB_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES "${_install}/include")
  add_dependencies(ZLIB::ZLIB zlib_ep)

  set(ZLIB_FOUND TRUE PARENT_SCOPE)
  set(_DJVU_ZLIB_INSTALL_DIR "${_install}" PARENT_SCOPE)
endfunction()

# ---- libjpeg-turbo ----------------------------------------------------
function(_djvu_build_jpeg source_dir)
  set(_install "${DJVU_THIRD_PARTY_INSTALL}/libjpeg-turbo")
  file(MAKE_DIRECTORY "${_install}/include")

  ExternalProject_Add(libjpeg_turbo_ep
    SOURCE_DIR  "${source_dir}"
    BINARY_DIR  "${DJVU_THIRD_PARTY_BUILD}/libjpeg-turbo"
    INSTALL_DIR "${_install}"
    CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
      -DCMAKE_INSTALL_LIBDIR=lib
      -DCMAKE_POSITION_INDEPENDENT_CODE=ON
      -DCMAKE_BUILD_TYPE=Release
      -DENABLE_SHARED=OFF
      -DENABLE_STATIC=ON
      -DWITH_TURBOJPEG=OFF
      -DWITH_JAVA=OFF
      -DWITH_DOCS=OFF
      -DWITH_TESTS=OFF
      -DWITH_TOOLS=OFF
      -DCMAKE_DEBUG_POSTFIX=
    BUILD_COMMAND
      ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release
    INSTALL_COMMAND
      ${CMAKE_COMMAND} --install <BINARY_DIR> --config Release
    BUILD_BYPRODUCTS
      "${_install}/lib/${_DJVU_JPEG_LIB}"
  )

  if(NOT TARGET JPEG::JPEG)
    add_library(JPEG::JPEG STATIC IMPORTED GLOBAL)
  endif()
  set_target_properties(JPEG::JPEG PROPERTIES
    IMPORTED_LOCATION "${_install}/lib/${_DJVU_JPEG_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES "${_install}/include")
  add_dependencies(JPEG::JPEG libjpeg_turbo_ep)

  set(JPEG_FOUND TRUE PARENT_SCOPE)
  set(_DJVU_JPEG_INSTALL_DIR "${_install}" PARENT_SCOPE)
endfunction()

# ---- libtiff -----------------------------------------------------------
function(_djvu_build_tiff source_dir)
  set(_install "${DJVU_THIRD_PARTY_INSTALL}/tiff")
  file(MAKE_DIRECTORY "${_install}/include")

  set(_zlib_dir "${_DJVU_ZLIB_INSTALL_DIR}")
  set(_jpeg_dir "${_DJVU_JPEG_INSTALL_DIR}")

  ExternalProject_Add(tiff_ep
    SOURCE_DIR  "${source_dir}"
    BINARY_DIR  "${DJVU_THIRD_PARTY_BUILD}/tiff"
    INSTALL_DIR "${_install}"
    CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
      -DCMAKE_INSTALL_LIBDIR=lib
      -DCMAKE_POSITION_INDEPENDENT_CODE=ON
      -DCMAKE_BUILD_TYPE=Release
      -DBUILD_SHARED_LIBS=OFF
      -Dtiff-tools=OFF
      -Dtiff-tests=OFF
      -Dtiff-contrib=OFF
      -Dtiff-docs=OFF
      -Dtiff-deprecated=OFF
      -Dzlib=ON
      -Djpeg=ON
      -Dold-jpeg=OFF
      -Djbig=OFF
      -Dlerc=OFF
      -Dlzma=OFF
      -Dzstd=OFF
      -Dwebp=OFF
      -Dlibdeflate=OFF
      -Dpixarlog=OFF
      -Dopengl=OFF
      -DZLIB_INCLUDE_DIR=${_zlib_dir}/include
      -DZLIB_LIBRARY=${_zlib_dir}/lib/${_DJVU_ZLIB_LIB}
      -DJPEG_INCLUDE_DIR=${_jpeg_dir}/include
      -DJPEG_LIBRARY=${_jpeg_dir}/lib/${_DJVU_JPEG_LIB}
      -DCMAKE_DEBUG_POSTFIX=
    DEPENDS
      zlib_ep
      libjpeg_turbo_ep
    BUILD_COMMAND
      ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release
    INSTALL_COMMAND
      ${CMAKE_COMMAND} --install <BINARY_DIR> --config Release
    BUILD_BYPRODUCTS
      "${_install}/lib/${_DJVU_TIFF_LIB}"
  )

  if(NOT TARGET TIFF::TIFF)
    add_library(TIFF::TIFF STATIC IMPORTED GLOBAL)
  endif()
  set_target_properties(TIFF::TIFF PROPERTIES
    IMPORTED_LOCATION "${_install}/lib/${_DJVU_TIFF_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES "${_install}/include"
    INTERFACE_LINK_LIBRARIES "JPEG::JPEG;ZLIB::ZLIB")
  add_dependencies(TIFF::TIFF tiff_ep)

  set(TIFF_FOUND TRUE PARENT_SCOPE)
endfunction()

# =======================================================================
# Public entry point
# =======================================================================
function(djvulibre_configure_third_party)
  if(NOT DJVULIBRE_USE_BUNDLED_DEPS)
    # ---- System packages ------------------------------------------------
    find_package(ZLIB REQUIRED)
    find_package(JPEG REQUIRED)
    find_package(TIFF QUIET)
    return()
  endif()

  # ---- Bundled: download / patch / build --------------------------------
  file(MAKE_DIRECTORY "${DJVU_THIRD_PARTY_ROOT}")
  file(MAKE_DIRECTORY "${DJVU_THIRD_PARTY_BUILD}")
  file(MAKE_DIRECTORY "${DJVU_THIRD_PARTY_INSTALL}")
  file(MAKE_DIRECTORY "${DJVU_THIRD_PARTY_DL}")

  _djvu_prepare_archive(zlib
    "https://zlib.net/fossils/zlib-1.3.1.tar.gz"
    "zlib-1.3.1")

  _djvu_prepare_archive(libjpeg-turbo
    "https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/3.1.3/libjpeg-turbo-3.1.3.tar.gz"
    "libjpeg-turbo-3.1.3")

  _djvu_prepare_archive(tiff
    "https://download.osgeo.org/libtiff/tiff-4.7.1.tar.gz"
    "tiff-4.7.1")

  _djvu_build_zlib("${zlib_SOURCE_DIR}")
  _djvu_build_jpeg("${libjpeg-turbo_SOURCE_DIR}")
  _djvu_build_tiff("${tiff_SOURCE_DIR}")

  # Propagate _FOUND variables to caller
  set(ZLIB_FOUND TRUE PARENT_SCOPE)
  set(JPEG_FOUND TRUE PARENT_SCOPE)
  set(TIFF_FOUND TRUE PARENT_SCOPE)
endfunction()

