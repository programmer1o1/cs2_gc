#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "networking_shared.h"

// Real P2P relay for TF2, mirroring csgo_gc/networking_client.h's
// NetworkingClient. Sends the local player's equipped-item SO cache to
// whatever TF2 game server (including our own listen server) we're
// connected to, over ISteamNetworkingMessages, same as CS:GO. See
// docs/tf2_live_hook.md for why this was previously a no-op and why it
// needs to be real: without it, the game server (even offline/local) never
// receives the player's loadout, so equipped items never show up in-game.
class ClientGCTF2;
class GCMessageRead;
class ISteamNetworkingMessages;

struct AuthTicketTF2
{
    uint64_t steamId{}; // gameserver
    std::vector<uint8_t> buffer;
};

class NetworkingClientTF2
{
public:
    explicit NetworkingClientTF2(ISteamNetworkingMessages *networkingMessages);

    void Update(ClientGCTF2 *gc);

    void SendMessage(const void *data, uint32_t size);

    // for gameserver validation
    void SetAuthTicket(uint32_t handle, const void *data, uint32_t size);
    void ClearAuthTicket(uint32_t handle);

private:
    // return false if it wasn't handled, in which case we pass it to the gc
    bool HandleMessage(ClientGCTF2 *gc, uint64_t steamId, GCMessageRead &message);

    ISteamNetworkingMessages *const m_networkingMessages;
    uint64_t m_serverSteamId{};

    std::unordered_map<uint32_t, AuthTicketTF2> m_tickets;

    STEAM_CALLBACK(NetworkingClientTF2,
        OnSessionRequest,
        SteamNetworkingMessagesSessionRequest_t,
        m_sessionRequest);

    STEAM_CALLBACK(NetworkingClientTF2,
        OnSessionFailed,
        SteamNetworkingMessagesSessionFailed_t,
        m_sessionFailed);
};
