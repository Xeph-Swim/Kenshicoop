# KenshiLib 0.3.0 compatibility patches

Run `powershell -NoProfile -ExecutionPolicy Bypass -File scripts\apply_kenshilib_patches.ps1`
from the repository root. `scripts/build_plugin.cmd` also runs this setup step.
Git and Windows PowerShell 5.1 are required. No dependency download, checkout,
reset, compiler retarget, or global Git configuration change is performed.

## Supported source snapshot

The manifest records SHA256 hashes of LF-normalized UTF-8 file content from
KenshiLib_Examples_deps revision `e75769b60dc7e7f0161580b1e86a01912f6404dd`
(KenshiLib 0.3.0). Exact original or exact patched file content is accepted;
the guarded canonical definitions must match too. This tolerates CRLF checkouts
but rejects unknown snapshots and manual changes, even if HEAD still says
0.3.0. Git also checks patch context before writing; final content is
hash-verified. Repeated application is a no-op. `-DependencyRoot <path>`
supports an alternate checkout or isolated setup test. Unknown content fails
with exit 1; inspect the mismatch rather than overwriting it or weakening the
hashes.

## 0001: canonical BuildingDesignation

Actual v100 baseline first error: C2011 at Building/Building.h:15, prior
definition at Platoon.h:32. Patch only Platoon.h: remove its duplicate enum
and include `Building/Building.h`. The canonical enum, its enumerators and
values are untouched, preserving its ABI.

Independent corroboration: [KenshiLua build instructions](https://github.com/Genpretz/KenshiLua#building-kenshilua)
document the same repair for KenshiLib 0.3.0. Other fixes listed by that project
are not applied here. Add another patch only after this project's compiler
reaches the corresponding failure and the repair is independently justified.

## 0002: complete CraftingItem

After 0001, the actual next v100 error was:

```text
VC\include\deque(795): error C2027: use of undefined type 'CraftingItem'
KenshiLib\Include\kenshi\Building\CraftingBuilding.h(10): forward declaration
KenshiLib\Include\kenshi\Building\CraftingBuilding.h(92): std::deque<CraftingItem> member
```

VC10 deque's `_DEQUESIZ` enum evaluates `sizeof(value_type)` at instantiation.
The fetched header set contains only a forward declaration of CraftingItem,
so including CraftingBuilding.h instantiates deque without a complete element
type. No complete definition exists anywhere in the 0.3.0 dependency snapshot.

Official KenshiLib commit
[`6f9168d`](https://github.com/BFrizzleFoShizzle/KenshiLib/commit/6f9168d18e82669a1fb15fd3e134ae2eadcaa288)
repairs the omission by adding the complete `CraftingItem` definition directly
to `CraftingBuilding.h`, before `CraftingBuilding` instantiates its deque. Patch
0002 ports that exact upstream class definition and documented offsets. It does
not invent padding, replace the deque, duplicate the type, or change any member
of `CraftingBuilding`. The patched file matches the upstream repaired blob
apart from its final newline.

## Local validation after 0002

September 15, 2026: isolated patch integration tests pass **13/13**. They cover
both patches, post-apply hashes, repeat no-op behavior, definition order, and
refusal to overwrite modified source or guard files.

`cmd /c scripts\build_plugin.cmd` then completes with **exit 0** using the v100
x64 compiler `16.00.40219.01` and links
`src/plugin/x64/Harness/KenshiCoop.dll` (1,401,856 bytes). Compiler warnings
remain, but there are no compiler or linker errors.

The zero-game baseline verification also exits 0:

- C++ protocol/unit layer: **842/842** checks passed.
- Harness contract fixtures: **32/32** checks passed.
- Crawl oracle fixture: **18/18** checks passed.
- Overall: **PASS**.

Run isolated setup integration tests with:
`powershell -NoProfile -ExecutionPolicy Bypass -File scripts\tests\KenshiLibPatches.Tests.ps1`.
Test copies and expected-rejection logs remain under ignored `build/`.
