# Three-PC experimental test package

The package installs one identical `KenshiCoop` mod folder on every PC and
selects Steam P2P by default. The automated host plus two-client admission,
roster, relay, and owner-isolated disconnect gate is green over UDP.

The current Steam transport still has a single-remote ENet tunnel. Therefore a
host plus two simultaneous Steam clients is **not yet a supported or validated
runtime session**. This package prepares the common install and Steam setup;
use the existing UDP path for the three-client runtime gate until multi-peer
Steam transport is implemented.

It is an experimental transport and presence milestone. Do not use an important
save and do not enable the systems listed under **Unsafe test scope**.

## Prepare one identical save

1. On the host, make a disposable save with at least three player-squad tabs.
2. Exit Kenshi so the save is no longer being written.
3. Copy that complete save folder to both join PCs under the location their
   Kenshi installation uses (`%LOCALAPPDATA%\kenshi\save` in the usual setup).
4. Confirm all three PCs can load the same save offline and see the same squad
   tabs in the same order.

Squad ownership uses zero-based tab ranks for this test:

| PC | Role | `ownSquads` | Visible squad tab |
| --- | --- | ---: | --- |
| Host | host | `"0"` | first tab |
| Join A | join | `"1"` | second tab |
| Join B | join | `"2"` | third tab |

The host rejects a join that claims a slot already held by an active player.

## Install the same folder on all three PCs

Build the artifact from the repository root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\make_three_player_experimental_kit.ps1
```

The result is `dist\KenshiCoop-3player-steam-experimental.zip`. It contains
one `KenshiCoop` folder. Copy that exact folder into `<Kenshi>\mods\` on all
three PCs and enable the mod in Kenshi's launcher. RE_Kenshi 0.3.1 or later is
required on all three PCs. Do not make Host, Join-A, or Join-B copies.

Keep Steam running and online on all PCs. No IP address, port forwarding, or
per-machine config edit is needed for the Steam setup.

The experimental config enables coordinated save and load transfer so a join
can be brought into the host's world and later follow an authoritative save or
load. It deliberately leaves these channels off:

- shared speed voting and clock correction;
- remote camera interest hints.

They also set `maxPlayers` to 3. This makes an accidental fourth participant
receive a clean session-full rejection during this test.

## Steam P2P smoke test (one host plus one join)

The shared package is ready for this two-PC Steam check:

1. Start the host first and load the prepared disposable save.
2. Open F2 on both PCs and leave **Transport: STEAM** selected.
3. Each player clicks **Copy my Steam ID** and sends it to the other; the
   receiving player clicks **Paste friend's Steam ID**. Set one PC to
   **Role: HOST**, the other to **Role: JOIN**, and toggle **Connection** to
   **ONLINE**.
4. Confirm the join can see and move its assigned squad, then toggle it
   **OFFLINE** and reconnect once.

The current Steam tunnel has one configured remote peer. Do not start a second
join against this host and count it as a three-player result.

## Three-player gate

Run the host plus two joins through the existing direct-UDP setup and automated
session test. If you use the shared folder manually for that test, switch the
F2 panel to **UDP** and provide the host's address in each local
`coop_config.json`; this is a transport-specific test override, not a reason to
create separate Host, Join-A, or Join-B mod folders. Confirm each player can
move only their assigned squad, all three see the other two squads, and one
join can leave while the other remains connected. Reconnect the departed join;
a fresh owner ID is expected and the prior squad claim should be accepted.

Collect these files from every PC after the run:

- `KenshiCoop_host.log` or `KenshiCoop_join.log` from the Kenshi directory;
- `RE_Kenshi_log.txt`;
- the final 100 lines of `kenshi.log` if a client crashed.

Healthy logs contain protocol `v56`, unique admitted owner IDs, and no
`dropped packet`, `session admission rejected`, or `protocol mismatch` errors.

## Unsafe test scope

The following systems do not yet have three-live-Kenshi validation or still
contain one-peer replication state. Avoid them in this first field test:

- Steam P2P transport: its tunnel still tracks one remote Steam peer. Do not
  treat a three-client Steam run as supported; use the UDP path for that gate.
- Save/load transfer is enabled, but keep the save disposable. Do not interrupt
  a transfer or close either game while a save/load operation is in progress.
- Shared speed voting and clock correction. Leave the game at 1x and do not
  pause from a join.
- Remote camera-based interest. The experimental config disables it.
- Cross-owner inventory trades, dropped items, equipment, carrying, cages,
  beds, prisoners, recruitment, and squad-tab reordering.
- Join-versus-join combat, damage, healing, stealth, and NPC authority handoff.
- Construction, crafting, research, doors, faction state, money, and production
  changes made concurrently by different players.
- Late join into a world that has diverged since the common save was copied.

Basic movement and presence are the intended test. A successful UDP session is
evidence for three-player transport and owner isolation, not proof that every
gameplay subsystem is N-player safe, that Steam handles multiple clients, or
that 32 live clients are supported.
