Potential code issues noticed while expanding tests
===============================================

1. `libdjvu/ddjvuapi.cpp`
Path: `ddjvu_page_render()`

- The local variable `dy` is computed as:
  `int dy = rrect.ymin - prect.xmin;`
- This is suspicious because the Y offset mixes `rrect.ymin` with `prect.xmin`.
- It likely should use `prect.ymin`, analogous to how `dx` uses X coordinates.
- Risk:
  Dithering/conversion origin can become wrong for non-zero Y offsets when rendering subrectangles through the public API.

2. `libdjvu/DjVuToPS.cpp`
Path: text-enabled printing path (`options.set_text(true)`)

- On the current reference fixture `mp3_bundled_mixed_layers.djvu`, enabling text in `DjVuToPS` produces valid PostScript, but the output appears identical to the `text=false` case for that page.
- This may be expected for the current implementation, but it is worth re-checking during refactoring because it suggests the hidden-text path has no externally visible effect in this scenario.
- Risk:
  A future refactor could silently keep this behavior even if text embedding is expected by callers.

3. `libdjvu/ddjvuapi.cpp`
Path: `ddjvu_document_search_pageno()`

- On the bundled shared-resource reference fixture, searching for the include file id `shared_page2.djbz` returned page `0` instead of a clear “not found” result.
- I did not lock this into a test expectation because the behavior looks suspicious and does not match the intuitive meaning of “search page number by page-ish identifier”.
- Risk:
  callers may accidentally get a valid page index for non-page resources.

4. `libdjvu/GUnicode.cpp`
Path: `checkmarks()`

- The BOM autodetect branch for leading bytes `0xFE 0xFF` assigns `rep=GStringRep::XUTF16LE`.
- `0xFE 0xFF` is the UTF-16 big-endian BOM, so this looks reversed.
- Risk:
  `GStringRep::XOTHER` autodetect may misdecode valid UTF-16BE input.

5. `libdjvu/GUnicode.cpp`
Path: `checkmarks()`

- After advancing `buf` past a BOM, the remaining size is adjusted with:
  `const size_t s=(size_t)xbuf-(size_t)buf;`
- The subtraction order looks reversed; once `buf > xbuf`, `s` underflows and the function typically zeroes `bufsize`.
- Risk:
  BOM autodetect paths can discard the payload that follows the BOM instead of decoding it.

6. `libdjvu/DjVuMessage.cpp` / `libdjvu/DjVuGlobal.h`
Path: `DjVuMessageLookUpUTF8()` and `DjVuMessageLookUpNative()`

- During test linking, the public declarations from `DjVuGlobal.h` did not match the symbol that was actually emitted by this build.
- I had to call the namespaced symbol explicitly from the test to hit the implementation that exists in the library.
- Risk:
  external callers or alternate build setups may see linker/API inconsistency around these entry points.

7. `libdjvu/DjVuToPS.cpp`
Path: synthetic/background-oriented print combinations

- While probing rarer `DjVuToPS` mode/layer combinations, I reproduced a hard crash on Linux in `BACK`-oriented scenarios.
- Reproducible cases from tests:
  - `level=2`, `mode=BACK` on a synthetic compound image
  - background-only synthetic image printed with `mode=BACK`
- The observed stop is `SIGSEGV` in `GPixmap::destroy(this=0x0)` at `libdjvu/GPixmap.cpp:178`.
- Risk:
  `DjVuToPS` appears to hit an invalid/null `GPixmap` destruction path for some background-only or background-preferred print modes, which is exactly the kind of regression-prone behavior that can get worse during refactoring.
