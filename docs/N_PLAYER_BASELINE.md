# N-player baseline and initial port audit

## Resumed session: compiler available; dependency header blockers remain

The legacy compiler and SDK 7.1 were installed before this resumed run.
The initial-session findings below are retained as history. The interrupted
preparation commit had not completed; its four files were still staged.
The branch remains `astra/n-player-32`, starting at `24c62bc`.

### Fresh baseline, before any source changes

- `cmd /c scripts\build_plugin.cmd` now finds the v100 x64 compiler. Sandbox
  SDK discovery still fails with access denied; the same command outside the
  sandbox reaches compilation, so the earlier SDK 10 discovery error no
  longer blocks that build.
- Initial DLL compile fails because ENet socket hooks are absent and the
  installed KenshiLib 0.4.0 moved `CombatClass.h` into `kenshi/combat`.
- Applied the existing ENet patches 0001 and 0002 after both `git apply
  --check` commands passed. These changes affect the ignored dependency only.
- Selected KenshiLib dependencies revision
  `e75769b60dc7e7f0161580b1e86a01912f6404dd` (KenshiLib 0.3.0) and downloaded
  its matching LFS library. The dependency had been clean; a failed sandbox
  LFS checkout was completed outside the sandbox. The resulting dependency
  checkout is clean and detached. No dependency files were committed.
- Confirmed `src/` matched `main` before rebuilding. With matching header
  paths and patched ENet, DLL build exits **1**: **8 C2011 diagnostics** for
  duplicate `BuildingDesignation`, and **64 C2027 diagnostics** for undefined
  `CraftingItem`. These are repeated across translation units, not 72
  independent defects. Full log: `build/baseline-plugin.log` (ignored).
- `BuildingDesignation` is defined in both `kenshi/Platoon.h` and
  `kenshi/Building/Building.h`. `CraftingBuilding.h` only forward-declares
  `CraftingItem`, then embeds `std::deque<CraftingItem>`; no definition was
  found in the fetched headers. No replacement engine layout was invented.
- Baseline `cmd /c scripts\build_prototest.cmd`: **exit 0**, v100 x64.
- Baseline `scripts/verify.ps1 -SkipBuild`: **exit 0**, **522/522 C++ checks**,
  **32/32 harness contracts**, **18/18 crawl fixture checks**.

The compiler blocker is resolved. A compatible, complete KenshiLib header set
is still needed for a successful unchanged DLL baseline and live validation.

## Initial session: implementation blocked at the baseline build

Recorded September 15, 2026. The N-player conversion is **not implemented**.
Only build diagnostics and documentation changed in this session. Protocol
55 and gameplay behavior remain unchanged. No multiplayer support or capacity
increase is claimed. This is an initial audit, not a completed repository-wide
review or a complete line-by-line review of PR #50.

## Revisions and authorization

- Clean starting branch: `astra/n-player-32`.
- Starting HEAD: `24c62bcdb9b738cdb3b490d1d78a2a1840c0d57c`
  (development instructions on top of main).
- Current main: `5a761e19a4184b42315e96ea7e92e3c1403d54db`.
- Reference tip: `85d9a59d7c34f9e30270489e0cfa75a331f3ed85`.
- Original multiplayer commit: `fd6586e`; its original parent/base is
  `6d4dc5efc6baf9f907dc46bf05aa9d9858f78a5a`.
- Current main/reference merge base: `7f0ed34117ad5b0ad426e912e9b0411367446287`.
  This differs from the original multiplayer commit's parent: the reference
  includes a later merge (`3f6f53c`). Review both deltas when porting.
- `origin` is `https://github.com/Xeph-Swim/KenshiCoop.git`.
  `upstream` points at nhoral and is read-only by project policy.

`AGENTS.md` was read completely before edits. Git initially rejected the
sandbox user's repository ownership; status succeeded under the user's account.
Subsequent inspection used an explicit per-command safe-directory setting for
this repository, without changing global Git configuration.

## Unmodified baseline results

| Command | Result |
| --- | --- |
| `cmd /c scripts\build_prototest.cmd` | Exit 1: `cl.exe` not recognized; no test executable produced |
| `cmd /c scripts\build_plugin.cmd` in sandbox | MSB4184: SDK discovery access denied; script incorrectly returned 0 |
| Same plugin build outside sandbox | MSB8036: Windows SDK `10.0.26100.0` not found; script incorrectly returned 0 |
| `powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify.ps1 -SkipBuild` | Exit 1 overall: `dist/prototest.exe` absent |
| Harness contract portion | **32/32 checks passed**, 0 failed |
| Crawl oracle fixture portion | **18 checks passed**, 0 failed; counted the fixture's executed `Check` calls; fixture prints an all-checks-passed summary |

