#include "stdafx.h"
#include "networking_server_tf2.h"
#include "gc_message.h"
#include "platform.h"

NetworkingServerTF2::NetworkingServerTF2([[maybe_unused]] ISteamNetworkingMessages *networkingMessages)
    : m_sessionRequest{ this, &NetworkingServerTF2::OnSessionRequest }
    , m_sessionFailed{ this, &NetworkingServerTF2::OnSessionFailed }
{
}

bool NetworkingServerTF2::ReceiveMessage(SteamNetworkingMessage_t *&message)
{
    ISteamNetworkingMessages *messages = Messages();
    if (!messages || !messages->ReceiveMessagesOnChannel(NetMessageChannel, &message, 1))
    {
        return false;
    }

    uint64_t steamId = message->m_identityPeer.GetSteamID64();

    // see if we have a session
    if (!m_clients.Has(steamId))
    {
        Platform::Print("NetworkingServerTF2: ignored message from %llu (no session)\n", steamId);
        message->Release();
        return false;
    }

    return true;
}

// helper for SteamNetworkingMessages::SendMessageToUser that attempts to do some kind of error handling
static void SendMessageToUserTF2(ISteamNetworkingMessages *networkingMessages, uint64_t steamId, const void *data, uint32_t size)
{
    if (!networkingMessages)
    {
        return;
    }

    SteamNetworkingIdentity identity;
    identity.SetSteamID64(steamId);

    EResult result = networkingMessages->SendMessageToUser(
        identity,
        data,
        size,
        NetMessageSendFlags,
        NetMessageChannel);

    if (result != k_EResultOK)
    {
        Platform::Print("SendMessageToUserTF2 failed for %llu: %d, closing session and trying again\n", steamId, result);

        networkingMessages->CloseChannelWithUser(identity, NetMessageChannel);

        result = networkingMessages->SendMessageToUser(
            identity,
            data,
            size,
            NetMessageSendFlags,
            NetMessageChannel);

        if (result != k_EResultOK)
        {
            // not much we can do in this situation i guess
            Platform::Print("SendMessageToUserTF2 failed for %llu\n", steamId);
        }
    }
}

void NetworkingServerTF2::ClientConnected(uint64_t steamId, const void *ticket, uint32_t ticketSize)
{
    if (!m_clients.Add(steamId))
    {
        Platform::Print("got ClientConnected for %llu but they're already on the list! ignoring\n", steamId);
        return;
    }

    // send a message, if the client has tf2_gc installed they will
    // reply with their so cache and we'll add them to our list
    GCMessageWrite messageWrite{ k_EMsgNetworkConnect };
    messageWrite.WriteUint32(ticketSize);
    messageWrite.WriteData(ticket, ticketSize);

    // FIXME: this gets sent when the client is connecting to the server, it's not uncommon for
    // the connection to time out, in which case the player's socache never gets to the server
    SendMessageToUserTF2(Messages(), steamId, messageWrite.Data(), messageWrite.Size());
}

void NetworkingServerTF2::ClientDisconnected(uint64_t steamId)
{
    if (!m_clients.Remove(steamId))
    {
        Platform::Print("got ClientDisconnected for %llu but they're not on the list! ignoring\n", steamId);
        return;
    }

    SteamNetworkingIdentity identity;
    identity.SetSteamID64(steamId);
    if (ISteamNetworkingMessages *messages = Messages())
    {
        messages->CloseChannelWithUser(identity, NetMessageChannel);
    }
}

void NetworkingServerTF2::SendMessage(uint64_t steamId, const void *data, uint32_t size)
{
    if (!m_clients.Has(steamId))
    {
        Platform::Print("No tf2_gc session with %llu, not sending message!!!\n", steamId);
        return;
    }

    SendMessageToUserTF2(Messages(), steamId, data, size);
}

void NetworkingServerTF2::OnSessionRequest(SteamNetworkingMessagesSessionRequest_t *param)
{
    uint64_t steamId = param->m_identityRemote.GetSteamID64();

    if (!m_clients.Has(steamId))
    {
        Platform::Print("%llu sent a session request, we don't have a tf2_gc session, ignoring...\n", steamId);
        return;
    }

    Platform::Print("%llu sent a session request, we were playing GC with them so accept\n", steamId);

    ISteamNetworkingMessages *messages = Messages();
    if (!messages || !messages->AcceptSessionWithUser(param->m_identityRemote))
    {
        Platform::Print("AcceptSessionWithUser with %llu failed???\n",
            param->m_identityRemote.GetSteamID64());
    }
}

void NetworkingServerTF2::OnSessionFailed(SteamNetworkingMessagesSessionFailed_t *param)
{
    // don't do anything, rely on the auth session
    Platform::Print("NetworkingServerTF2::OnSessionFailed: %s\n", param->m_info.m_szEndDebug);
}
