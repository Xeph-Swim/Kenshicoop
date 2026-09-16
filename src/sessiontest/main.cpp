// Real NetLink loopback integration test: the same ENet worker, handshake,
// roster and relay code that ships in the plugin, without launching Kenshi.

#define _CRT_SECURE_NO_WARNINGS 1

#include <windows.h>
#include <cstdio>
#include <cstring>
#include <deque>
#include <set>
#include <string>

#include "../plugin/CoopLog.h"
#include "../plugin/net/NetLink.h"

using namespace coop;

namespace {
int g_checks = 0;
int g_failed = 0;

void check(const char* name, bool ok) {
    ++g_checks;
    std::printf("  %s  %s\n", ok ? "ok " : "FAIL", name);
    if (!ok) ++g_failed;
}

void drainIds(Inbound& in, std::set<u32>& connects, std::set<u32>& leaves) {
    std::deque<u32> q;
    in.drainConnects(q);
    while (!q.empty()) { connects.insert(q.front()); q.pop_front(); }
    in.drainLeaves(q);
    while (!q.empty()) { leaves.insert(q.front()); q.pop_front(); }
}

bool waitRoster(NetLink& host, NetLink& a, NetLink* b,
                Inbound& hi, Inbound& ai, Inbound* bi,
                std::set<u32>& hc, std::set<u32>& ac, std::set<u32>& bc,
                DWORD timeoutMs) {
    std::set<u32> unused;
    const DWORD begin = GetTickCount();
    while ((GetTickCount() - begin) < timeoutMs) {
        drainIds(hi, hc, unused);
        drainIds(ai, ac, unused);
        if (bi) drainIds(*bi, bc, unused);
        const u32 aid = a.localId();
        const u32 bid = b ? b->localId() : OWNER_ID_ALL;
        if (aid != 0 && aid != OWNER_ID_ALL && hc.count(aid) && ac.count(0)) {
            if (!b) return true;
            if (bid != 0 && bid != OWNER_ID_ALL && bid != aid &&
                hc.count(bid) && ac.count(bid) && bc.count(0) && bc.count(aid))
                return true;
        }
        Sleep(10);
    }
    return false;
}

void queueMarker(NetLink& from, u32 owner, u32 marker) {
    EventPacket ev;
    std::memset(&ev, 0, sizeof(ev));
    ev.type = (u8)PKT_EVENT;
    ev.ownerId = owner;
    ev.eventId = marker;
    from.queueEvent(ev);
}

bool waitEvent(Inbound& in, u32 owner, u32 marker, DWORD timeoutMs) {
    const DWORD begin = GetTickCount();
    while ((GetTickCount() - begin) < timeoutMs) {
        std::deque<InboundEvent> events;
        in.drainEvents(events);
        for (std::deque<InboundEvent>::const_iterator it = events.begin();
             it != events.end(); ++it) {
            if (it->ownerId == owner && it->ev.eventId == marker) return true;
        }
        Sleep(10);
    }
    return false;
}

bool waitLeave(Inbound& in, u32 owner, DWORD timeoutMs) {
    const DWORD begin = GetTickCount();
    while ((GetTickCount() - begin) < timeoutMs) {
        std::deque<u32> leaves;
        in.drainLeaves(leaves);
        for (std::deque<u32>::const_iterator it = leaves.begin();
             it != leaves.end(); ++it)
            if (*it == owner) return true;
        Sleep(10);
    }
    return false;
}

std::set<u32> claim(u32 slot) {
    std::set<u32> result;
    result.insert(slot);
    return result;
}

void testTwoPlayer(int port) {
    std::printf("== two-player NetLink regression ==\n");
    Inbound hi, ai;
    NetLink host, a;
    check("host session config", host.setSessionConfig(2, claim(0)));
    check("join session config", a.setSessionConfig(2, claim(1)));
    check("host starts", host.startHost(port, &hi));
    Sleep(50);
    check("join starts", a.startClient("127.0.0.1", port, &ai));

    std::set<u32> hc, ac, none;
    check("host + one join admission and roster",
          waitRoster(host, a, 0, hi, ai, 0, hc, ac, none, 5000));
    const u32 aid = a.localId();
    queueMarker(a, aid, 1001);
    check("join state reaches host", waitEvent(hi, aid, 1001, 3000));
    queueMarker(host, 0, 1002);
    check("host state reaches join", waitEvent(ai, 0, 1002, 3000));

    a.stop();
    check("host observes normal join disconnect", waitLeave(hi, aid, 3000));
    host.stop();
}

void testThreePlayer(int port) {
    std::printf("== host + two-client NetLink admission/roster/relay ==\n");
    Inbound hi, ai, bi;
    NetLink host, a, b;
    check("host session config (3 players)", host.setSessionConfig(3, claim(0)));
    check("join A squad claim", a.setSessionConfig(3, claim(1)));
    check("join B squad claim", b.setSessionConfig(3, claim(2)));
    check("host starts", host.startHost(port, &hi));
    Sleep(50);
    check("join A starts", a.startClient("127.0.0.1", port, &ai));
    check("join B starts", b.startClient("127.0.0.1", port, &bi));

    std::set<u32> hc, ac, bc;
    check("all clients admitted and complete roster announced",
          waitRoster(host, a, &b, hi, ai, &bi, hc, ac, bc, 5000));
    const u32 aid = a.localId();
    const u32 bid = b.localId();
    check("owner ids are unique and wider than squad slots",
          aid != 0 && bid != 0 && aid != bid);

    queueMarker(a, aid, 2001);
    check("join A state reaches host", waitEvent(hi, aid, 2001, 3000));
    check("join A state relays through host to join B",
          waitEvent(bi, aid, 2001, 3000));

    queueMarker(b, bid, 2002);
    check("join B state reaches host", waitEvent(hi, bid, 2002, 3000));
    check("join B state relays through host to join A",
          waitEvent(ai, bid, 2002, 3000));

    a.stop();
    check("host receives join A departure", waitLeave(hi, aid, 3000));
    check("join B receives join A departure roster edge", waitLeave(bi, aid, 3000));
    check("join B remains connected", b.isRunning());

    queueMarker(b, bid, 2003);
    check("surviving join still reaches host after peer departure",
          waitEvent(hi, bid, 2003, 3000));
    queueMarker(host, 0, 2004);
    check("host still reaches surviving join after peer departure",
          waitEvent(bi, 0, 2004, 3000));

    b.stop();
    host.stop();
}
} // namespace

int main() {
    char logPath[MAX_PATH];
    GetTempPathA(MAX_PATH, logPath);
    std::strcat(logPath, "kenshicoop_sessiontest.log");
    logInit(logPath, "TEST");

    const int basePort = 28000 + (int)(GetCurrentProcessId() % 1000);
    testTwoPlayer(basePort);
    testThreePlayer(basePort + 1);

    logClose();
    std::printf("\nsessiontest: %d/%d checks passed%s\n",
                g_checks - g_failed, g_checks, g_failed ? " - FAIL" : " - PASS");
    return g_failed ? 1 : 0;
}