The negative synthetic cases deliberately produce FAIL/SKIP oracle verdicts;
the surrounding fixture checks confirm those verdicts and pass. These are not
live-game failures or evidence of multiplayer compatibility.

### Exact external blockers

1. `C:\Program Files (x86)\Microsoft Visual Studio 10.0` does not exist.
   In particular, the required `VC\bin\amd64\cl.exe` is missing.
2. `C:\Program Files\Microsoft SDKs\Windows\v7.1\Include\Windows.h`
   is missing; the expected SDK parent directory does not exist.
3. Outside the sandbox, the unmodified plugin build reports MSB8036 for
   `10.0.26100.0`. That version's `um\Windows.h`, x64 `kernel32.lib`, and
   the Windows Kits SDK manifest do exist. Therefore this is an unresolved
   SDK discovery/completeness problem, not proof that all SDK 10 files are
   absent. Installing v100 alone may not resolve it.

KenshiLib headers and Boost's `version.hpp` exist. The fetched library sizes
are KenshiLib 4,030,178 bytes, MyGUI 1,465,892 bytes, Ogre 4,360,656 bytes;
these are not tiny LFS pointer files. ENet's header exists. Dependency
patch correctness and linking remain unverified until compilation works.
No toolset or SDK target was changed, and no external installer was run.

## Architecture discovered

- `NetLink` owns ENet on a background thread; `Inbound` queues bridge to
  main-thread game mutation. Host outbound state already uses broadcast.
- Wire records are packed C++03 structs, little-endian, with `u32` owner IDs.
  This already offers far more ID space than a 32-player policy limit.
- Epoch tracking and sender clock estimates have owner keys. World items
  use `(ownerId, netId)` keys. Those foundations should be retained.
- Entity drive records are keyed by engine hand; the current `Driven`
  ingestion path does not record its author for isolated teardown.
- Host owns shared world state and each participant owns its squad, with
  newer cell-authority logic governing split-world NPC replication.

## Confirmed two-player assumptions and port hazards

| Area | Evidence / required adaptation |
| --- | --- |
| Admission and relay | `NetLink.cpp` admits another join but logs `3+ players unsupported`; join packets are delivered locally without a relay. Add authenticated peer registry, capacity policy, claim validation, explicit routing. |
| Sender identity | Receive paths trust payload `ownerId`; entity batches call `acceptEpoch(hdr.ownerId, ...)` without binding it to the admitted ENet peer. Validate before enqueueing or relaying. |
| Ownership | `OwnRanks.h` defaults every join to rank 1; supports an explicit set of ranks. A one-rank HELLO must not silently ignore additional configured ranks. |
| Presence | `Plugin.cpp` uses `g_peerPresent`; any leave sets false. Replace with active owner set. UI may derive an aggregate boolean from that set. |
| Disconnect | `processNetEvents` sweeps carries and calls full `clearPeerReplicationState` plus `flushWorldState` after any leave. Scope body/pin/queue/epoch cleanup to the departed owner. |
| Epochs | `NetLink` blanket-clears owner epochs on connection edges. One join's arrival must not reset other senders' gates. Also remove that owner's delayed simulated packets on leave. |
| Interpolation/pins | Record the owner on driven records and peer pins; preserve current lifecycle helpers and body liveness checks during cleanup. |
| Speed | `speedPeerReq_` and `speedPeerCombat_` are scalar. Reduce votes over all active owners and erase a departed vote. |
| Cameras/interest | One `peerCam_`, engine `s_peerCam`, two squad-leader entries, and 12-float anchor buffers. Replace capacity-dependent arrays and audit attention bit masks and all callers. |
| NPC presence | `applyNpcCensus` uses only `got.back()` and a single census owner/set. Current code explicitly says it assumes one peer. Preserve per-row cell authority checks while making history per owner. |
| Symmetric channels | Faction/door/build and newer deed/fixture rows have sequence state that needs sender-scope review. Do not replace newer upstream rows with the older reference's definitions. |
| Items/trades | Preserve current transfer ACKs, conservation checks, pending retries, and `(owner, netId)` item identity. Route intent and verdict according to the actual authority, not a blanket relay. |
| Save/load | Global transfer sender, scalar ACK status, and broadcast connect bootstrap. Need queued/serialized targeted newcomer work and per-transfer expected participant ACK sets. Disconnect must remove only that participant from pending work. |
| Steam | Single `g_peer` and a fabricated address; host must map each registered Steam ID to a distinct ENet address. Lobby/panel/config must handle collections too. |
| Tests | Current runner and log oracles use host/join pairs; extend participant lists and all directed edges, with 2/3/4-player fixtures and failure cases. |

