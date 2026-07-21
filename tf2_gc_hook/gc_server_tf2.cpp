#include "stdafx.h"
#include "gc_server_tf2.h"
#include "gc_const_tf2.h"
#include "config.h"
#include "gc_const.h"
#include "networking_shared.h"
#include "platform.h"

ServerGCTF2::ServerGCTF2()
{
    StartThread();

    Platform::Print("ServerGCTF2 spawned\n");
}

ServerGCTF2::~ServerGCTF2()
{
    StopThread();
    Platform::Print("ServerGCTF2 destroyed\n");
}

void ServerGCTF2::HandleEvent(GCEvent type, uint64_t id, const std::vector<uint8_t> &buffer)
{
    switch (type)
    {
    case GCEvent::Message:
        HandleMessage(static_cast<uint32_t>(id), buffer.data(), static_cast<uint32_t>(buffer.size()));
        break;

    case GCEvent::NetMessage:
        HandleNetMessage(id, buffer.data(), static_cast<uint32_t>(buffer.size()));
        break;

    case GCEvent::ClientSOCacheUnsubscribe:
        HandleClientSOCacheUnsubscribe(id);
        break;

    default:
        // LocalPlayerRoundMVP, SyncLocalPlayerMusicKitState, SOCacheRequest,
        // ReloadInventory, RoundEnd are client-side-only events; steam_hook.cpp
        // never posts them to the server gc.
        break;
    }
}

void ServerGCTF2::HandleMessage(uint32_t type, const void *data, uint32_t size)
{
    GCMessageRead messageRead{ type, data, size };
    if (!messageRead.IsValid())
    {
        assert(false);
        return;
    }

    if (!messageRead.IsProtobuf())
    {
        return;
    }

    switch (messageRead.TypeUnmasked())
    {
    case k_EMsgGCServerHello:
        Platform::Print("ServerGCTF2: received k_EMsgGCServerHello, sending welcome + SO cache request\n");
        SendServerWelcome();
        // Ask the local player's ClientGCTF2 to push its equipped-item SO
        // cache to this server. TF2's real GC also relies on per-client
        // validate messages for other players, but for our single local
        // player this proactive push (same as CS2's ServerGC handling) is
        // enough to get our own loadout applied.
        PostToHost(HostEvent::SOCacheRequest, 0, nullptr, 0);
        break;

    default:
        Platform::Print("ServerGCTF2::HandleMessage: unhandled protobuf message %s\n",
            MessageName(messageRead.TypeUnmasked()));
        break;
    }
}

void ServerGCTF2::HandleClientSOCacheUnsubscribe(uint64_t steamId)
{
    Platform::Print("ServerGCTF2::HandleClientSOCacheUnsubscribe: %llu\n", steamId);

    CMsgSOCacheUnsubscribed message;
    message.mutable_owner_soid()->set_type(SoIdTypeSteamId);
    message.mutable_owner_soid()->set_id(steamId);

    GCMessageWrite write{ k_ESOMsg_CacheUnsubscribed, message };
    PostToHost(HostEvent::Message, write.TypeMasked(), write.Data(), write.Size());
}

template<typename T>
static bool ValidateMessageOwnerSOIDTF2(GCMessageRead &messageRead, uint64_t steamId, std::optional<GCMessageWrite> &)
{
    T message;
    if (!messageRead.ReadProtobuf(message))
    {
        Platform::Print("ValidateMessageOwnerSOIDTF2 %llu: parsing failed\n", steamId);
        return false;
    }

    if (message.owner_soid().type() != SoIdTypeSteamId
        || message.owner_soid().id() != steamId)
    {
        Platform::Print("ValidateMessageOwnerSOIDTF2 %llu: steam id mismatch (message has %llu)\n",
            steamId, message.owner_soid().id());
        return false;
    }

    return true;
}

static int MaxServerSOCacheItemsTF2() { return GetConfig().MaxServerItems(); }

static bool RemoveUnequippedItemsTF2(CMsgSOCacheSubscribed &message, int &itemCount)
{
    bool modified = false;

    for (auto it = message.mutable_objects()->begin(); it != message.mutable_objects()->end(); it++)
    {
        if (it->type_id() != SOTypeItemTF2)
        {
            continue;
        }

        for (auto obj = it->mutable_object_data()->begin(); obj != it->mutable_object_data()->end(); )
        {
            CSOEconItem item;
            if (!item.ParseFromString(*obj) || !item.equipped_state_size())
            {
                obj = it->mutable_object_data()->erase(obj);
                modified = true;
            }
            else
            {
                obj++;
                itemCount++;
            }
        }
    }

    return modified;
}

