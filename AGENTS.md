\# KenshiCoop N-Player Development



\## Primary goal



Generalize KenshiCoop from its current two-player architecture into an N-player architecture.



The implementation must NOT be specifically designed for exactly three players.



Immediate requirements:



\* Configurable maximum player count.

\* Default maximum player count: 32.

\* Three players are the immediate real-world target.

\* Two-, three-, and four-player operation must be used as initial validation targets.

\* Four-player validation is specifically intended to prove the design is genuinely N-player rather than a hardcoded three-player implementation.

\* Do not claim 32 simultaneous Kenshi clients are supported or performant unless they have actually been tested.



\## Repository branches



Development branch:



`astra/n-player-32`



Current upstream-derived baseline:



`main`



Reference implementation:



`pr50-three-player`



The reference branch corresponds to upstream PR #50:



https://github.com/nhoral/KenshiCoop/pull/50



PR #50 already contains substantial work involving:



\* host peer registry

\* star-topology relay

\* squad-slot claims

\* per-owner replication state

\* targeted save/load transfer

\* owner-isolated disconnect cleanup

\* Steam peer registry

\* three-player fixture/test infrastructure

\* three-player smoke testing

\* WAN simulation



Study this work carefully.



Do NOT blindly merge PR #50.



It was developed against an earlier upstream state. Preserve newer changes from current upstream/main and port, adapt, or reimplement PR #50's useful architecture where appropriate.



\## Architecture



Player count must be data, not hardcoded branching.



Do not create architecture such as:



\* client1

\* client2

\* client3

\* special third-player paths



Prefer concepts such as:



\* PeerId -> PeerState

\* OwnerId -> OwnerState

\* SquadId -> OwnerId



Use dynamic containers/registries where appropriate.



A configured 32-player maximum is a policy limit rather than the representational maximum of the wire protocol.



Use player/owner identifiers with substantially more address space than 32 participants.



\## Authority model



Preserve KenshiCoop's fundamental model unless evidence requires a specific change:



\* Host is authoritative for shared world state.

\* Each player is authoritative for their own player-controlled squad.

\* Clients communicate through the host.

\* Clients do not directly communicate authoritative gameplay state to each other.

\* Join-originated state required by other joins is relayed or authoritatively propagated through the host according to explicit packet policy.



Do not indiscriminately relay every packet type.



Validate sender identity and ownership.



\## Squad ownership



Squad ownership must be explicit and unique.



The host must reject duplicate active squad-slot claims.



A disconnect, timeout, or crash involving one player must not clear or corrupt another connected player's ownership or replication state.



Reconnect behavior must be deterministic.



Do not rely on assumptions equivalent to:



\* squad 0 = host forever

\* squad 1 = only possible client



except where necessary for backward compatibility.



\## Audit requirements



Audit the entire repository for assumptions that restrict multiplayer to host + one join.



Pay particular attention to:



\* single-peer variables

\* one-remote-player state

\* fixed-size two-player structures

\* booleans representing remote presence

\* `ownRank` assumptions

\* host/client branches that actually represent ownership

\* sequence gates

\* interpolation state

\* pins/driven records

\* combat replication

\* damage

\* health

\* NPC authority/presence

\* inventories

\* equipment

\* direct trading

\* dropped/world items

\* containers

\* buildings/construction

\* crafting

\* research

\* doors/locks

\* beds/cages

\* prisoners

\* carried characters

\* factions/world states

\* money

\* game speed

\* camera hints

\* save/load synchronization

\* disconnect/reconnect cleanup

\* Steam peer tracking



Remote-participant state must be keyed by stable owner/peer identity wherever necessary.



\## Networking



Maintain the project's existing ENet and Steam P2P approaches.



The host must be capable of maintaining multiple simultaneous peer connections.



For join-authored packet types that other joins need to observe, implement an explicit relay policy.



Do not relay host-directed intent packets as though they were authoritative replicated state.



Malformed, unauthenticated, duplicate, out-of-range, or ownership-invalid packets must fail safely.



Protocol-breaking changes require an intentional protocol-version bump.



\## Save/load



Save synchronization must work with multiple joins.



Where practical, large save transfers intended for a newly connected participant should be targeted to that participant rather than redundantly broadcast to established players.



Existing connected players must remain valid when another participant:



\* joins

\* disconnects

\* reconnects

\* saves

\* loads



\## Toolchain



KenshiLib plugin compatibility is mandatory.



Do not casually retarget the plugin away from its required Visual C++ 2010/v100 toolchain.



Remain compatible with the C++ language/library level supported by the required toolchain.



Avoid introducing modern C++ features that cannot compile under the project's required compiler.



Do not modify reverse-engineered Kenshi engine interaction code unnecessarily.



\## Baseline requirement



Before making the large multiplayer refactor:



1\. Confirm the Git working tree and branch.

2\. Read repository documentation.

3\. Inspect the build system.

4\. Inspect the protocol.

5\. Inspect replication architecture.

6\. Inspect current test infrastructure.

7\. Build the unmodified current baseline if the required external toolchain is available.

8\. Run available unit/protocol tests.

9\. Record exact baseline results.



If an external prerequisite prevents the baseline build, identify it precisely. Do not bypass compatibility requirements merely to make compilation succeed.



\## Validation



After significant networking changes, build and run the relevant automated tests.



Required progression:



\### Two players



Existing host + one join behavior must continue to work.



\### Three players



Host + two joins.



Validate relevant directional propagation:



\* Host -> Join A

\* Host -> Join B

\* Join A -> Host

\* Join B -> Host

\* Join A -> Join B through host relay

\* Join B -> Join A through host relay



\### Four players



Host + three joins.



This is required as an architectural validation that no disguised three-player assumptions remain.



\### Session maximum



Verify configurable maximum-player admission and clean rejection when the configured session capacity is reached.



It is not necessary to launch 32 Kenshi instances merely to prove that the policy maximum can be configured to 32.



\## Failure testing



Exercise where practical:



\* normal disconnect

\* timeout/crash

\* reconnect

\* duplicate squad-slot claim

\* session full

\* malformed/unsupported protocol

\* newcomer joining an active multiplayer session

\* save with multiple participants connected

\* load with multiple participants connected



A departing peer must never wipe another active participant's replication state.



\## Performance



Correctness for two to four participants comes first.



Do not prematurely optimize specifically for 32 live Kenshi processes.



However, avoid unnecessary O(N²) broadcasts where host-authoritative or interest-filtered replication can provide equivalent semantics.



Instrument packet volume or replication cost when useful.



\## Git rules



Work on:



`astra/n-player-32`



Do not make feature commits directly on `main`.



Never force-push.



Never rewrite upstream history.



Never push directly to `nhoral/KenshiCoop`.



`origin` is the writable Xeph-Swim fork.



`upstream` is read-only reference/upstream development.



Keep changes in logical commits.



Do not commit fetched dependency repositories or generated build artifacts.



\## Definition of first production milestone



The first production-quality target requires:



\* current upstream fixes retained

\* existing two-player behavior preserved

\* host + two joins working

\* host + three joins validated sufficiently to prove N-generic architecture

\* dynamic peer/player architecture

\* configurable maximum players

\* default configured maximum = 32

\* no hardcoded third-player architecture

\* owner-isolated disconnect handling

\* multi-peer save/load behavior

\* protocol documentation updated

\* tests updated

\* remaining limitations documented accurately



