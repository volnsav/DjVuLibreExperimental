set(DJVULIBRE_ICON_SIZES 16 20 22 24 32 48 64 72 96 128 256)

foreach(size IN LISTS DJVULIBRE_ICON_SIZES)
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/prebuilt-hi${size}-djvu.png"
          DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/${size}x${size}/mimetypes"
          RENAME "image-vnd.djvu.png")
endforeach()

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/djvu.svg"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/mimetypes"
        RENAME "image-vnd.djvu.svg")
