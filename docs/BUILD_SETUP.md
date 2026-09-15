# Build setup

## Required toolchain

The plugin and wire-contract tests use **Visual C++ 2010 (v100), x64**.
KenshiLib compatibility requires that compiler. The VC++ 2010 runtime
redistributable does not install the compiler.

The tracked build scripts expect:

| Component | Required location |
| --- | --- |
| v100 x64 compiler | `C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\amd64\cl.exe` |
| VC10 headers and x64 CRT libraries | `VC\include` and `VC\lib\amd64` under that installation |
| Windows SDK 7.1 | `C:\Program Files\Microsoft SDKs\Windows\v7.1\Include` and `Lib\x64` |
| MSBuild | Visual Studio Installer's `vswhere.exe`; fallback is VS2022 Build Tools |
| KenshiLib dependencies | `third_party/KenshiLib_deps/KenshiLib/Include` and `Libraries` |
| Boost 1.60 | `third_party/KenshiLib_deps/boost_1_60_0/boost` |
| Patched ENet | `third_party/enet/enet/include/enet/enet.h` and sources |

The compiler can come from VS2010 with x64 tools, or Windows SDK 7.1 plus
the VC++ 2010 SP1 compiler update (KB2519277). The scripts explicitly use
SDK 7.1 through `PATH`, `INCLUDE`, `LIB`, and `UseEnv=true`. The project also
declares `WindowsTargetPlatformVersion=10.0`, so modern MSBuild's Windows
SDK discovery must succeed before compilation. Diagnose that separately
from missing v100 tools; do not retarget the plugin to a modern compiler.

KenshiLib's libraries must be actual binaries, not Git LFS pointer files:
`KenshiLib.lib`, `OgreMain_x64.lib`, and `MyGUIEngine_x64.lib`. Dependencies
and build outputs are ignored and must not be committed.

ENet requires the tracked patches under `third_party/enet/patches`, including
C89 compatibility and Steam socket hooks. Inspect the fetched dependency
before applying a patch twice. `third_party/vc10_compat` supplies header shims.

## Build

### Repository-local dependency setup

`build_plugin.cmd` resolves `KENSHILIB_DIR` and `BOOST_INCLUDE_PATH` from its
own repository root and sets them inside `setlocal`, alongside `INCLUDE` and
`LIB`. Builds do not depend on old user-level paths such as `F:\Kenshi`.
It also invokes the [tracked KenshiLib compatibility patch setup](../third_party/kenshilib_patches/README.md),
which checks source hashes and applies only required patches idempotently.
Unsupported or manually modified headers fail clearly before compilation.

For IDE sessions that read user environment variables, the intended values are
`<repo>\third_party\KenshiLib_deps\KenshiLib` and
`<repo>\third_party\KenshiLib_deps\boost_1_60_0` respectively. To update them
from a PowerShell session at this repository root:

```powershell
[Environment]::SetEnvironmentVariable('KENSHILIB_DIR', (Join-Path $PWD 'third_party\KenshiLib_deps\KenshiLib'), 'User')
[Environment]::SetEnvironmentVariable('BOOST_INCLUDE_PATH', (Join-Path $PWD 'third_party\KenshiLib_deps\boost_1_60_0'), 'User')
```

Restart the IDE/shell to inherit those values. They were corrected on this
development machine on September 15, 2026; the command-line build remains
deterministic without them. Dependency directories are still ignored; commit
patches and their manifest, not the fetched library tree.

From the repository root in PowerShell:

```powershell
cmd /c scripts\build_plugin.cmd Harness
cmd /c scripts\build_plugin.cmd Release
cmd /c scripts\build_prototest.cmd
```

The default plugin configuration is `Harness`, which includes scenarios.
`Release` excludes scenarios and produces the player DLL. Outputs are under
`src/plugin/x64/<configuration>/`; prototest is `dist/prototest.exe`.
Check exit codes before deploying. A pre-existing DLL does not prove the
current build succeeded.

## Verify without launching Kenshi

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify.ps1
```

This builds/runs C++ wire/hash/interpolation/save-receiver tests, then runs
harness contracts and synthetic crawl oracle fixtures. `-SkipBuild` requires
an executable from the intended source revision. Script fixtures can also run
individually while the C++ toolchain is unavailable:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\tests\Contract.Tests.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\tests\CrawlMove.Fixture.ps1
```

Passing script fixtures does not validate the DLL or wire layout.

## In-game validation

The live harness requires Kenshi with RE_Kenshi, a newly built Harness DLL,
separate host/join installations, and the scenario's save fixture. Inspect
`scripts/CoopHarness.psm1`, `scripts/run_test.ps1`, and `scripts/scenarios.psd1`
for paths, parameters, and requirements. `scripts/dev_cycle.ps1` combines build,
deployment, and testing; `scripts/regress.ps1` runs a scenario suite. Check the
selected paths: the runners can stop existing game processes and copy saves.

Current runners target two participants. The N-player conversion must extend
the harness to verify every directed pair in three- and four-player runs,
including join-to-join propagation through the host.

## Local baseline

See [N-player baseline and port audit](N_PLAYER_BASELINE.md) for September 15,
2026 results, external blockers, and remaining work. This guide replaces a
stub pointing to an absent, untracked `resources` file. PR #50's guide was
consulted; its machine-specific completion claims and old scenario status
descriptions do not apply to this checkout.