Combat, treatment, health, equipment, dropped items, construction, crafting,
research, doors/locks, beds/cages, prisoners, carried characters, factions,
money, save/load, and reconnect require end-to-end authority and ownership
review. Their presence in an audit search is not evidence that they are
N-player-correct. The complete repository audit remains outstanding.

## PR #50 review and decisions

The reference delta from the shared base has 99 changed files, 4,474 insertions,
and 518 deletions (including binary fixtures). Current main has substantial
newer source changes: 53 source files, 11,510 insertions, 735 deletions relative
to the shared base. Replacing source files wholesale would discard them.

Reviewed the reference commit list, both three-player documents, restored build
guide, wire changes, full NetLink delta, Steam registry delta, save targeting,
and portions of owner cleanup/replication changes. The remaining reference
diff, fixture tooling, discovery, UI, and harness need full review before a
gameplay port. No reference gameplay code was merged or cherry-picked.

| Reference portion | Decision |
| --- | --- |
| Peer maps, reliable roster, targeted send queue entries | Reuse architectural pattern; add validated admission and explicit sender policy. |
| Per-owner votes, camera records, sequence gates, cleanup | Adapt to newer main, including cell/NPC authority and safe adopted/minted proxy handling. |
| Duplicate slot handling | Reject reference behavior: it evicts the incumbent even if live. Reject the newcomer until disconnect/timeout releases the claim. |
| Steam fabricated addresses | Reject reference formula `0x01000001u \| (slot << 24)`: slots 0 and 1 both become `0x01000001`. Use a bijective mapping and test round trips. |
| Fixed limits | Replace reference `MAX_PLAYERS=3`, eight Steam entries, two peer camera slots, and enlarged fixed anchor arrays with runtime policy/dynamic state. |
| Save ACKs | Reimplement: reference `noteAck(xferId, fromOwner, ok)` discards `fromOwner`, despite documentation describing per-peer ACK tracking. |
| Relay allowlist | Re-evaluate. Reference relays combat-hit and money packets and relays listed types after the receive ladder even if payload parsing failed. Current money is host-total plus join-delta. Validate type, size, direction, author, and authority before relay. |
| Wire version/tags | Do not reuse v49 or tag 44: main is v55 and tag 44 is `PKT_CELL_CLAIM`. Choose an intentional new version and unoccupied roster tag. |
| 3-player fixture, all-six-edge oracle, per-link WAN proxy | Useful reference designs; generalize participant loops and add four-player coverage. Reference's reported live passes were not reproduced here. |
| Discovery, friend kit, menu changes | Defer independent UX features; not a prerequisite for the core port. |
| Restored build documentation | Consulted and adapted into a current local guide, excluding stale machine/scenario claims. |

## Changes and validation this session

- `scripts/build_plugin.cmd`: explicit v100/SDK 7.1 prerequisite errors;
  preserve MSBuild exit status across `endlocal`.
- `scripts/build_prototest.cmd`: same compiler/SDK guards, preventing fallback
  to an unrelated modern compiler on PATH.
- `docs/BUILD_SETUP.md`: replace broken resources link with applicable setup
  and validation instructions.
- `docs/N_PLAYER_BASELINE.md`: this baseline, findings, and remaining work.

After changes both build entry points exit **1**, explicitly naming missing
v100, as expected. `git diff --check` passes (only line-ending conversion
warnings). The successful-compile and downstream MSBuild-failure paths cannot
be exercised on this machine until prerequisites are restored.

## Remaining validation and next step

**Next:** install/repair the v100 x64 + SDK 7.1 build environment, resolve
MSBuild SDK detection, and rerun the baseline from these recorded revisions.
Then finish the complete main/reference audit and implement incremental,
buildable changes on `astra/n-player-32`:

1. Pure admission/claim/packet-policy units, configurable `maxPlayers=32`,
   dynamic registries and intentional protocol bump.
2. Owner-aware routing, roster, ownership validation and isolated cleanup;
   preserve current cell authority and newer upstream fixes.
3. Multi-peer Steam mapping and coordinated targeted save/load scheduling.
4. Generalize fixtures/oracles and run protocol/unit tests, 2-player regression,
   3-player six-direction propagation, and 4-player twelve-direction coverage.
5. Exercise normal leave, timeout/crash, reconnect, duplicate slot, full
   session, malformed protocol, newcomer during play, and multi-peer save/load.

All live 2/3/4-player tests, C++ tests, Steam sessions, WAN behavior, failure
scenarios, and multi-peer saves remain **untested** in this session. No 32-client
run was performed. Configured maximum 32 is a future policy default, not a
statement of tested capacity or performance.