template<>
bool ValidateMessageOwnerSOIDTF2<CMsgSOCacheSubscribed>(GCMessageRead &messageRead, uint64_t steamId, std::optional<GCMessageWrite> &sanitized)
{
    CMsgSOCacheSubscribed message;
    if (!messageRead.ReadProtobuf(message))
    {
        Platform::Print("ValidateMessageOwnerSOIDTF2 %llu: parsing failed\n", steamId);
        return false;
    }

    if (message.owner_soid().type() != SoIdTypeSteamId
        || message.owner_soid().id() != steamId)
    {
        Platform::Print("ValidateMessageOwnerSOIDTF2 %llu: steam id mismatch (message has %llu)\n",
            steamId, message.owner_soid().id());
        return false;
    }

    size_t oldSize = message.ByteSizeLong();

    int itemCount = 0;
    bool modified = RemoveUnequippedItemsTF2(message, itemCount);

    if (itemCount > MaxServerSOCacheItemsTF2())
    {
        Platform::Print("Client %llu socache has %d items (max allowed %d), ignoring\n",
            steamId, itemCount, MaxServerSOCacheItemsTF2());
        return false;
    }

    if (modified)
    {
        Platform::Print("SOCache from %llu had to be cleaned up (%zu -> %zu bytes)\n", steamId, oldSize, message.ByteSizeLong());
        sanitized.emplace(k_ESOMsg_CacheSubscribed, message);
    }

    return true;
}

void ServerGCTF2::HandleNetMessage(uint64_t steamId, const void *data, uint32_t size)
{
    Platform::Print("ServerGCTF2::HandleNetMessage: %llu, %u bytes\n", steamId, size);

    GCMessageRead validate{ 0, data, size };
    if (!validate.IsValid())
    {
        assert(false);
        return;
    }

    if (!validate.IsProtobuf())
    {
        Platform::Print("ServerGCTF2: ignoring non protobuf message %u from %llu\n",
            validate.TypeUnmasked(), steamId);
        return;
    }

    // validate the type and contents
    bool isValid = false;
    std::optional<GCMessageWrite> sanitized;

    switch (validate.TypeUnmasked())
    {
    case k_ESOMsg_Create:
    case k_ESOMsg_Update:
    case k_ESOMsg_Destroy:
        isValid = ValidateMessageOwnerSOIDTF2<CMsgSOSingleObject>(validate, steamId, sanitized);
        break;

    case k_ESOMsg_CacheSubscribed:
        isValid = ValidateMessageOwnerSOIDTF2<CMsgSOCacheSubscribed>(validate, steamId, sanitized);
        break;

    case k_ESOMsg_UpdateMultiple:
        isValid = ValidateMessageOwnerSOIDTF2<CMsgSOMultipleObjects>(validate, steamId, sanitized);
        break;

    case k_EMsgGCItemAcknowledged:
        isValid = true;
        break;
    }

    if (!isValid)
    {
        Platform::Print("ServerGCTF2: ignoring net message %u from %llu\n",
            validate.TypeUnmasked(), steamId);
        return;
    }

    if (!m_sentWelcome)
    {
        // FIXME: ideally this would be sent on steam logon instead, but we
        // don't have confirmation TF2's server always sends
        // k_EMsgGCServerHello promptly, so fall back to sending it here too
        // (same fallback csgo_gc/gc_server.cpp uses for CS2).
        Platform::Print("ServerGCTF2: sending server welcome due to net message\n");
        SendServerWelcome();
    }

    if (sanitized.has_value())
    {
        // pass the sanitized message
        PostToHost(HostEvent::Message, sanitized->TypeMasked(), sanitized->Data(), sanitized->Size());
    }
    else
    {
        // otherwise the old message was fine
        PostToHost(HostEvent::Message, validate.TypeMasked(), data, size);
    }
}

void ServerGCTF2::SendServerWelcome()
{
    // Unlike CS:GO's SendServerWelcome, TF2 doesn't need any game-specific
    // payload in game_data for our minimal goal (getting the player's
    // loadout applied) -- we just need the game server's GCSDK client to see
    // a welcome so it considers the GC connection established.
    CMsgClientWelcome welcome;
    welcome.set_version(0);
    welcome.set_rtime32_gc_welcome_timestamp(static_cast<uint32_t>(time(nullptr)));

    GCMessageWrite write{ k_EMsgGCServerWelcome, welcome };
    PostToHost(HostEvent::Message, write.TypeMasked(), write.Data(), write.Size());

    Platform::Print("ServerGCTF2: sent k_EMsgGCServerWelcome\n");
    m_sentWelcome = true;
}
