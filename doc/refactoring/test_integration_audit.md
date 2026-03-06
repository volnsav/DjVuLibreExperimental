# Test Integration Audit Copy

Summary:

1. Covered well
- Codec paths, common document workflows, and print flows have solid regression coverage.

2. Main gaps
- Legacy formats are still weak: `DIR0`, `OLD_BUNDLED`, `OLD_INDEXED`, and old page/url/id mapping paths.
- Streaming and async API coverage is still limited for real incremental decode flows.
- Some branches are covered only by synthetic tests, not by real fixtures.

3. Main risk
- Refactoring can break low-traffic compatibility paths without immediate test failures.

4. Practical need
- Add real legacy fixtures and more end-to-end API tests around streaming, mapping, and fileinfo behavior.
