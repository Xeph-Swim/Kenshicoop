# N-player conversion progress

## Three-player experimental milestone

Protocol 56 provides an N-generic direct-UDP session layer. The first
experimental host plus two-client kit is ready for a narrow physical-PC test of
admission, roster propagation, owned-squad presence, star relay, and isolated
join disconnect. See [the three-PC runbook](THREE_PLAYER_EXPERIMENT.md).

This milestone does not claim that every gameplay channel is safe with three
players. The kit disables coordinated save/load, speed/time synchronization,
and remote camera interest. Steam P2P still has a single-remote peer model. The
runbook lists the other systems to avoid during the first field test.

## Implemented architecture

- `SessionRegistry` stores dynamic `owner -> claims` and `squad -> owner` maps.
  Capacity includes the host, defaults to 32, and is configurable before the
  session starts. Owner IDs are 32-bit, monotonic, and are not recycled.
- HELLO carries one or more explicit squad claims. The host rejects malformed,
  duplicate, already-owned, session-full, protocol-mismatched, and exhausted-ID
  admissions without disturbing an incumbent.
- A dynamic ENet peer registry binds each admitted connection to its assigned
  owner. `PKT_PEER_STATUS` gives each join a complete roster and reports precise
  arrivals and departures.
- The host applies a packet policy by type. Join-authored replicated state is
  validated and relayed to the other admitted joins. Host-directed intents stay
  at the host. Time packets use a bidirectional direct policy because joins
  report their clock to the host and those reports must not be relayed.
- Exact framing, direction, admitted sender, and payload owner are checked before
  gameplay packets reach main-thread queues.
- Replication targets record their owner. A concrete departure removes only that
  owner's queued packets, interpolation rows, proxies, world-item proxies,
  clock rows, and dedupe records. Other active owners remain intact.
- The join treats loss of its host link as a full remote-session teardown. Hidden
  native NPCs are restored before state is reset, and host-authority suppression
  does not run while the join is offline.
- Existing two-player behavior is preserved: the default host claim is squad 0
  and the default join claim is squad 1. Explicit `ownSquads` configuration
  supplies any other unique rank set.

## Protocol changes

Protocol 56 is intentionally incompatible with earlier builds:

- variable-length HELLO squad claims;
- `PKT_PEER_STATUS` dynamic roster rows;
- `PKT_REJECT` with explicit admission reasons;
- authenticated 32-bit owner assignment and packet-owner validation;
- explicit star-relay and host-directed packet policies.

The configured 32-player default is an admission policy. ENet session capacity
is capped at 4095 participants by its 12-bit peer representation, while protocol
owner IDs retain the full non-reserved 32-bit range. No 32-client live test has
been performed.

## Validation recorded September 15, 2026

| Layer | Result |
| --- | --- |
| Unmodified current-main baseline v100 DLL | PASS |
| Baseline C++ protocol/unit checks | 842/842 pass |
| Current Harness DLL, v100 x64 | PASS |
| Current Release DLL, v100 x64 | PASS |
| Current C++ protocol/unit checks | 905/905 pass |
| Harness contract fixtures | 32/32 pass |
| Crawl oracle fixture | 18/18 pass |
| Real ENet loopback session integration | 25/25 pass |
| Existing two-Kenshi `coop_presence` regression | PASS |

The real ENet integration test covers the existing host plus one-join path and
host plus two joins. It verifies admission, complete rosters, host-to-join,
join-to-host, both join-to-join directions through the host, normal departure of
one join, the surviving join's roster, and traffic in both directions after the
departure.

The two-Kenshi regression result is in
`tools/test-runs/20260915_204130/verdict.json` (ignored runtime artifact). All
gating checks passed: bidirectional presence, suppression churn, snap rate,
march-in-place, and clock sync. Smoothness skipped for insufficient scored
frames and is advisory for this scenario.

The dependency compatibility patch integration suite remains 13/13 pass. Both
DLL configurations use Visual C++ 2010 SP1 x64 (`cl.exe` 16.00.40219.01) and
Windows SDK 7.1; the project was not retargeted.

## PR #50 port decisions

Reused or adapted:

- dynamic host peer registry and roster concept;
- star topology with explicit relay policy;
- explicit squad claims;
- owner-scoped state and disconnect cleanup patterns;
- three-player transport test concepts.

Reimplemented against current main:

- packet tags and protocol version;
- sender and exact-frame validation;
- capacity and identity allocation;
- owner-aware inbound cleanup and current replication maps.

Deferred or rejected:

- fixed three-player caps and special client slots;
- eviction of a live incumbent on a duplicate squad claim;
- colliding fabricated Steam addresses;
- save acknowledgements that ignore the participant identity;
- relay after failed parsing or blanket relay of host-directed intents;
- wholesale source replacement that would discard newer main behavior.

## Remaining risks and validation

The first field test is intentionally limited. The following work remains:

1. Run the packaged Release build on three physical PCs and capture all three
   logs, including a Join A disconnect/reconnect while Join B continues.
2. Add a four-participant ENet integration case, then a four-Kenshi field test
   when the machine setup permits it.
3. Replace scalar speed vote, time, camera, NPC/cell authority, and other
   channel state with per-owner records where required.
4. Implement a dynamic Steam peer registry and validate multiple simultaneous
   Steam peers.
5. Target newcomer save transfer and track save/load transactions and ACKs by
   participant; validate save/load with multiple joins.
6. Audit and test combat, medical state, inventory/equipment, trades, world
   items, carried/prisoner/furniture state, recruitment, buildings, production,
   research, doors, faction state, and money with multiple joins.
7. Add transport-level duplicate-claim and session-full rejection integration
   cases; these cases currently pass in the pure session/protocol tests.

No live three-player or four-player Kenshi run has been completed yet. No claim
is made for 32 simultaneous clients, performance at that scale, multi-peer Steam,
or multi-peer save/load.
