# Fixture Generation Notes

Possible issues:

1. Missing toolchain
- Generation requires `documenttodjvu.exe`, `djvutoxml.exe`, `djvuparsexml.exe`, `djvubundle.exe`, `djvujoin.exe`, and `djvudump.exe`.

2. Local path differences
- `DEE5_BIN` and `DJVULIBRE_BIN` may need local overrides.

3. Output reset
- The generator recreates `tests/fixtures/reference` and removes existing contents.

4. External source files
- `real_*.djvu` fixtures come from the original DjVuLibre repository and are not generated locally.

5. Reproducibility
- Output hashes may differ across tool versions or environments.
