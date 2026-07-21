#pragma once

#include <cstdint>
#include <unordered_set>

#include "networking_shared.h"

// Real P2P relay for TF2, mirroring csgo_gc/networking_server.h's
// NetworkingServer. Tracks which clients (players) have an active GC P2P
// session with our (listen/dedicated) TF2 game server and relays their
// equipped-item SO cache messages up to ServerGCTF2. Game-agnostic --
// doesn't reference ClientGCTF2/ServerGCTF2 at all, same as the CS:GO
// original.
class GCMessageWrite;

// this wrapper type is useless now, but i can't be bothered to inline it
class ClientSetTF2
{
public:
    bool Add(uint64_t id)
    {
        auto [it, inserted] = m_set.insert(id);
        return inserted;
    }

    bool Remove(uint64_t id)
    {
        return (m_set.erase(id) == 1);
    }

    bool Has(uint64_t id)
    {
        return (m_set.find(id) != m_set.end());
    }

private:
    std::unordered_set<uint64_t> m_set;
};

class NetworkingServerTF2
{
public:
    NetworkingServerTF2(ISteamNetworkingMessages *networkingMessages);

    // caller needs to call message->Release()
    bool ReceiveMessage(SteamNetworkingMessage_t *&message);

    void ClientConnected(uint64_t steamId, const void *ticket, uint32_t ticketSize);
    void ClientDisconnected(uint64_t steamId);

    void SendMessage(uint64_t steamId, const void *data, uint32_t size);

private:
    // NOTE: deliberately NOT caching the ISteamNetworkingMessages interface, same reasoning
    // as csgo_gc/networking_server.h (the interface is freed on gameserver restart/map change).
    static ISteamNetworkingMessages *Messages() { return SteamGameServerNetworkingMessages(); }

    ClientSetTF2 m_clients;

    STEAM_GAMESERVER_CALLBACK(NetworkingServerTF2,
        OnSessionRequest,
        SteamNetworkingMessagesSessionRequest_t,
        m_sessionRequest);

    STEAM_GAMESERVER_CALLBACK(NetworkingServerTF2,
        OnSessionFailed,
        SteamNetworkingMessagesSessionFailed_t,
        m_sessionFailed);
};
