# Dependency Modernization TODO

Status: draft roadmap for contributors.

## Goal

Modernize external C/C++ dependencies in a controlled way, while preserving
runtime compatibility and avoiding regressions in `libdjvulibre`.

Core rule: **tests first, dependency updates second**.

## Scope

- Windows build chain under `windows/djvulibre/*`
- Linux/autotools build path (`autogen.sh`, `configure.ac`, `Makefile.am`)
- CI verification (Linux + Windows)

## Priority Order

1. Stabilize/expand tests (ongoing)
2. Update `zlib`
3. Update `jpeg` (prefer `libjpeg-turbo`)
4. Update `tiff` (highest risk, do last)

## Library Targets and Variants

### zlib

- Current legacy source: `zlib-1.2.13` snapshot in Windows flow.
- Target variant: upstream `zlib` latest stable (major 1.x).
- Notes: usually low-risk API compatibility.

### JPEG

- Current legacy source: IJG `jpeg-6b`.
- Preferred target variant: `libjpeg-turbo` (modern, actively maintained,
  libjpeg-compatible API).
- Fallback variant: newer IJG libjpeg (only if turbo path is blocked).

### TIFF

- Current legacy source: `tiff-4.0.10`.
- Target variant: modern `libtiff` 4.x stable.
- Risk notes:
  - many deprecation warnings (`uint16/uint32` style types),
  - possible API/behavior differences in tools path,
  - should be done only after test coverage is improved.

## Test/Quality Gate Before Any Update

- `make check` on Linux must pass (including optional gtest mode).
- Windows x64 Release build must pass.
- For dependency PRs: no increase in warning count in touched areas unless
  explicitly justified.

## Work Items

1. Expand unit coverage in `tests/` for:
   - string/url helpers,
   - stream/buffer operations,
   - edge cases around integer sizes (`size_t` vs `int`).
2. Add small regression fixtures (minimal DjVu samples) for parser smoke tests.
3. Add/update CI steps to publish test logs/artifacts on failure.
4. Introduce `zlib` update PR (separate from other deps).
5. Introduce `jpeg` update PR (prefer `libjpeg-turbo`) with compatibility notes.
6. Introduce `tiff` update PR with warning cleanup plan.

## Contribution Rules

- One dependency update per PR.
- Keep PRs small and reviewable.
- Include:
  - what changed,
  - why chosen variant/version,
  - test evidence (Linux + Windows),
  - known risks and rollback plan.

## Open Questions

- Keep vendored third-party archives in-repo vs fetch during CI?
- Pin exact versions in-tree vs follow package manager versions?
- Add dedicated warning-baseline report for dependency migration PRs?

