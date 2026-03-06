# Windows non-linking targets for gtest

Last updated: 2026-03-06

These classes/packages currently fail at link stage in the Windows gtest binary (`libdjvu_gtest.exe`) due to unresolved externals (DLL export/symbol visibility mismatch). Keep them in the Linux backlog first.

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

- Source files are present in `windows/djvulibre/libdjvulibre/libdjvulibre.vcxproj`, but symbols used by tests are not link-resolved from `libdjvulibre.dll` in current Windows setup.
- These tests are excluded only from the Windows MSVC gtest project. They remain part of the Linux/autotools test set.
- Practical workflow: implement/expand tests for these packages under Linux build first, then return to Windows after export/link cleanup.
