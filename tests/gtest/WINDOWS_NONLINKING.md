# Windows non-linking targets for gtest

Last updated: 2026-03-06

These classes/packages currently fail at link stage in the Windows CMake gtest
binary (`libdjvu_gtest.exe`) due to unresolved externals and incomplete DLL
export coverage. They stay enabled on Linux and are temporarily excluded from
the Windows target list in `tests/CMakeLists.txt`.

## Non-linking on Windows (current)

1. `DjVuErrorList` (`libdjvu/DjVuErrorList.*`)
2. `DjVuNavDir` (`libdjvu/DjVuNavDir.*`)
3. `DjVuFileCache` (`libdjvu/DjVuFileCache.*`)
4. `UnicodeByteStream` / `XMLByteStream` (`libdjvu/UnicodeByteStream.*`)
5. `DjVuToPS` (`libdjvu/DjVuToPS.*`, including `DjVuToPS::Options`)
6. `DjVuDebug` (`libdjvu/debug.*`)
7. `DjVmDir0` (`libdjvu/DjVmDir0.*`)
8. `DjVuInterface`-dependent test paths in `DjVuImage`
9. `DjVuMessage` C API lookup helpers
10. `GIFFManager` / `GIFFChunk` (`libdjvu/GIFFManager.*`)
11. `GMapAreas` (`libdjvu/GMapAreas.*`)
12. low-level `GThread` API (`libdjvu/GThreads.*`)
13. `MMXControl` (`libdjvu/MMX.*`)
14. `ZPCodec` (`libdjvu/ZPCodec.*`)

## Notes

- The library now builds through CMake; there is no separate MSBuild project to
  maintain for these exclusions.
- These tests are excluded only from the Windows gtest target. They remain part
  of the Linux and WSL CMake test set.
- Practical workflow: extend tests under Linux first, then return to Windows
  after export/link cleanup.
