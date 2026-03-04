# Windows non-linking targets for gtest

Last updated: 2026-03-02

These classes/packages currently fail at link stage in the Windows gtest binary (`libdjvu_gtest.exe`) due to unresolved externals (DLL export/symbol visibility mismatch). Keep them in the Linux backlog first.

## Non-linking on Windows (current)

1. `DjVuErrorList` (`libdjvu/DjVuErrorList.*`)
2. `DjVuNavDir` (`libdjvu/DjVuNavDir.*`)
3. `DjVuFileCache` (`libdjvu/DjVuFileCache.*`)
4. `UnicodeByteStream` / `XMLByteStream` (`libdjvu/UnicodeByteStream.*`)
5. `DjVuToPS` (`libdjvu/DjVuToPS.*`, including `DjVuToPS::Options`)

## Notes

- Source files are present in `windows/djvulibre/libdjvulibre/libdjvulibre.vcxproj`, but symbols used by tests are not link-resolved from `libdjvulibre.dll` in current Windows setup.
- Practical workflow: implement/expand tests for these packages under Linux build first, then return to Windows after export/link cleanup.
