#pragma once

#include "gc_shared.h"

// TF2's ServerGC equivalent: answers a TF2 game server's (including our own
// listen server's, for offline/practice play) generic GCSDK handshake
// (k_EMsgGCServerHello -> k_EMsgGCServerWelcome), asks the local player's
// ClientGCTF2 to push its equipped-item SO cache to the server, and
// validates/relays the resulting SO cache messages that arrive over P2P
// (NetworkingServerTF2) up to the game server's own GCSDK client (client.dll)
// so it actually applies the loadout in-game and on the loadout screen.
//
// This is what was missing even after backpack display + client-side equip
// worked: TF2's client.dll pops "WARNING! The server you are playing on has
// lost connection to the item server" (TF_ServerNoSteamConn_Explanation) and
// silently falls back to default items because there was no server-side GC
// connection at all to answer it -- see docs/tf2_live_hook.md.
class ServerGCTF2 final : public SharedGC
{
public:
    ServerGCTF2();
    ~ServerGCTF2();

    bool RoundMVPMusicKitCountForUserId(int, int &) const { return false; }

private:
    void HandleEvent(GCEvent type, uint64_t id, const std::vector<uint8_t> &buffer) override;

    void HandleMessage(uint32_t type, const void *data, uint32_t size);
    void HandleNetMessage(uint64_t steamId, const void *data, uint32_t size);
    void HandleClientSOCacheUnsubscribe(uint64_t steamId);

    void SendServerWelcome();

    bool m_sentWelcome{};
};
