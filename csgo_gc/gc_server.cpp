#include "stdafx.h"
#include "gc_server.h"
#include "gc_const.h"
#include "gc_const_csgo.h"
#include "graffiti.h"

// yuck!! needed for CSteamID (construct full id from account id)
#include "steam/steamclientpublic.h"

ServerGC::ServerGC()
{
    // also called from ClientGC's constructor
    Graffiti::Initialize();

    StartThread();

    Platform::Print("ServerGC spawned\n");
}

ServerGC::~ServerGC()
{
    StopThread();
    Platform::Print("ServerGC destroyed\n");
}

void ServerGC::HandleEvent(GCEvent type, uint64_t id, const std::vector<uint8_t> &buffer)
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
        assert(false);
        break;
    }
}

void ServerGC::HandleMessage(uint32_t type, const void *data, uint32_t size)
{
    GCMessageRead messageRead{ type, data, size };
    if (!messageRead.IsValid())
    {
        assert(false);
        return;
    }

    if (messageRead.IsProtobuf())
    {
        switch (messageRead.TypeUnmasked())
        {
        case k_EMsgGCServerHello:
            OnServerHello(messageRead);
            break;

        case k_EMsgGCCStrike15_v2_Server2GCClientValidate:
            // server doesn't want a response so ignore
            break;

        case k_EMsgGC_IncrementKillCountAttribute:
            IncrementKillCountAttribute(messageRead);
            break;
        case k_EMsgGCCStrike15_v2_MatchmakingServerReservationResponse:
            OnMatchmakingServerReservationResponse(messageRead);
            break;
            
        default:
            Platform::Print("ServerGC::HandleMessage: unhandled protobuf message %s)\n",
                MessageName(messageRead.TypeUnmasked()));
            break;
        }
    }
}

void ServerGC::HandleClientSOCacheUnsubscribe(uint64_t steamId)
{
    Platform::Print("HandleClientSOCacheUnsubscribe: %llu\n", steamId);

    CMsgSOCacheUnsubscribed message;
    message.mutable_owner_soid()->set_type(SoIdTypeSteamId);
    message.mutable_owner_soid()->set_id(steamId);

    GCMessageWrite write{ k_ESOMsg_CacheUnsubscribed, message };
    PostToHost(HostEvent::Message, write.TypeMasked(), write.Data(), write.Size());
}

template<typename T>
static bool ValidateMessageOwnerSOID(GCMessageRead &messageRead, uint64_t steamId)
{
    T message;
    if (!messageRead.ReadProtobuf(message))
    {
        Platform::Print("ValidateMessageOwnerSOID %llu: parsing failed\n", steamId);
        return false;
    }

    if (message.owner_soid().type() != SoIdTypeSteamId
        || message.owner_soid().id() != steamId)
    {
        Platform::Print("ValidateMessageOwnerSOID %llu: steam id mismatch (message has %llu)\n",
            steamId, message.owner_soid().id());
        return false;
    }

    return true;
}


void ServerGC::SendConfirmToClient(uint64_t clientId, const PendingReservation& res)
{
    CMsgGCCStrike15_v2_MatchmakingGC2ServerConfirm confirm;
    confirm.set_token(res.token);
    confirm.set_stamp(res.stamp);
    confirm.set_exchange(res.exchange);

    GCMessageWrite write{ 9114, confirm };   // k_EMsgGCCStrike15_v2_MatchmakingGC2ServerConfirm
    PostToHost(HostEvent::NetMessage, clientId, write.Data(), write.Size());
    Platform::Print("ServerGC: sent confirm to client %llu (res %llu)\n", clientId, res.exchange);
}

