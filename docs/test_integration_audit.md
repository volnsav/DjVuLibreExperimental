## Test Integration Audit

Date: 2026-03-06

Scope:
- `tests/gtest`
- current `coverage/current.info`
- focus on integration-test quality, not just line coverage

### Current Strengths

Strong or good-enough integration coverage already exists for these areas:

1. Codec core
- `JB2`, `IW44`, `MMR`, `ZP`, converter-heavy paths
- many boundary and corruption-style unit tests
- strong regression value for codec refactoring

2. Real-fixture document roundtrips
- `DjVuDocument`, `DjVuFile`, `DjVuImage`, `DjVuText`, `XMLParser`
- bundled and indirect reference fixtures
- shared-resource fixture paths
- save/reopen, expand/reopen, XML/text/anno extraction

3. Print coverage
- `DjVuToPS` now has broad option and real-fixture coverage
- single-page, subset, booklet, EPS, `BACK/FORE/BW`, hidden-text paths
- good regression net for print setup and layer-selection behavior

4. Public API baseline
- `ddjvuapi.cpp` has real-fixture coverage for text, anno, render, save, subset, print
- negative non-DjVu inputs are covered

### Main Finding

The suite is now strong for codec and common document workflows, but still has several places where coverage is either:
- mostly synthetic,
- only reachability-oriented,
- or not truly end-to-end with real streamed/document-editing behavior.

These are the main integration gaps.

### Priority 1: Legacy Document Formats

Files:
- `libdjvu/DjVuDocument.cpp`
- `libdjvu/ddjvuapi.cpp`

Weak spots:
- `DIR0` decode path
- `OLD_BUNDLED`
- `OLD_INDEXED`
- old-style `page_to_url`, `id/url -> page`, and fileinfo mapping branches
- legacy doctype mapping in `ddjvuapi`

Why current tests are not enough:
- current real fixtures are modern `SINGLE_PAGE`, `BUNDLED`, and `INDIRECT`
- old-format branches remain mostly uncovered in `gcov`
- synthetic tests reach API surfaces, but do not validate legacy mapping semantics on real files

What is needed:
- valid legacy fixtures:
  - one `OLD_BUNDLED` / `DIR0`
  - one `OLD_INDEXED`
- integration tests through:
  - `DjVuDocument::create_wait`
  - `page_to_url`, `id_to_url`, `url_to_page`, `id_to_page`
  - `ddjvu_document_get_type`
  - `ddjvu_document_get_fileinfo`

### Priority 2: Streaming and Async Public API

File:
- `libdjvu/ddjvuapi.cpp`

Weak spots:
- incremental `ddjvu_stream_write` / `ddjvu_stream_close` behavior on real multi-stream docs
- message queue sequencing (`peek/wait/pop`) under actual decode progress
- thumbnail trigger lifecycle
- cleanup paths that clear pointers from current queued messages
- `fileinfo` and trigger behavior while decode is still in progress

Why current tests are not enough:
- existing tests use stream APIs, but mostly in compact synthetic scenarios
- most reference-fixture tests open from filename, not via chunked streaming
- current checks validate status and reachability, but not realistic event ordering

What is needed:
- stream existing reference fixtures incrementally in small chunks
- assert message order classes, not exact full sequences:
  - document/page info appears before terminal completion
  - thumbnail/page jobs transition out of `NOTSTARTED`
  - `wait/pop` behavior remains consistent while data arrives
- exercise multiple named streams for indirect docs

### Priority 3: Document Editor Persistence

File:
- `libdjvu/DjVuDocEditor.cpp`

Weak spots:
- shared annotation creation/update persistence after reopen
- thumbnail generation and persistence across save/reopen
- `insert_group()` with real imported multi-page sources
- recursive remove/unref behavior on real include graphs
- editing flows that change both save names and structure, then reopen via `DjVuDocument`

Why current tests are not enough:
- editor tests cover lifecycle, rename, save, group/remove, and some reference roundtrips
- many editor-specific branches are still only touched by synthetic or reachability checks
- coverage shows low execution in import/rewrite branches and shared-anno mutation paths

What is needed:
- one reference editing fixture with:
  - shared anno file
  - thumbnails
  - at least one included shared resource
