# Three-PC experimental test

This build is ready for a narrow host plus two-client field test over direct
UDP. It validates admission, the full three-player roster, star relay, basic
owned-squad movement, and removal of one join while the other stays connected.

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

## Install the three-PC kit

Build the artifact from the repository root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\make_three_player_experimental_kit.ps1
```

The result is `dist\KenshiCoop-3player-experimental.zip`. It contains a
preconfigured mod folder for each PC:

- `Host\KenshiCoop`
- `Join-A\KenshiCoop`
- `Join-B\KenshiCoop`

Copy the appropriate `KenshiCoop` folder into `<Kenshi>\mods\` on each PC and
enable the mod in Kenshi's launcher. RE_Kenshi 0.3.1 or later is required on
all three PCs.

On both join PCs, edit `coop_config.json` and replace `HOST_LAN_IP` with the
host PC's LAN IPv4 address. Keep the same UDP port on all PCs. Allow inbound
UDP port 27800 through the host's Windows firewall.

The experimental configs deliberately set these channels off:

- coordinated save and load transfer;
- shared speed voting and clock correction;
- remote camera interest hints.

They also set `maxPlayers` to 3. This makes an accidental fourth participant
receive a clean session-full rejection during this test.

## Run the test

1. Start the host first and load the prepared disposable save.
2. Start Join A and Join B. Each loads its local copy of the same save.
3. Open the F2 panel on each PC. The connection should already be online from
   `autoConnect`; if it is not, select UDP and toggle it online.
4. Confirm each player can move only their assigned squad and that all three
   see the other two squads move.
5. Close Join A or toggle it offline. Confirm the host reports only Join A's
   owner ID leaving and Join B stays online and continues moving.
6. Reconnect Join A. A fresh owner ID is expected; the same squad claim should
   be accepted after the prior owner has been removed.

Collect these files from every PC after the run:

- `KenshiCoop_host.log` or `KenshiCoop_join.log` from the Kenshi directory;
- `RE_Kenshi_log.txt`;
- the final 100 lines of `kenshi.log` if a client crashed.

Healthy logs contain protocol `v56`, unique admitted owner IDs, and no
`dropped packet`, `session admission rejected`, or `protocol mismatch` errors.

## Unsafe test scope

The following systems do not yet have three-live-Kenshi validation or still
contain one-peer replication state. Avoid them in this first field test:

- Steam P2P transport: its tunnel still tracks one remote Steam peer. Use UDP.
- Save transfer, coordinated save, and coordinated load. Keep the three local
  save copies disposable and do not save during the connected test.
- Shared speed voting and clock correction. Leave the game at 1x and do not
  pause from a join.
- Remote camera-based interest. The experimental config disables it.
- Cross-owner inventory trades, dropped items, equipment, carrying, cages,
  beds, prisoners, recruitment, and squad-tab reordering.
- Join-versus-join combat, damage, healing, stealth, and NPC authority handoff.
- Construction, crafting, research, doors, faction state, money, and production
  changes made concurrently by different players.
- Late join into a world that has diverged since the common save was copied.

Basic movement and presence are the intended test. A successful session is
evidence for three-player transport and owner isolation, not proof that every
gameplay subsystem is N-player safe or that 32 live clients are supported.