void ServerGC::CheckPendingReservations()
{
    if (m_pendingReservations.empty())
        return;

    auto it = m_pendingReservations.begin();
    while (it != m_pendingReservations.end())
    {
        uint64_t clientId = it->first;
        if (m_networking.HasClient(clientId))
        {
            SendConfirmToClient(clientId, it->second);
            it = m_pendingReservations.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void ServerGC::OnMatchmakingServerReservationResponse(GCMessageRead &messageRead)
{
    CMsgGCCStrike15_v2_MatchmakingServerReservationResponse response;
    if (!messageRead.ReadProtobuf(response))
    {
        Platform::Print("Failed to parse MatchmakingServerReservationResponse\n");
        return;
    }

    // Extract account ID from the response.
    // Adjust the field name based on your protobuf definition.
    uint32_t accountId = response.account_id();   // or response.steam_id()
    if (accountId == 0)
    {
        Platform::Print("ServerGC: reservation response missing account_id\n");
        return;
    }

    // Construct full Steam ID (account type Individual, universe Public).
    uint64_t clientId = CSteamID(accountId, k_EUniversePublic, k_EAccountTypeIndividual).ConvertToUint64();

    Platform::Print("ServerGC: received reservation response for res %llu, map %s, client %llu\n",
                    response.reservationid(), response.map().c_str(), clientId);

    if (!m_networking.HasClient(clientId))
    {
        PendingReservation pending;
        pending.exchange = response.reservationid();
        pending.token = 0x12345678;
        pending.stamp = static_cast<uint32_t>(time(nullptr));
        m_pendingReservations[clientId] = pending;
        Platform::Print("ServerGC: pending reservation for client %llu\n", clientId);
        return;
    }

    SendConfirmToClient(clientId, { response.reservationid(), 0x12345678, static_cast<uint32_t>(time(nullptr)) });
}

void ServerGC::HandleNetMessage(uint64_t steamId, const void *data, uint32_t size)
{
    assert(CanHandleNetMessages());

    GCMessageRead validate{ 0, data, size };
    if (!validate.IsValid())
    {
        assert(false);
        return;
    }

    if (!validate.IsProtobuf())
    {
        // all the allowed messages are protobuf based
        Platform::Print("ServerGC: ignoring non protobuf message %u from %llu\n",
            validate.TypeUnmasked(), steamId);
        return;
    }

    // validate the type and contents
    bool isValid = false;

    switch (validate.TypeUnmasked())
    {
    case k_ESOMsg_Create:
    case k_ESOMsg_Update:
    case k_ESOMsg_Destroy:
        isValid = ValidateMessageOwnerSOID<CMsgSOSingleObject>(validate, steamId);
        break;

    case k_ESOMsg_CacheSubscribed:
        isValid = ValidateMessageOwnerSOID<CMsgSOCacheSubscribed>(validate, steamId);
        break;

    case k_ESOMsg_UpdateMultiple:
        isValid = ValidateMessageOwnerSOID<CMsgSOMultipleObjects>(validate, steamId);
        break;

    case k_EMsgGCItemAcknowledged:
        isValid = true;
        break;
    }

    if (!isValid)
    {
        Platform::Print("ServerGC: ignoring net message %u from %llu\n",
            validate.TypeUnmasked(), steamId);
        return;
    }

    PostToHost(HostEvent::Message, validate.TypeMasked(), data, size);
}

void ServerGC::OnServerHello(GCMessageRead &messageRead)
{
    CMsgServerHello hello;
    if (!messageRead.ReadProtobuf(hello))
    {
        Platform::Print("Parsing CMsgServerHello failed, ignoring\n");
        return;
    }

    // we don't care about anything in this message, just reply

    CMsgCStrike15Welcome csWelcome;
    csWelcome.set_gscookieid(GameServerCookieId);

    CMsgClientWelcome welcome;
    welcome.set_version(0);
    welcome.set_game_data(csWelcome.SerializeAsString());
    welcome.set_rtime32_gc_welcome_timestamp(static_cast<uint32_t>(time(nullptr)));

    GCMessageWrite write{ k_EMsgGCServerWelcome, welcome };
    PostToHost(HostEvent::Message, write.TypeMasked(), write.Data(), write.Size());

    m_receivedHello.store(true, std::memory_order_release);
}

void ServerGC::IncrementKillCountAttribute(GCMessageRead &messageRead)
{
    CMsgIncrementKillCountAttribute message;
    if (!messageRead.ReadProtobuf(message))
    {
        Platform::Print("Parsing CMsgIncrementKillCountAttribute failed, ignoring\n");
        return;
    }

    // just forward it to the killer
    GCMessageWrite messageWrite{ k_EMsgGC_IncrementKillCountAttribute, message };
    CSteamID killerId{ message.killer_account_id(), k_EUniversePublic, k_EAccountTypeIndividual };
    PostToHost(HostEvent::NetMessage, killerId.ConvertToUint64(), messageWrite.Data(), messageWrite.Size());
}
