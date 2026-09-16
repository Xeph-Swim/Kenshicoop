// C++03 wire helpers for N-player admission and explicit star-topology routing.
// This layer has no ENet or engine dependencies, so its malformed-input and
// direction rules are exercised by prototest before the DLL is loaded.
#ifndef KENSHICOOP_SESSION_PROTOCOL_H
#define KENSHICOOP_SESSION_PROTOCOL_H

#include <set>
#include <string>
#include <vector>
#include <cstring>
#include <cstddef>
#include "../../netproto/Wire.h"

namespace coop {

enum { MAX_SQUAD_CLAIMS = 64, MAX_PEER_NAME = 63 };

enum PacketPolicy {
    POLICY_INVALID = 0,
    POLICY_HANDSHAKE,
    POLICY_CLIENT_TO_HOST,
    POLICY_HOST_TO_CLIENT,
    POLICY_BIDIRECTIONAL_DIRECT,
    POLICY_RELAY_STATE
};

inline PacketPolicy packetPolicy(u8 type) {
    switch (type) {
        case PKT_HELLO: case PKT_WELCOME: case PKT_PEER_STATUS: case PKT_REJECT:
            return POLICY_HANDSHAKE;
        case PKT_TIME_PING: case PKT_SPEED_REQ: case PKT_SPAWN_REQ:
        case PKT_SAVE_REQ: case PKT_SAVE_ACK: case PKT_LOAD_REQ:
        case PKT_LOAD_NACK: case PKT_MONEY_DELTA: case PKT_COMBAT_HIT:
            return POLICY_CLIENT_TO_HOST;
        case PKT_TIME_PONG: case PKT_SPEED_SET:
        case PKT_SPAWN_INFO: case PKT_MONEY: case PKT_PROD:
        case PKT_RESEARCH: case PKT_SAVE_BEGIN:
        case PKT_SAVE_FILE: case PKT_SAVE_DONE: case PKT_LOAD_GO:
            return POLICY_HOST_TO_CLIENT;
        // Both directions use PKT_TIME: host samples drive join correction;
        // join samples feed the host brake. A join report is host-directed
        // telemetry and must not be relayed as host-authoritative time.
        case PKT_TIME:
            return POLICY_BIDIRECTIONAL_DIRECT;
        case PKT_ENTITY_BATCH: case PKT_EVENT: case PKT_INV_SNAPSHOT:
        case PKT_WORLD_ITEM: case PKT_WORLD_ITEM_REMOVE:
        case PKT_WORLD_ITEM_CLAIM: case PKT_WORLD_DROP: case PKT_WORLD_PICKUP:
        case PKT_INV_XFER: case PKT_INV_XFER_ACK: case PKT_MEDICAL:
        case PKT_TREATMENT: case PKT_STATS: case PKT_FACTION: case PKT_DOOR:
        case PKT_BUILD_PLACE: case PKT_BUILD_STATE: case PKT_BUILD_DOOR:
        case PKT_BUILD_REMOVE: case PKT_CAM_HINT: case PKT_CELL_CLAIM:
        case PKT_DEED: case PKT_FIXTURE: case PKT_STEALTH:
        case PKT_NPC_CENSUS:
            return POLICY_RELAY_STATE;
        default:
            return POLICY_INVALID;
    }
}

inline bool packetHasOwner(u8 type) {
    // Clock probes authenticate by their bound ENet connection and carry no
    // owner field in their wire structs.
    if (type == PKT_TIME_PING || type == PKT_TIME_PONG) return false;
    const PacketPolicy p = packetPolicy(type);
    return p == POLICY_CLIENT_TO_HOST || p == POLICY_HOST_TO_CLIENT ||
           p == POLICY_BIDIRECTIONAL_DIRECT ||
           p == POLICY_RELAY_STATE;
}

inline bool readPacketOwner(const void* data, unsigned len, u32& ownerId) {
    if (!data || len == 0) return false;
    const u8 type = *(const u8*)data;
    if (!packetHasOwner(type)) return false;
    unsigned offset = 0;
    switch (type) {
        case PKT_ENTITY_BATCH: offset = offsetof(EntityBatchHeader, ownerId); break;
        case PKT_EVENT: offset = offsetof(EventPacket, ownerId); break;
        case PKT_INV_SNAPSHOT: offset = offsetof(InvSnapshotHeader, ownerId); break;
        case PKT_WORLD_ITEM: offset = offsetof(WorldItemSnapshotHeader, ownerId); break;
        case PKT_WORLD_ITEM_REMOVE: offset = offsetof(WorldItemRemoveHeader, ownerId); break;
        case PKT_WORLD_ITEM_CLAIM: offset = offsetof(WorldItemClaimHeader, ownerId); break;
        case PKT_NPC_CENSUS: offset = offsetof(NpcCensusHeader, ownerId); break;
        case PKT_WORLD_DROP: offset = offsetof(WorldDropPacket, ownerId); break;
        case PKT_WORLD_PICKUP: offset = offsetof(WorldPickupPacket, ownerId); break;
        case PKT_INV_XFER: offset = offsetof(InvXferPacket, ownerId); break;
        case PKT_INV_XFER_ACK: offset = offsetof(InvXferAckPacket, ownerId); break;
        case PKT_MEDICAL: offset = offsetof(MedicalPacket, ownerId); break;
        case PKT_TREATMENT: offset = offsetof(TreatmentPacket, ownerId); break;
        case PKT_COMBAT_HIT: offset = offsetof(CombatHitPacket, ownerId); break;
        case PKT_SPEED_REQ: case PKT_SPEED_SET: offset = offsetof(SpeedPacket, ownerId); break;
        case PKT_STATS: offset = offsetof(StatsPacket, ownerId); break;
        case PKT_MONEY: offset = offsetof(MoneyPacket, ownerId); break;
        case PKT_MONEY_DELTA: offset = offsetof(MoneyDeltaPacket, ownerId); break;
        case PKT_FACTION: offset = offsetof(FactionPacket, ownerId); break;
        case PKT_TIME: offset = offsetof(TimePacket, ownerId); break;
        case PKT_DOOR: offset = offsetof(DoorPacket, ownerId); break;
        case PKT_PROD: offset = offsetof(ProdPacket, ownerId); break;
        case PKT_RESEARCH: offset = offsetof(ResearchPacket, ownerId); break;
        case PKT_DEED: offset = offsetof(DeedPacket, ownerId); break;
        case PKT_FIXTURE: offset = offsetof(FixturePacket, ownerId); break;
        case PKT_BUILD_PLACE: offset = offsetof(BuildPlacePacket, ownerId); break;
        case PKT_BUILD_STATE: offset = offsetof(BuildStatePacket, ownerId); break;
        case PKT_BUILD_DOOR: offset = offsetof(BuildDoorPacket, ownerId); break;
        case PKT_BUILD_REMOVE: offset = offsetof(BuildRemovePacket, ownerId); break;
        case PKT_STEALTH: offset = offsetof(StealthPacket, ownerId); break;
        case PKT_SPAWN_REQ: offset = offsetof(SpawnReqPacket, ownerId); break;
        case PKT_SPAWN_INFO: offset = offsetof(SpawnInfoPacket, ownerId); break;
        case PKT_SAVE_REQ: offset = offsetof(SaveReqPacket, ownerId); break;
        case PKT_SAVE_BEGIN: offset = offsetof(SaveBeginPacket, ownerId); break;
        case PKT_SAVE_FILE: offset = offsetof(SaveFileHeader, ownerId); break;
        case PKT_SAVE_DONE: offset = offsetof(SaveDoneHeader, ownerId); break;
        case PKT_SAVE_ACK: offset = offsetof(SaveAckPacket, ownerId); break;
        case PKT_LOAD_GO: offset = offsetof(LoadGoPacket, ownerId); break;
        case PKT_LOAD_REQ: offset = offsetof(LoadReqPacket, ownerId); break;
        case PKT_LOAD_NACK: offset = offsetof(LoadNackPacket, ownerId); break;
        case PKT_CAM_HINT: offset = offsetof(CamHintPacket, ownerId); break;
        case PKT_CELL_CLAIM: offset = offsetof(CellClaimPacket, ownerId); break;
        default: return false;
    }
    if (len < offset + sizeof(ownerId)) return false;
    std::memcpy(&ownerId, (const u8*)data + offset, sizeof(ownerId));
    return ownerId != OWNER_ID_ALL;
}

inline bool packetAllowedFromClient(u8 type) {
    const PacketPolicy p = packetPolicy(type);
    return p == POLICY_CLIENT_TO_HOST || p == POLICY_BIDIRECTIONAL_DIRECT ||
           p == POLICY_RELAY_STATE;
}

inline bool packetAllowedFromHost(u8 type) {
    const PacketPolicy p = packetPolicy(type);
    return p == POLICY_HOST_TO_CLIENT || p == POLICY_BIDIRECTIONAL_DIRECT ||
           p == POLICY_RELAY_STATE;
}

inline bool packetRelayedByHost(u8 type) {
    return packetPolicy(type) == POLICY_RELAY_STATE;
}

inline bool exactCountedSize(unsigned len, unsigned headerSize,
                             unsigned count, unsigned itemSize,
                             unsigned maxCount) {
    return count <= maxCount && len >= headerSize &&
           count <= (len - headerSize) / itemSize &&
           len == headerSize + count * itemSize;
}

// Complete framing check performed before a gameplay packet is dispatched or
// relayed. Existing handlers may still repeat semantic bounds close to use.
inline bool gameplayPacketFramingValid(u8 type, const void* data, unsigned len) {
    if (!data || len == 0 || *(const u8*)data != type) return false;
#define KC_FIXED_PACKET(tag, packetTypeName) case tag: return len == sizeof(packetTypeName)
    switch (type) {
        KC_FIXED_PACKET(PKT_EVENT, EventPacket);
        KC_FIXED_PACKET(PKT_WORLD_DROP, WorldDropPacket);
        KC_FIXED_PACKET(PKT_WORLD_PICKUP, WorldPickupPacket);
        KC_FIXED_PACKET(PKT_INV_XFER, InvXferPacket);
        KC_FIXED_PACKET(PKT_INV_XFER_ACK, InvXferAckPacket);
        KC_FIXED_PACKET(PKT_MEDICAL, MedicalPacket);
        KC_FIXED_PACKET(PKT_TREATMENT, TreatmentPacket);
        KC_FIXED_PACKET(PKT_COMBAT_HIT, CombatHitPacket);
        case PKT_SPEED_REQ: case PKT_SPEED_SET: return len == sizeof(SpeedPacket);
        KC_FIXED_PACKET(PKT_STATS, StatsPacket);
        KC_FIXED_PACKET(PKT_MONEY, MoneyPacket);
        KC_FIXED_PACKET(PKT_MONEY_DELTA, MoneyDeltaPacket);
        KC_FIXED_PACKET(PKT_FACTION, FactionPacket);
        KC_FIXED_PACKET(PKT_TIME, TimePacket);
        KC_FIXED_PACKET(PKT_DOOR, DoorPacket);
        KC_FIXED_PACKET(PKT_PROD, ProdPacket);
        KC_FIXED_PACKET(PKT_RESEARCH, ResearchPacket);
        KC_FIXED_PACKET(PKT_DEED, DeedPacket);
        KC_FIXED_PACKET(PKT_FIXTURE, FixturePacket);
        KC_FIXED_PACKET(PKT_BUILD_PLACE, BuildPlacePacket);
        KC_FIXED_PACKET(PKT_BUILD_STATE, BuildStatePacket);
        KC_FIXED_PACKET(PKT_BUILD_DOOR, BuildDoorPacket);
        KC_FIXED_PACKET(PKT_BUILD_REMOVE, BuildRemovePacket);
        KC_FIXED_PACKET(PKT_STEALTH, StealthPacket);
        KC_FIXED_PACKET(PKT_SPAWN_REQ, SpawnReqPacket);
        KC_FIXED_PACKET(PKT_SPAWN_INFO, SpawnInfoPacket);
        KC_FIXED_PACKET(PKT_SAVE_REQ, SaveReqPacket);
        KC_FIXED_PACKET(PKT_SAVE_BEGIN, SaveBeginPacket);
        KC_FIXED_PACKET(PKT_SAVE_ACK, SaveAckPacket);
        KC_FIXED_PACKET(PKT_LOAD_GO, LoadGoPacket);
        KC_FIXED_PACKET(PKT_LOAD_REQ, LoadReqPacket);
        KC_FIXED_PACKET(PKT_LOAD_NACK, LoadNackPacket);
        KC_FIXED_PACKET(PKT_CAM_HINT, CamHintPacket);
        KC_FIXED_PACKET(PKT_CELL_CLAIM, CellClaimPacket);
        KC_FIXED_PACKET(PKT_TIME_PING, TimePingPacket);
        KC_FIXED_PACKET(PKT_TIME_PONG, TimePongPacket);
        default: break;
    }
#undef KC_FIXED_PACKET

    if (type == PKT_ENTITY_BATCH && len >= sizeof(EntityBatchHeader)) {
        EntityBatchHeader h; std::memcpy(&h, data, sizeof(h));
        return exactCountedSize(len, sizeof(h), h.count, sizeof(EntityState), ENTITY_BATCH_MAX);
    }
    if (type == PKT_INV_SNAPSHOT && len >= sizeof(InvSnapshotHeader)) {
        InvSnapshotHeader h; std::memcpy(&h, data, sizeof(h));
        return exactCountedSize(len, sizeof(h), h.count, sizeof(InvItemEntry), INV_ITEMS_MAX);
    }
    if (type == PKT_WORLD_ITEM && len >= sizeof(WorldItemSnapshotHeader)) {
        WorldItemSnapshotHeader h; std::memcpy(&h, data, sizeof(h));
        return exactCountedSize(len, sizeof(h), h.count, sizeof(WorldItemEntry), WORLD_ITEMS_MAX);
    }
    if (type == PKT_WORLD_ITEM_REMOVE && len >= sizeof(WorldItemRemoveHeader)) {
        WorldItemRemoveHeader h; std::memcpy(&h, data, sizeof(h));
        return exactCountedSize(len, sizeof(h), h.count, sizeof(u32), 255);
    }
    if (type == PKT_WORLD_ITEM_CLAIM && len >= sizeof(WorldItemClaimHeader)) {
        WorldItemClaimHeader h; std::memcpy(&h, data, sizeof(h));
        return exactCountedSize(len, sizeof(h), h.count, sizeof(u32), 255);
    }
    if (type == PKT_NPC_CENSUS && len >= sizeof(NpcCensusHeader)) {
        NpcCensusHeader h; std::memcpy(&h, data, sizeof(h));
        return exactCountedSize(len, sizeof(h), h.count,
                                5 * sizeof(u32) + 3 * sizeof(f32), NPC_CENSUS_MAX);
    }
    if (type == PKT_SAVE_FILE && len >= sizeof(SaveFileHeader)) {
        SaveFileHeader h; std::memcpy(&h, data, sizeof(h));
        if (h.pathLen == 0 || h.pathLen > SAVE_PATH_MAX || h.dataLen > SAVE_CHUNK_MAX)
            return false;
        return h.pathLen <= len - sizeof(h) &&
               h.dataLen == len - sizeof(h) - h.pathLen;
    }
    if (type == PKT_SAVE_DONE && len >= sizeof(SaveDoneHeader)) {
        SaveDoneHeader h; std::memcpy(&h, data, sizeof(h));
        return exactCountedSize(len, sizeof(h), h.fileCount, sizeof(u32),
                                (len - sizeof(h)) / sizeof(u32));
    }
    return false;
}

inline bool validClaims(const std::set<u32>& claims) {
    return !claims.empty() && claims.size() <= MAX_SQUAD_CLAIMS &&
           claims.count(OWNER_ID_ALL) == 0;
}

inline bool encodeHello(const std::set<u32>& claims, const std::string& name,
                        std::vector<u8>& out) {
    out.clear();
    if (!validClaims(claims) || name.size() > MAX_PEER_NAME) return false;
    const unsigned bytes = (unsigned)sizeof(HelloPacket) +
                           (unsigned)claims.size() * sizeof(u32) +
                           (unsigned)name.size();
    out.resize(bytes);
    HelloPacket h;
    h.type = (u8)PKT_HELLO;
    h.version = PROTOCOL_VERSION;
    h.claimCount = (u16)claims.size();
    h.nameLen = (u8)name.size();
    std::memcpy(&out[0], &h, sizeof(h));
    u8* p = &out[0] + sizeof(h);
    for (std::set<u32>::const_iterator i = claims.begin(); i != claims.end(); ++i) {
        std::memcpy(p, &*i, sizeof(u32));
        p += sizeof(u32);
    }
    if (!name.empty()) std::memcpy(p, name.data(), name.size());
    return true;
}

inline bool decodeHello(const void* data, unsigned len, u16& version,
                        std::set<u32>& claims, std::string& name) {
    claims.clear(); name.clear(); version = 0;
    if (!data || len < sizeof(HelloPacket)) return false;
    HelloPacket h;
    std::memcpy(&h, data, sizeof(h));
    if (h.type != PKT_HELLO || h.claimCount == 0 ||
        h.claimCount > MAX_SQUAD_CLAIMS || h.nameLen > MAX_PEER_NAME) return false;
    const unsigned expected = (unsigned)sizeof(h) +
                              (unsigned)h.claimCount * sizeof(u32) + h.nameLen;
    if (len != expected) return false;
    const u8* p = (const u8*)data + sizeof(h);
    for (unsigned i = 0; i < h.claimCount; ++i) {
        u32 claim;
        std::memcpy(&claim, p, sizeof(claim));
        p += sizeof(claim);
        if (claim == OWNER_ID_ALL || !claims.insert(claim).second) {
            claims.clear(); return false;
        }
    }
    if (h.nameLen) name.assign((const char*)p, h.nameLen);
    version = h.version;
    return true;
}

inline bool encodePeerStatus(bool present, u32 ownerId,
                             const std::set<u32>& claims, std::vector<u8>& out) {
    out.clear();
    if (ownerId == OWNER_ID_ALL || !validClaims(claims)) return false;
    out.resize(sizeof(PeerStatusPacket) + claims.size() * sizeof(u32));
    PeerStatusPacket s;
    s.type = (u8)PKT_PEER_STATUS;
    s.present = present ? 1 : 0;
    s.ownerId = ownerId;
    s.claimCount = (u16)claims.size();
    std::memcpy(&out[0], &s, sizeof(s));
    u8* p = &out[0] + sizeof(s);
    for (std::set<u32>::const_iterator i = claims.begin(); i != claims.end(); ++i) {
        std::memcpy(p, &*i, sizeof(u32));
        p += sizeof(u32);
    }
    return true;
}

inline bool decodePeerStatus(const void* data, unsigned len, bool& present,
                             u32& ownerId, std::set<u32>& claims) {
    claims.clear(); present = false; ownerId = OWNER_ID_ALL;
    if (!data || len < sizeof(PeerStatusPacket)) return false;
    PeerStatusPacket s;
    std::memcpy(&s, data, sizeof(s));
    if (s.type != PKT_PEER_STATUS || s.present > 1 ||
        s.ownerId == OWNER_ID_ALL || s.claimCount == 0 ||
        s.claimCount > MAX_SQUAD_CLAIMS ||
        len != sizeof(s) + (unsigned)s.claimCount * sizeof(u32)) return false;
    const u8* p = (const u8*)data + sizeof(s);
    for (unsigned i = 0; i < s.claimCount; ++i) {
        u32 claim;
        std::memcpy(&claim, p, sizeof(claim)); p += sizeof(claim);
        if (claim == OWNER_ID_ALL || !claims.insert(claim).second) {
            claims.clear(); return false;
        }
    }
    present = s.present != 0;
    ownerId = s.ownerId;
    return true;
}

inline u8 rejectionForAdmission(int admission) {
    // SessionRegistry::Admission values intentionally remain transport-free.
    switch (admission) {
        case 1: return (u8)REJECT_INVALID_CLAIMS;
        case 2: return (u8)REJECT_CLAIM_TAKEN;
        case 3: return (u8)REJECT_SESSION_FULL;
        case 4: return (u8)REJECT_IDS_EXHAUSTED;
        default: return (u8)REJECT_INVALID_CLAIMS;
    }
}

} // namespace coop
#endif