- integration tests for:
  - create shared anno -> save -> reopen -> anno visible through document API
  - modify page titles/names + remove pages + save -> verify mapping stability
  - `insert_group()` from real indirect or bundled document, not only synthetic temp files

### Priority 4: DjVuFile Partial-Availability and Include Graphs

File:
- `libdjvu/DjVuFile.cpp`

Weak spots:
- `wait_for_chunk()`
- included-file decode ordering while data is still arriving
- `process_incl_chunks()` under partial data
- `ALL_DATA_PRESENT` notification path
- decode wait/error propagation through included files

Why current tests are not enough:
- current fixture tests use complete local files
- they validate stable roundtrips, but not live decode behavior with incomplete pools
- uncovered branches are concentrated exactly in wait/partial-availability logic

What is needed:
- stream-backed integration tests using `DataPool`
- existing multipage/shared-resource fixtures fed incrementally
- assertions on:
  - include creation timing
  - decode stop/fail propagation
  - `ALL_DATA_PRESENT` only after all includes are available

### Priority 5: Print Semantics vs Print Markers

File:
- `libdjvu/DjVuToPS.cpp`

Weak spots:
- semantic correctness of generated PS is still weakly checked
- current tests mostly verify markers and structural output

Why current tests are not enough:
- line coverage is now strong
- but many assertions are on emitted markers such as setup strings, text markers, or layer markers
- this catches many regressions, but not subtle layout/render regressions

What is needed:
- golden-output style checks on a small curated set:
  - page count
  - bounding boxes
  - presence/absence of selected layer content
  - stable text-layer snippets
- ideally, render printed output through a PS interpreter in a separate optional test tier

### Priority 6: Cross-Thread Object Handover

Files:
- `libdjvu/DjVuImage.cpp`
- `libdjvu/DjVuFile.cpp`
- `libdjvu/DjVuDocument.cpp`
- `libdjvu/ddjvuapi.cpp`

Weak spots:
- no meaningful integration tests where one decoded object is handed across threads
- no tests for concurrent read-only usage of the same document/page/image object

Why current tests are not enough:
- low-level thread/runtime helpers are covered
- but old-library risk is usually in object ownership, cache visibility, and wait-notify interaction
- this is a practical integration gap, not just a line-coverage gap

What is needed:
- thread handover tests on real reference fixtures:
  - create on thread A, render/query on thread B
  - simultaneous read-only render/text extraction from one `DjVuImage` or page handle
  - repeated open/render/release through `ddjvuapi` with shared context

### Priority 7: Shared Anno and Thumbnails Through Public API

Files:
- `libdjvu/ddjvuapi.cpp`
- `libdjvu/DjVuDocEditor.cpp`
- `libdjvu/DjVuDocument.cpp`

Weak spots:
- public API coverage for shared-anno and thumbnail metadata remains shallow
- editor coverage creates and manipulates some structures, but not enough end-to-end verification after reopen

What is needed:
- reference fixture with explicit shared anno and thumbnails
- tests through both internal API and `ddjvuapi`
- verify after save/reopen:
  - fileinfo reports `S` and `T` entries
  - anno is actually visible through page/document APIs
  - thumbnail jobs resolve and return image data

### Areas That Are Good Enough For Refactoring

These areas already have a solid safety net:

1. Codec internals
2. Common document save/reopen and XML/text/anno roundtrips
3. Standard print and print-option behavior
4. Non-DjVu negative input rejection
5. Core render format conversion paths

For these areas, the remaining gains are mostly incremental.

### Areas That Still Need Better Integration Tests Before Aggressive Refactoring

1. Legacy document compatibility code
2. Async and incremental streaming behavior in `ddjvuapi`
3. Editor persistence for shared anno, thumbnails, and real import graphs
4. `DjVuFile` partial-data include/decode behavior
5. Cross-thread object handover scenarios

### Recommended Next Batches

1. Add valid legacy fixtures and wire them into `DjVuDocument` and `ddjvuapi` integration tests.
2. Add chunked-stream integration tests for bundled and indirect reference fixtures through `ddjvuapi`.
3. Add editor integration tests for shared anno and thumbnails with reopen verification.
4. Add thread-handover tests on real fixtures for `DjVuImage` and `ddjvuapi`.

