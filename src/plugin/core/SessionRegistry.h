// C++03 session admission policy. No engine, transport, or wire-layout changes.
// The host owns this registry on one thread. Transport integration must bind
// an accepted OwnerId to its connection and validate packets against that binding.
#ifndef KENSHICOOP_SESSION_REGISTRY_H
#define KENSHICOOP_SESSION_REGISTRY_H

#include <map>
#include <set>
#include "../../netproto/Wire.h"

namespace coop {

class SessionRegistry {
public:
    enum { DEFAULT_MAX_PLAYERS = 32 };
    typedef std::set<u32> SquadClaims;
    typedef std::map<u32, SquadClaims> Owners;
    enum Admission { ACCEPTED, INVALID_CLAIMS, CLAIM_TAKEN, SESSION_FULL,
                     IDS_EXHAUSTED };

    SessionRegistry() : maxPlayers_(DEFAULT_MAX_PLAYERS), nextId_(1) {
        owners_[0].insert(0); // backwards-compatible default; configure can change it
        ownerBySquad_[0] = 0;
    }

    // Configure before admitting joins. Failed configuration leaves all state
    // intact. Capacity counts the host; it is independent of the u32 ID space.
    bool configure(u32 maxPlayers, const SquadClaims& hostClaims) {
        if (owners_.size() != 1 || maxPlayers < 2 ||
            maxPlayers == OWNER_ID_ALL || !validClaims(hostClaims)) return false;
        maxPlayers_ = maxPlayers;
        owners_[0] = hostClaims;
        ownerBySquad_.clear();
        for (SquadClaims::const_iterator i = hostClaims.begin(); i != hostClaims.end(); ++i)
            ownerBySquad_[*i] = 0;
        // Never reset nextId_: packets from a departed owner must not acquire
        // authority over a later connection when its squad is reclaimed.
        return true;
    }

    Admission admit(const SquadClaims& claims, u32& assignedId) {
        assignedId = OWNER_ID_ALL;
        if (!validClaims(claims)) return INVALID_CLAIMS;
        for (SquadClaims::const_iterator i = claims.begin(); i != claims.end(); ++i)
            if (ownerBySquad_.count(*i)) return CLAIM_TAKEN;
        if (owners_.size() >= maxPlayers_) return SESSION_FULL;
        if (nextId_ == OWNER_ID_ALL) return IDS_EXHAUSTED;
        const u32 id = nextId_++;
        owners_[id] = claims;
        for (SquadClaims::const_iterator i = claims.begin(); i != claims.end(); ++i)
            ownerBySquad_[*i] = id;
        assignedId = id;
        return ACCEPTED;
    }

    // Call only after the transport has established that this connection left
    // or timed out. A duplicate claim is never permission to evict an incumbent.
    bool remove(u32 ownerId) {
        if (ownerId == 0) return false;
        Owners::iterator owner = owners_.find(ownerId);
        if (owner == owners_.end()) return false;
        for (SquadClaims::const_iterator i = owner->second.begin();
             i != owner->second.end(); ++i) ownerBySquad_.erase(*i);
        owners_.erase(owner);
        return true;
    }

    bool contains(u32 ownerId) const { return owners_.count(ownerId) != 0; }
    u32 squadOwner(u32 squad) const {
        std::map<u32, u32>::const_iterator i = ownerBySquad_.find(squad);
        return i == ownerBySquad_.end() ? OWNER_ID_ALL : i->second;
    }
    bool owns(u32 ownerId, u32 squad) const {
        return ownerId != OWNER_ID_ALL && squadOwner(squad) == ownerId;
    }
    u32 maxPlayers() const { return maxPlayers_; }
    const Owners& owners() const { return owners_; }

private:
    static bool validClaims(const SquadClaims& claims) {
        return !claims.empty() && claims.count(OWNER_ID_ALL) == 0;
    }
    u32 maxPlayers_;
    u32 nextId_;
    Owners owners_;
    std::map<u32, u32> ownerBySquad_;
};

} // namespace coop
#endif
