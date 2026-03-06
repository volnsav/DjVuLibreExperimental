set(DJVULIBRE_SHARE_FILES
    djvu/osi/languages.xml
    djvu/osi/cs/messages.xml
    djvu/osi/de/libdjvu++.xml
    djvu/osi/de/messages.xml
    djvu/osi/en/djvutools-reference.xml
    djvu/osi/en/libdjvu++.xml
    djvu/osi/en/messages.xml
    djvu/osi/fr/libdjvu++.xml
    djvu/osi/fr/messages.xml
    djvu/osi/ja/libdjvu++.xml
    djvu/osi/ja/messages.xml
    djvu/osi/zh/libdjvu++.xml
    djvu/osi/zh/messages.xml
    djvu/pubtext/DjVuMessages.dtd
    djvu/pubtext/DjVuOCR.dtd
    djvu/pubtext/DjVuXML-s.dtd)

foreach(relative_path IN LISTS DJVULIBRE_SHARE_FILES)
  get_filename_component(relative_dir "${relative_path}" DIRECTORY)
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/${relative_path}"
          DESTINATION "${CMAKE_INSTALL_DATADIR}/${relative_dir}")
endforeach()
