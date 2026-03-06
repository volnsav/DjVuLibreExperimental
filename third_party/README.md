Third-party dependencies live under this directory.

Repository policy:

- Linux builds use system packages where practical.
- Windows builds use `vcpkg` rooted at `third_party/vcpkg`.
- Optional vendored source trees should also stay under `third_party/`.

The default Windows helper script automatically bootstraps and uses
`third_party/vcpkg`.
