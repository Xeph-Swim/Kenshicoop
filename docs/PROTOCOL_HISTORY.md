# Protocol history

## Version 56: N-player admission and roster

- `PKT_HELLO` carries a bounded list of 32-bit squad-slot claims.
- `PKT_PEER_STATUS` carries dynamic owner presence and all claimed squad slots.
- `PKT_REJECT` reports protocol, claim, capacity, and identity admission failures.
- The host binds an admitted ENet connection to its assigned 32-bit owner ID and
  validates packet direction and payload ownership before dispatch or relay.
- Join-authored replicated state uses an explicit host relay policy. Host-directed
  intent packets stay at the host.
- The host session capacity is configurable and defaults to 32 participants.

This is an intentional breaking change. The handshake rejects older protocol
versions; there is no mixed-version compatibility mode.

Versions through 55 are documented alongside their packet and field definitions
in `src/netproto/Wire.h` and in the repository history that introduced each bump.
