# KenshiLib 0.3.0 compatibility patches

Run `powershell -NoProfile -ExecutionPolicy Bypass -File scripts\apply_kenshilib_patches.ps1`
from the repository root. `scripts/build_plugin.cmd` also runs this setup step.
Git and Windows PowerShell 5.1 are required. No dependency download, checkout,
reset, compiler retarget, or global Git configuration change is performed.

## Supported source snapshot

The manifest records SHA256 hashes of LF-normalized UTF-8 file content from
KenshiLib_Examples_deps revision `e75769b60dc7e7f0161580b1e86a01912f6404dd`
(KenshiLib 0.3.0). Exact original or exact patched file content is accepted;
the canonical Building.h must match too. This tolerates CRLF checkouts but
rejects unknown snapshots and manual changes, even if HEAD still says 0.3.0.
Git also checks patch context before writing; final content is hash-verified.
Repeated application is a no-op. `-DependencyRoot <path>` supports an alternate
checkout or isolated setup test. Unknown content fails with exit 1; inspect
the mismatch rather than overwriting it or weakening the hashes.

## 0001: canonical BuildingDesignation

Actual v100 baseline first error: C2011 at Building/Building.h:15, prior
definition at Platoon.h:32. Patch only Platoon.h: remove its duplicate enum
and include `Building/Building.h`. The canonical enum, its enumerators and
values are untouched, preserving its ABI.

Independent corroboration: [KenshiLua build instructions](https://github.com/Genpretz/KenshiLua#building-kenshilua)
document the same repair for KenshiLib 0.3.0. Other fixes listed by that project
are not applied here. Add another patch only after this project's compiler
reaches the corresponding failure and the repair is independently justified.

## Local validation after 0001

September 15, 2026: apply and second-run no-op verified. Rebuilt using
`cmd /c scripts\build_plugin.cmd` with v100 x64 (16.00.40219.01).
The BuildingDesignation error is gone. The new first error is:

```text
VC\include\deque(795): error C2027: use of undefined type 'CraftingItem'
KenshiLib\Include\kenshi\Building\CraftingBuilding.h(10): forward declaration
KenshiLib\Include\kenshi\Building\CraftingBuilding.h(92): std::deque<CraftingItem> member
```

VC10 deque's `_DEQUESIZ` enum evaluates `sizeof(value_type)` at instantiation.
The fetched header set contains only a forward declaration of CraftingItem,
so including CraftingBuilding.h instantiates deque without a complete element
type. No missing CraftingItem definition was found in that header set. This
does not justify inventing a layout, replacing the member with padding, or
changing the compiler. No second compatibility patch is included. Full build
diagnostics are in ignored `build/baseline-enum-patch.log`.

Run isolated setup integration tests with:
`powershell -NoProfile -ExecutionPolicy Bypass -File scripts\tests\KenshiLibPatches.Tests.ps1`.
These check original application, repeat no-op, canonical enum preservation,
and refusal to overwrite unexpected source/canonical-header edits. Test copies
and expected-rejection logs remain under ignored `build/`.

The DLL baseline remains incomplete; baseline tests must be run after a
successful DLL build before N-player networking integration resumes.
