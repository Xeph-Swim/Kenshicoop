# N-player conversion progress

## Implemented foundation

`src/plugin/core/SessionRegistry.h` is a C++03, transport-independent admission
registry. It uses owner-to-squad and squad-to-owner maps. Capacity counts the
host, is configurable before joins are admitted, and defaults to 32. Owner
identifiers use the existing 32-bit type, reserve host 0 and `OWNER_ID_ALL`,
and are not recycled after disconnect. Exhaustion fails rather than wrapping.

Host squad 0 is only the backward-compatible initial default: configuration
can reserve a different set of host squads. Each owner can claim multiple
squads. Overlapping or invalid claims fail without evicting an incumbent or
partially reserving the remaining squads. Removal releases only that owner's
claims; transport integration must invoke it only after a verified leave or
timeout. A newcomer claiming a live player's squad cannot take over the slot.

This is an **isolated foundation**, not an enabled N-player session feature.
It is currently exercised by prototest; NetLink and Config do not yet use it.
No gameplay, packet layout, protocol version, or runtime capacity changed.
The current plugin remains the two-player implementation.

## Validation

Compiled using `scripts/build_prototest.cmd`, Visual C++ 2010 x64, exit 0.
`scripts/verify.ps1 -SkipBuild`, exit 0:

| Layer | Result |
| --- | --- |
| Baseline C++ checks before source changes | 522/522 pass |
| C++ checks including admission foundation | 842/842 pass |
| Harness contracts | 32/32 pass |
| Crawl oracle fixture | 18/18 pass |

The 320 additional checks exercise 2-, 3-, and 4-participant registries, a
synthetic capacity-32 registry, full-session rejection, duplicate claim
rejection, removal and reconnect, preservation of other claims, multi-squad
ownership, invalid claims/configuration, and 70 consecutive reconnects with
fresh IDs. These are data-model tests, **not** ENet sessions or live Kenshi
clients. The ID exhaustion branch is inspected but not executed by these tests.
Logs from the final verification are in ignored `build/n-player-units.log`.

## Baseline build status and remaining work

See [baseline and initial audit](N_PLAYER_BASELINE.md) for the earlier failures
and the completed baseline. Two tracked, hash-verified
[compatibility patches](../third_party/kenshilib_patches/README.md) resolve the
KenshiLib 0.3.0 `BuildingDesignation` duplication and its omitted complete
`CraftingItem` definition. The latter is an exact port of the definition added
by official KenshiLib commit `6f9168d`. Patch integration tests pass **13/13**.

`cmd /c scripts\build_plugin.cmd` now exits 0 and links the Harness x64 DLL with
the required v100 compiler. A fresh `scripts/verify.ps1` run also exits 0 with
**842/842 C++ checks**, **32/32 harness contracts**, and **18/18 crawl fixture
checks**. No compiler retarget or fabricated engine layout was used.

The following remain unfinished:

1. Complete the remaining current-main and original-base PR #50 diff audit.
2. Bind admitted owners to actual ENet connections, validate packet authors
   and squad/entity ownership, and add runtime `maxPlayers` configuration.
3. Add a new protocol version and roster/claims encoding that preserves
   newer main packet tags (PR #50's tag 44 conflicts with current cell claims).
4. Implement explicit authoritative propagation/relay, per-owner replication,
   NPC/cell/camera state, and surgical disconnect cleanup throughout queues
   and engine-facing state.
5. Integrate dynamic Steam peer mapping, target newcomer saves, and track
   coordinated save/load work and ACKs by participant.
6. Extend the live harness to arbitrary participant lists. Validate 2/3/4
   clients, all directed propagation, timeout/crash/reconnect, session full,
   duplicate claims, malformed packets, newcomer joins, and multi-peer saves.

PR #50 supplied architectural ideas, but no gameplay source was copied into
this foundation. Its incumbent eviction, fixed caps, colliding Steam address
formula, and owner-ignoring save ACKs must not be ported unchanged. Its reported
test results are not evidence for this branch. No live 2/3/4/32-client run,
Steam session, WAN test, or multi-peer save/load was performed here.
