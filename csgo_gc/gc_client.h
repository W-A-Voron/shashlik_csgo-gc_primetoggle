#pragma once

#include "config.h"
#include "gc_shared.h"
#include "inventory.h"
#include "keyvalue.h"

#include <steam/isteamhttp.h>
#include <steam/steam_api_common.h>

class ClientGC final : public SharedGC
{
public:
    ClientGC(uint64_t steamId);
    ~ClientGC();
    void CheckFileReloads();

    uint64_t GetSteamId() const { return m_steamId; }

    void OnOverwatchHTTPResponse(HTTPRequestCompleted_t *pCallback);
    void OnOverwatchCaseStatus(GCMessageRead &messageRead);
    void OnOverwatchCaseUpdate(GCMessageRead &messageRead);

private:
    KeyValue m_priceSheet;
    KeyValue m_passes;
    KeyValue m_unusualLootLists;

    void HandleEvent(GCEvent type, uint64_t id, const std::vector<uint8_t> &buffer) override;
    bool m_isSearching{ false };
    
    void HandleMessage(uint32_t type, const void *data, uint32_t size);
    void HandleNetMessage(const void *data, uint32_t size);
    void HandleSOCacheRequest();

    void SendMessageToGame(bool sendToGameServer, uint32_t type,
        const google::protobuf::MessageLite &message, uint64_t jobId = JobIdInvalid);

    void OnClientHello(GCMessageRead &messageRead);
    void AdjustItemEquippedState(GCMessageRead &messageRead);
    void ClientPlayerDecalSign(GCMessageRead &messageRead);
    void UseItemRequest(GCMessageRead &messageRead);
    void ClientRequestJoinServerData(GCMessageRead &messageRead);
    void SetItemPositions(GCMessageRead &messageRead);
    void IncrementKillCountAttribute(GCMessageRead &messageRead);
    void ApplySticker(GCMessageRead &messageRead);
    void StoreGetUserData(GCMessageRead &messageRead);
    void StorePurchaseInit(GCMessageRead &messageRead);
    void StorePurchaseFinalize(GCMessageRead &messageRead);
    void PartySearch(GCMessageRead &messageRead);
    void RequestCoPlays(GCMessageRead &messageRead);
    void ClientRequestPlayersProfile(GCMessageRead &messageRead);
    void CasketItemLoadContents(GCMessageRead &messageRead);
    void CasketItemAdd(GCMessageRead &messageRead);
    void CasketItemExtract(GCMessageRead &messageRead);
    void StatTrakSwap(GCMessageRead &messageRead);

    void DeleteItem(GCMessageRead &messageRead);
    void UnlockCrate(GCMessageRead &messageRead);
    void NameItem(GCMessageRead &messageRead);
    void NameBaseItem(GCMessageRead &messageRead);
    void RemoveItemName(GCMessageRead &messageRead);

    void BuildMatchmakingHello(CMsgGCCStrike15_v2_MatchmakingGC2ClientHello &message);
    void BuildClientWelcome(CMsgClientWelcome &message, const CMsgCStrike15Welcome &csWelcome,
        const CMsgGCCStrike15_v2_MatchmakingGC2ClientHello &matchmakingHello);
    
    const uint64_t m_steamId;
    void ProcessGiftUse(uint64_t giftId);
    Inventory m_inventory;

    uint64_t m_transactionId{};
    std::vector<uint64_t> m_transactionItemIds;

    void SendInventoryUpdate();
    void ReloadInventory();
    void ReloadConfig();
    void ReloadPriceSheet();
    void ReloadPasses();
    void ReloadUnusualLootLists();

    std::vector<uint32_t> m_overwatchSuspects;
    size_t m_nextOverwatchIndex = 0;
    uint64_t m_nextCaseId = 1;
    std::mutex m_overwatchMutex;

    void FetchOverwatchCases();
    void SendOverwatchCaseAssignment(uint32_t suspectAccountId);
    void SendVerdictToCloudflare(const CMsgGCCStrike15_v2_PlayerOverwatchCaseUpdate &msg);

    static uint32_t SteamIDStringToAccountId(const std::string& str);

    CCallback<ClientGC, HTTPRequestCompleted_t, false> m_httpCallback;
    void SendMatchmakingHelloUpdate();
    uint32_t AccountId() const { return m_steamId & 0xffffffff; }
    uint32_t EffectiveAccountId() const { return AccountId(); }
    std::chrono::steady_clock::time_point m_matchmakingStartTime;
    bool m_matchmakingReservationSent = false;
    uint64_t m_matchmakingReservationId = 0;
    void SendMatchmakingReservation();
    bool m_isCooldownActive{ false };
    std::chrono::steady_clock::time_point m_cooldownEndTime;
    void SendCompetitiveCooldown();
    void UpdateCooldown();
};
