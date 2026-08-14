#include "stdafx.h"
#include "gc_client.h"
#include "graffiti.h"
#include "keyvalue.h"
#include <filesystem>
#include "case_opening.h"
#include <steam/isteamhttp.h>
#include "steam_hook.h"   // for RecreateClientGC()

// ------------------------------------------------
// Встроенные данные (заменяют config.txt и price_sheet.txt)
// ------------------------------------------------

// Реалистичные ранги (средний уровень)
constexpr int CONFIG_COMPETITIVE_RANK = 10;   // Gold Nova Master (примерно 10-й ранг)
constexpr int CONFIG_COMPETITIVE_WINS = 150;  // нормальное количество побед
constexpr int CONFIG_WINGMAN_RANK = 10;
constexpr int CONFIG_WINGMAN_WINS = 50;
constexpr int CONFIG_DANGERZONE_RANK = 10;
constexpr int CONFIG_DANGERZONE_WINS = 30;

constexpr bool CONFIG_DESTROY_USED_ITEMS = true; // предметы должны исчезать после использования

// Реалистичные награды (не миллиарды)
constexpr int CONFIG_COMMENDED_FRIENDLY = 250;
constexpr int CONFIG_COMMENDED_TEACHING = 120;
constexpr int CONFIG_COMMENDED_LEADER = 80;
constexpr int CONFIG_PLAYER_LEVEL = 25;
constexpr int CONFIG_PLAYER_XP = 3500;

// Веса редкости (реальные шансы, как у Valve)
// Сумма всех весов = 10000000 (как в оригинале)
static const std::vector<RarityWeight> CONFIG_RARITY_WEIGHTS = {
    { ItemSchema::RarityCommon,   10000000 },
    { ItemSchema::RarityUncommon, 2000000 },
    { ItemSchema::RarityRare,     400000 },
    { ItemSchema::RarityMythical, 80000 },
    { ItemSchema::RarityLegendary,16000 },
    { ItemSchema::RarityAncient,  3200 },
    { ItemSchema::RarityUnusual,  1280 },
};

// Цены (в рублях) – максимально приближены к реальным рыночным ценам на 2026 год
// Ключи, кейсы, наборы
static const std::unordered_map<uint32_t, int> ITEM_PRICES = {
    // Ключи
    { 5000, 140 },  // Weapon Case Key
    { 5010, 140 },  // E-Sports Case Key
    { 5015, 140 },  // Community Case Key
    { 5020, 140 },  // Falchion Case Key
    { 5025, 140 },  // Gamma Case Key
    { 5030, 140 },  // Chroma Case Key
    { 5035, 140 },  // Revolver Case Key
    { 5040, 140 },  // Spectrum Case Key
    { 5045, 140 },  // Horizon Case Key
    { 5050, 140 },  // Danger Zone Case Key
    { 5055, 140 },  // Prisma Case Key
    { 5060, 140 },  // Fracture Case Key
    { 5065, 140 },  // Operation Case Key (общие)
    { 5070, 140 },  // Riptide Case Key
    { 5075, 140 },  // Snakebite Case Key
    { 5080, 140 },  // Dreams & Nightmares Case Key
    { 5085, 140 },  // Recoil Case Key
    { 5090, 140 },  // CS20 Case Key
    { 5095, 140 },  // Prisma 2 Case Key
    { 5100, 140 },  // Operation Broken Fang Case Key
    // Кейсы
    { 1200, 150 },  // Weapon Case 1
    { 1201, 150 },  // Weapon Case 2
    { 1202, 150 },  // Weapon Case 3
    { 1203, 150 },  // eSports 2013 Case
    { 1204, 150 },  // Winter 2013 Case
    { 1205, 150 },  // Operation Bravo Case
    { 1206, 150 },  // Community Case 1
    { 1207, 150 },  // Community Case 2
    { 1208, 150 },  // Operation Phoenix Case
    { 1209, 150 },  // Operation Breakout Case
    { 1210, 150 },  // Vanguard Case
    { 1211, 150 },  // Bloodhound Case
    { 1212, 150 },  // Wildfire Case
    { 1213, 150 },  // Chroma Case
    { 1214, 150 },  // Chroma 2 Case
    { 1215, 150 },  // Chroma 3 Case
    { 1216, 150 },  // Gamma Case
    { 1217, 150 },  // Gamma 2 Case
    { 1218, 150 },  // Spectrum Case
    { 1219, 150 },  // Spectrum 2 Case
    { 1220, 150 },  // Horizon Case
    { 1221, 150 },  // Danger Zone Case
    { 1222, 150 },  // Prisma Case
    { 1223, 150 },  // Prisma 2 Case
    { 1224, 150 },  // Fracture Case
    { 1225, 150 },  // Shattered Web Case
    { 1226, 150 },  // Broken Fang Case
    { 1227, 150 },  // Operation Riptide Case
    { 1228, 150 },  // Snakebite Case
    { 1229, 150 },  // Dreams & Nightmares Case
    { 1230, 150 },  // Recoil Case
    { 1231, 150 },  // CS20 Case
    // Наборы стикеров (обычные)
    { 2000, 10 },   // Sticker Capsule 1
    { 2001, 15 },   // Sticker Capsule 2
    { 2002, 20 },   // Sticker Capsule 3
    { 2003, 25 },   // Community Sticker Capsule
    { 2004, 30 },   // Team Roles Capsule
    // Премиум-наборы (событийные)
    { 3000, 60 },   // Katowice 2019 Capsule
    { 3001, 55 },   // Berlin 2019 Capsule
    { 3002, 65 },   // Stockholm 2021 Capsule
    { 3003, 70 },   // Antwerp 2022 Capsule
    { 3004, 75 },   // Rio 2022 Capsule
    { 3005, 80 },   // Paris 2023 Capsule
    // Прочее
    { 1300, 200 },  // Music Kit (обычный)
    { 1301, 350 },  // StatTrak Music Kit
    { 1400, 40 },   // Name Tag
    { 1401, 450 },  // StatTrak Swap Tool
    { 1402, 600 },  // Storage Unit
    // Операционные пропуски
    { 4000, 100 },  // Operation Payback Pass
    { 4001, 150 },  // Operation Bravo Pass
    { 4002, 150 },  // Operation Phoenix Pass
    { 4003, 150 },  // Operation Breakout Pass
    { 4004, 150 },  // Operation Vanguard Pass
    { 4005, 150 },  // Operation Bloodhound Pass
    { 4006, 150 },  // Operation Wildfire Pass
    { 4007, 150 },  // Operation Hydra Pass
    { 4008, 150 },  // Operation Shattered Web Pass
    { 4009, 150 },  // Operation Broken Fang Pass
    { 4010, 150 },  // Operation Riptide Pass
    // Турнирные пропуски (зрителя)
    { 4500, 200 },  // Katowice 2019 Viewer Pass
    { 4501, 180 },  // Berlin 2019 Viewer Pass
    { 4502, 220 },  // Stockholm 2021 Viewer Pass
    { 4503, 230 },  // Antwerp 2022 Viewer Pass
    { 4504, 240 },  // Rio 2022 Viewer Pass
    { 4505, 250 },  // Paris 2023 Viewer Pass
    // Пополнения (дропы из кейсов - будут добавлены динамически)
};

// Функция получения цены по defIndex
static int GetItemPrice(uint32_t defIndex) {
    auto it = ITEM_PRICES.find(defIndex);
    if (it != ITEM_PRICES.end()) return it->second;
    return 0; // цена по умолчанию (бесплатно) – для предметов, не в списке
}

// ------------------------------------------------
// ClientGC (основной класс)
// ------------------------------------------------

ClientGC::ClientGC(uint64_t steamId)
    : m_steamId{ steamId }
    , m_inventory{ m_steamId }
    , m_httpCallback(this, &ClientGC::OnOverwatchHTTPResponse)
{
    Graffiti::Initialize();
    StartThread();
    FetchOverwatchCases();

    Platform::Print("ClientGC spawned for user %llu\n", m_steamId);
}

ClientGC::~ClientGC()
{
    StopThread();
    Platform::Print("ClientGC destroyed\n");
}

void ClientGC::HandleEvent(GCEvent type, uint64_t id, const std::vector<uint8_t> &buffer)
{
    switch (type)
    {
    case GCEvent::Message:
        HandleMessage(static_cast<uint32_t>(id), buffer.data(), static_cast<uint32_t>(buffer.size()));
        break;

    case GCEvent::NetMessage:
        HandleNetMessage(buffer.data(), static_cast<uint32_t>(buffer.size()));
        break;

    case GCEvent::SOCacheRequest:
        HandleSOCacheRequest();
        break;

    default:
        assert(false);
        break;
    }
}

void ClientGC::SendCompetitiveCooldown()
{
    uint32_t seconds = 0;
    if (m_isCooldownActive) {
        auto remaining = std::chrono::duration_cast<std::chrono::seconds>(
            m_cooldownEndTime - std::chrono::steady_clock::now()).count();
        seconds = (remaining > 0) ? static_cast<uint32_t>(remaining) : 0;
        if (seconds == 0) {
            m_isCooldownActive = false;
        }
    }

    CMsgGCCStrike15_v2_ServerNotificationForUserPenalty penalty;
    penalty.set_account_id(EffectiveAccountId());
    penalty.set_reason(0); // competitive cooldown
    penalty.set_seconds(seconds);
    penalty.set_communication_cooldown(false);
    SendMessageToGame(false, k_EMsgGCCStrike15_v2_ServerNotificationForUserPenalty, penalty);
}

void ClientGC::OnMatchmakingPing(GCMessageRead &messageRead)
{
    SendMatchmakingUpdate();
}

void ClientGC::SendMatchmakingUpdate()
{
    CMsgGCCStrike15_v2_MatchmakingGC2ClientUpdate update;
    update.set_matchmaking(m_isSearching ? 1 : 0);

    auto* stats = update.mutable_global_stats();
    stats->set_players_searching(1000);
    stats->set_search_time_avg(60);

    auto* detail = stats->add_search_statistics();
    detail->set_game_type(0);
    detail->set_search_time_avg(60);
    detail->set_players_searching(1000);

    SendMessageToGame(false, k_EMsgGCCStrike15_v2_MatchmakingGC2ClientUpdate, update);
}

void ClientGC::OnMatchmakingStart(GCMessageRead &messageRead)
{
    // Если есть кулдаун, блокируем поиск
    // (Здесь можно заменить на чтение из конфига, но мы используем жёстко 0)
    uint32_t cooldown = 0;
    if (cooldown > 0)
    {
        m_isSearching = false;
        m_isCooldownActive = true;
        m_cooldownEndTime = std::chrono::steady_clock::now() + std::chrono::seconds(cooldown);

        SendMatchmakingUpdate();
        SendCompetitiveCooldown();
        Platform::Print("Blocked matchmaking: Cooldown active (%u seconds)\n", cooldown);
        return;
    }

    m_isSearching = true;
    SendMatchmakingUpdate();
}

void ClientGC::OnMatchmakingStop(GCMessageRead &messageRead)
{
    m_isSearching = false;
    SendMatchmakingUpdate();
}

void ClientGC::SendMatchmakingReservation()
{
    CMsgGCCStrike15_v2_MatchmakingGC2ClientReserve reserve;
    reserve.set_serverid(0x12345678);
    reserve.set_direct_udp_ip(0xC0A8000E);
    reserve.set_direct_udp_port(27019);
    reserve.set_reservationid(++m_matchmakingReservationId);
    reserve.set_map("de_dust2");
    reserve.set_server_address("192.168.0.14:27019");
    CMsgGCCStrike15_v2_MatchmakingGC2ServerReserve *sub = reserve.mutable_reservation();

    SendMessageToGame(false, k_EMsgGCCStrike15_v2_MatchmakingGC2ClientReserve, reserve);
    m_matchmakingReservationSent = true;

    Platform::Print("Sent matchmaking reservation to %s:%d\n", "192.168.0.14", 27019);
}

void ClientGC::HandleMessage(uint32_t type, const void *data, uint32_t size)
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
        case k_EMsgGCClientHello:
            OnClientHello(messageRead);
            break;

        case k_EMsgGCAdjustItemEquippedState:
            AdjustItemEquippedState(messageRead);
            break;

        case k_EMsgGCCStrike15_v2_ClientPlayerDecalSign:
            ClientPlayerDecalSign(messageRead);
            break;

        case k_EMsgGCUseItemRequest:
            UseItemRequest(messageRead);
            break;

        case k_EMsgGCCStrike15_v2_ClientRequestJoinServerData:
            ClientRequestJoinServerData(messageRead);
            break;

        case k_EMsgGCSetItemPositions:
            SetItemPositions(messageRead);
            break;

        case k_EMsgGCApplySticker:
            ApplySticker(messageRead);
            break;

        case k_EMsgGCStoreGetUserData:
            StoreGetUserData(messageRead);
            break;

        case k_EMsgGCStorePurchaseInit:
            StorePurchaseInit(messageRead);
            break;

        case k_EMsgGCStorePurchaseFinalize:
            StorePurchaseFinalize(messageRead);
            break;

        case k_EMsgGCCStrike15_v2_Party_Search:
            PartySearch(messageRead);
            break;

        case k_EMsgGCCStrike15_v2_Account_RequestCoPlays:
            RequestCoPlays(messageRead);
            break;

        case k_EMsgGCCStrike15_v2_ClientRequestPlayersProfile:
            ClientRequestPlayersProfile(messageRead);
            break;

        case k_EMsgGCCasketItemLoadContents:
            CasketItemLoadContents(messageRead);
            break;

        case k_EMsgGCCasketItemAdd:
            CasketItemAdd(messageRead);
            break;

        case k_EMsgGCCasketItemExtract:
            CasketItemExtract(messageRead);
            break;

        case k_EMsgGCStatTrakSwap:
            StatTrakSwap(messageRead);
            break;

        case k_EMsgGCCStrike15_v2_MatchmakingClient2ServerPing:
            OnMatchmakingPing(messageRead);
            break;

        case k_EMsgGCCStrike15_v2_MatchmakingStart:
            OnMatchmakingStart(messageRead);
            break;

        case k_EMsgGCCStrike15_v2_MatchmakingStop:
            OnMatchmakingStop(messageRead);
            break;

        case k_EMsgGCCStrike15_v2_PlayerOverwatchCaseStatus:
            OnOverwatchCaseStatus(messageRead);
            break;

        case k_EMsgGCCStrike15_v2_PlayerOverwatchCaseUpdate:
            OnOverwatchCaseUpdate(messageRead);
            break;

        default:
            Platform::Print("ClientGC::HandleMessage: unhandled protobuf message %s\n",
                MessageName(messageRead.TypeUnmasked()));
            break;
        }
    }
    else
    {
        switch (messageRead.TypeUnmasked())
        {
        case k_EMsgGCDelete:
            DeleteItem(messageRead);
            break;

        case k_EMsgGCUnlockCrate:
            UnlockCrate(messageRead);
            break;

        case k_EMsgGCNameItem:
            NameItem(messageRead);
            break;

        case k_EMsgGCNameBaseItem:
            NameBaseItem(messageRead);
            break;

        case k_EMsgGCRemoveItemName:
            RemoveItemName(messageRead);
            break;

        default:
            Platform::Print("ClientGC::HandleMessage: unhandled struct message %s\n",
                MessageName(messageRead.TypeUnmasked()));
            break;
        }
    }
}

void ClientGC::SendMatchmakingHelloUpdate()
{
    CMsgGCCStrike15_v2_MatchmakingGC2ClientHello mmHello;
    BuildMatchmakingHello(mmHello);
    SendMessageToGame(false, k_EMsgGCCStrike15_v2_MatchmakingGC2ClientHello, mmHello);
}

void ClientGC::UpdateCooldown()
{
    if (!m_isCooldownActive)
        return;
    auto now = std::chrono::steady_clock::now();
    if (now >= m_cooldownEndTime) {
        m_isCooldownActive = false;
        SendCompetitiveCooldown();
        Platform::Print("Competitive cooldown expired.\n");
    }
}

void ClientGC::ProcessGiftUse(uint64_t giftId)
{
    const CSOEconItem *giftItem = m_inventory.GetItem(giftId);
    if (!giftItem)
    {
        Platform::Print("Gift item not found\n");
        return;
    }

    uint32_t defIndex = giftItem->def_index();
    int numItems = 1;
    if (defIndex == 1210) numItems = 1;
    else if (defIndex == 1211) numItems = 9;
    else if (defIndex == 1215) numItems = 25;
    else {
        Platform::Print("Unknown gift type\n");
        return;
    }

    uint32_t series = 0;
    uint32_t attrDef = m_inventory.GetItemSchema().GetAttributeDefIndex("set supply crate series");
    if (attrDef)
    {
        for (const auto &attr : giftItem->attribute())
        {
            if (attr.def_index() == attrDef)
            {
                series = m_inventory.GetItemSchema().AttributeUint32(&attr);
                break;
            }
        }
    }

    if (series == 0)
    {
        Platform::Print("Gift has no supply crate series\n");
        return;
    }

    const LootList *lootList = m_inventory.GetItemSchema().GetLootListBySeries(series);
    if (!lootList)
    {
        Platform::Print("No loot list for series %u\n", series);
        return;
    }

    CMsgSOMultipleObjects updateMultiple;
    CMsgSOSingleObject destroy;
    CMsgGCItemCustomizationNotification notification;
    notification.set_request(k_EGCItemCustomizationNotification_Gift);

    CaseOpening caseOpening(m_inventory.GetItemSchema(), m_inventory.GetRandom());

    for (int i = 0; i < numItems; i++)
    {
        CSOEconItem newItemProto;
        if (!caseOpening.SelectItemFromLootList(*lootList, newItemProto))
        {
            Platform::Print("Failed to select item from loot list\n");
            continue;
        }

        CSOEconItem &createdItem = m_inventory.CreateItem(newItemProto);
        m_inventory.AddToMultipleObjects(updateMultiple, SOTypeItem, createdItem);
        notification.add_item_id(createdItem.id());
    }

    if (CONFIG_DESTROY_USED_ITEMS)
    {
        m_inventory.RemoveItem(giftId, destroy);
    }

    if (updateMultiple.objects_modified_size() > 0)
    {
        SendMessageToGame(true, k_ESOMsg_UpdateMultiple, updateMultiple);
        SendMessageToGame(false, k_ESOMsg_UpdateMultiple, updateMultiple);
    }

    if (destroy.has_type_id())
    {
        SendMessageToGame(true, k_ESOMsg_Destroy, destroy);
        SendMessageToGame(false, k_ESOMsg_Destroy, destroy);
    }

    if (notification.item_id_size() > 0)
    {
        SendMessageToGame(false, k_EMsgGCItemCustomizationNotification, notification);
    }
}

void ClientGC::HandleNetMessage(const void *data, uint32_t size)
{
    GCMessageRead messageRead{ 0, data, size };
    if (!messageRead.IsValid())
    {
        assert(false);
        return;
    }

    if (messageRead.IsProtobuf())
    {
        switch (messageRead.TypeUnmasked())
        {
        case k_EMsgGC_IncrementKillCountAttribute:
            IncrementKillCountAttribute(messageRead);
            return;
        }
    }

    Platform::Print("ClientGC::HandleNetMessage: unhandled protobuf message %s\n",
        MessageName(messageRead.TypeUnmasked()));
}

void ClientGC::HandleSOCacheRequest()
{
    CMsgSOCacheSubscribed message;
    m_inventory.BuildCacheSubscription(message, CONFIG_PLAYER_LEVEL, true);

    GCMessageWrite messageWrite{ k_ESOMsg_CacheSubscribed, message };
    PostToHost(HostEvent::NetMessage, 0, messageWrite.Data(), messageWrite.Size());
}

void ClientGC::SendMessageToGame(bool sendToGameServer, uint32_t type,
    const google::protobuf::MessageLite &message, uint64_t jobId)
{
    GCMessageWrite messageWrite{ type, message, jobId };

    if (sendToGameServer)
    {
        PostToHost(HostEvent::NetMessage, 0, messageWrite.Data(), messageWrite.Size());
    }

    PostToHost(HostEvent::Message, messageWrite.TypeMasked(), messageWrite.Data(), messageWrite.Size());
}

constexpr uint32_t MakeAddress(uint32_t v1, uint32_t v2, uint32_t v3, uint32_t v4)
{
    return v4 | (v3 << 8) | (v2 << 16) | (v1 << 24);
}

static void BuildCSWelcome(CMsgCStrike15Welcome &message)
{
    message.set_store_item_hash(136617352);
    message.set_timeplayedconsecutively(0);
    message.set_time_first_played(1329845773);
    message.set_last_time_played(1680260376);
    message.set_last_ip_address(MakeAddress(127, 0, 0, 1));
}

void ClientGC::BuildMatchmakingHello(CMsgGCCStrike15_v2_MatchmakingGC2ClientHello &message)
{
    message.set_account_id(EffectiveAccountId());

    message.mutable_global_stats()->set_players_online(10000);
    message.mutable_global_stats()->set_servers_online(10000);
    message.mutable_global_stats()->set_players_searching(1000);
    message.mutable_global_stats()->set_servers_available(1000);
    message.mutable_global_stats()->set_ongoing_matches(999);
    message.mutable_global_stats()->set_search_time_avg(60);

    auto* stats = message.mutable_global_stats();
    stats->set_players_searching(1000);
    stats->set_servers_available(1000);
    stats->set_search_time_avg(60);

    auto* detail = stats->add_search_statistics();
    detail->set_game_type(6);
    detail->set_search_time_avg(60);
    detail->set_players_searching(1000);

    message.mutable_global_stats()->set_main_post_url("");
    message.mutable_global_stats()->set_required_appid_version(13857);
    message.mutable_global_stats()->set_pricesheet_version(1680057676);
    message.mutable_global_stats()->set_twitch_streams_version(2);
    message.mutable_global_stats()->set_active_tournament_eventid(20);
    message.mutable_global_stats()->set_active_survey_id(0);
    message.mutable_global_stats()->set_required_appid_version2(13862);

    message.set_vac_banned(false);
    message.mutable_commendation()->set_cmd_friendly(CONFIG_COMMENDED_FRIENDLY);
    message.mutable_commendation()->set_cmd_teaching(CONFIG_COMMENDED_TEACHING);
    message.mutable_commendation()->set_cmd_leader(CONFIG_COMMENDED_LEADER);
    message.set_player_level(CONFIG_PLAYER_LEVEL);
    message.set_player_cur_xp(CONFIG_PLAYER_XP);
}

void ClientGC::BuildClientWelcome(CMsgClientWelcome &message, const CMsgCStrike15Welcome &csWelcome,
    const CMsgGCCStrike15_v2_MatchmakingGC2ClientHello &matchmakingHello)
{
    message.set_version(0);
    message.set_game_data(csWelcome.SerializeAsString());
    m_inventory.BuildCacheSubscription(*message.add_outofdate_subscribed_caches(), CONFIG_PLAYER_LEVEL, false);
    message.mutable_location()->set_latitude(65.0133006f);
    message.mutable_location()->set_longitude(25.4646212f);
    message.mutable_location()->set_country("RU");
    message.set_game_data2(matchmakingHello.SerializeAsString());
    message.set_rtime32_gc_welcome_timestamp(static_cast<uint32_t>(time(nullptr)));
    message.set_currency(3); // RUB
    message.set_txn_country_code("RU");
}

void ClientGC::SendRankUpdate()
{
    CMsgGCCStrike15_v2_ClientGCRankUpdate message;

    PlayerRankingInfo *rank = message.add_rankings();
    rank->set_account_id(EffectiveAccountId());
    rank->set_rank_id(CONFIG_COMPETITIVE_RANK);
    rank->set_wins(CONFIG_COMPETITIVE_WINS);
    rank->set_rank_type_id(RankTypeCompetitive);

    rank = message.add_rankings();
    rank->set_account_id(EffectiveAccountId());
    rank->set_rank_id(CONFIG_WINGMAN_RANK);
    rank->set_wins(CONFIG_WINGMAN_WINS);
    rank->set_rank_type_id(RankTypeWingman);

    rank = message.add_rankings();
    rank->set_account_id(AccountId());
    rank->set_rank_id(CONFIG_DANGERZONE_RANK);
    rank->set_wins(CONFIG_DANGERZONE_WINS);
    rank->set_rank_type_id(RankTypeDangerZone);

    SendMessageToGame(false, k_EMsgGCCStrike15_v2_ClientGCRankUpdate, message);
}

void ClientGC::OnClientHello(GCMessageRead &messageRead)
{
    CMsgClientHello hello;
    if (!messageRead.ReadProtobuf(hello))
    {
        Platform::Print("Parsing CMsgClientHello failed, ignoring\n");
        return;
    }

    CMsgCStrike15Welcome csWelcome;
    BuildCSWelcome(csWelcome);

    CMsgGCCStrike15_v2_MatchmakingGC2ClientHello mmHello;
    BuildMatchmakingHello(mmHello);

    CMsgClientWelcome clientWelcome;
    BuildClientWelcome(clientWelcome, csWelcome, mmHello);

    SendMessageToGame(false, k_EMsgGCClientWelcome, clientWelcome);
    SendMessageToGame(false, k_EMsgGCCStrike15_v2_MatchmakingGC2ClientHello, mmHello);
    SendRankUpdate();

    uint32_t cooldown = 0;
    if (cooldown > 0) {
        SendCompetitiveCooldown();
    }
}

void ClientGC::AdjustItemEquippedState(GCMessageRead &messageRead)
{
    CMsgAdjustItemEquippedState message;
    if (!messageRead.ReadProtobuf(message))
    {
        Platform::Print("Parsing CMsgAdjustItemEquippedState failed, ignoring\n");
        return;
    }

    CMsgSOMultipleObjects update;
    if (!m_inventory.EquipItem(message.item_id(), message.new_class(), message.new_slot(), update))
    {
        assert(false);
        return;
    }

    SendMessageToGame(true, k_ESOMsg_UpdateMultiple, update);
}

void ClientGC::ClientPlayerDecalSign(GCMessageRead &messageRead)
{
    CMsgGCCStrike15_v2_ClientPlayerDecalSign message;
    if (!messageRead.ReadProtobuf(message))
    {
        Platform::Print("Parsing CMsgGCCStrike15_v2_ClientPlayerDecalSign failed, ignoring\n");
        return;
    }

    if (!Graffiti::SignMessage(*message.mutable_data()))
    {
        Platform::Print("Could not sign graffiti! it won't appear\n");
        return;
    }

    SendMessageToGame(false, k_EMsgGCCStrike15_v2_ClientPlayerDecalSign, message);
}

void ClientGC::UseItemRequest(GCMessageRead &messageRead)
{
    CMsgUseItem message;
    if (!messageRead.ReadProtobuf(message))
    {
        Platform::Print("Parsing CMsgUseItem failed, ignoring\n");
        return;
    }

    uint64_t itemId = message.item_id();

    const CSOEconItem *giftItem = m_inventory.GetItem(itemId);
    if (giftItem)
    {
        uint32_t defIndex = giftItem->def_index();
        if (defIndex == 1210 || defIndex == 1211 || defIndex == 1215)
        {
            ProcessGiftUse(itemId);
            return;
        }
    }

    CMsgSOSingleObject destroy;
    CMsgSOMultipleObjects updateMultiple;
    CMsgGCItemCustomizationNotification notification;

    if (m_inventory.UseItem(itemId, destroy, updateMultiple, notification))
    {
        SendMessageToGame(true, k_ESOMsg_Destroy, destroy);
        SendMessageToGame(true, k_ESOMsg_UpdateMultiple, updateMultiple);

        SendMessageToGame(false, k_EMsgGCItemCustomizationNotification, notification);
    }
}

static void AddressString(uint32_t ip, uint32_t port, char *buffer, size_t bufferSize)
{
    snprintf(buffer, bufferSize,
        "%u.%u.%u.%u:%u\n",
        (ip >> 24) & 0xff,
        (ip >> 16) & 0xff,
        (ip >> 8) & 0xff,
        ip & 0xff,
        port);
}

void ClientGC::ClientRequestJoinServerData(GCMessageRead &messageRead)
{
    CMsgGCCStrike15_v2_ClientRequestJoinServerData request;
    if (!messageRead.ReadProtobuf(request))
    {
        Platform::Print("Parsing CMsgGCCStrike15_v2_ClientRequestJoinServerData failed, ignoring\n");
        return;
    }

    CMsgGCCStrike15_v2_ClientRequestJoinServerData response = request;

    if (m_matchmakingReservationSent) {
        response.mutable_res()->set_serverid(0x12345678);
        response.mutable_res()->set_direct_udp_ip(0xC0A8000E);
        response.mutable_res()->set_direct_udp_port(27019);
        response.mutable_res()->set_reservationid(m_matchmakingReservationId);
        response.mutable_res()->set_server_address("192.168.0.14:27019");
    } else {
        response.mutable_res()->set_serverid(request.version());
        response.mutable_res()->set_direct_udp_ip(request.server_ip());
        response.mutable_res()->set_direct_udp_port(request.server_port());
        response.mutable_res()->set_reservationid(GameServerCookieId);
        char addressString[32];
        AddressString(request.server_ip(), request.server_port(), addressString, sizeof(addressString));
        response.mutable_res()->set_server_address(addressString);
    }

    SendMessageToGame(false, k_EMsgGCCStrike15_v2_ClientRequestJoinServerData, response);
}

void ClientGC::SetItemPositions(GCMessageRead &messageRead)
{
    CMsgSetItemPositions message;
    if (!messageRead.ReadProtobuf(message))
    {
        Platform::Print("Parsing CMsgSetItemPositions failed, ignoring\n");
        return;
    }

    std::vector<CMsgItemAcknowledged> acknowledgements;
    acknowledgements.reserve(message.item_positions_size());

    CMsgSOMultipleObjects update;
    if (m_inventory.SetItemPositions(message, acknowledgements, update))
    {
        for (const CMsgItemAcknowledged &acknowledgement : acknowledgements)
        {
            GCMessageWrite messageWrite{ k_EMsgGCItemAcknowledged, acknowledgement };
            PostToHost(HostEvent::NetMessage, 0, messageWrite.Data(), messageWrite.Size());
        }

        SendMessageToGame(true, k_ESOMsg_UpdateMultiple, update);
    }
    else
    {
        assert(false);
    }
}

void ClientGC::IncrementKillCountAttribute(GCMessageRead &messageRead)
{
    CMsgIncrementKillCountAttribute message;
    if (!messageRead.ReadProtobuf(message))
    {
        Platform::Print("Parsing CMsgIncrementKillCountAttribute failed, ignoring\n");
        return;
    }

    assert(message.event_type() == 0);

    CMsgSOSingleObject update;
    if (m_inventory.IncrementKillCountAttribute(message.item_id(), message.amount(), update))
    {
        SendMessageToGame(true, k_ESOMsg_Update, update);
    }
    else
    {
        assert(false);
    }
}

void ClientGC::ApplySticker(GCMessageRead &messageRead)
{
    CMsgApplySticker message;
    if (!messageRead.ReadProtobuf(message))
    {
        Platform::Print("Parsing CMsgApplySticker failed, ignoring\n");
        return;
    }

    assert(!message.item_item_id() != !message.baseitem_defidx());

    CMsgSOSingleObject update, destroy;
    CMsgGCItemCustomizationNotification notification;

    if (!message.sticker_item_id())
    {
        if (m_inventory.ScrapeSticker(message, update, destroy, notification))
        {
            if (destroy.has_type_id())
            {
                SendMessageToGame(true, k_ESOMsg_Destroy, destroy);
            }

            if (update.has_type_id())
            {
                SendMessageToGame(true, k_ESOMsg_Update, update);
            }

            if (notification.has_request())
            {
                SendMessageToGame(false, k_EMsgGCItemCustomizationNotification, notification);
            }
        }
        else
        {
            assert(false);
        }
    }
    else if (m_inventory.ApplySticker(message, update, destroy, notification))
    {
        SendMessageToGame(true, k_ESOMsg_Destroy, destroy);
        SendMessageToGame(true, k_ESOMsg_Update, update);

        SendMessageToGame(false, k_EMsgGCItemCustomizationNotification, notification);
    }
    else
    {
        assert(false);
    }
}

void ClientGC::StoreGetUserData(GCMessageRead &messageRead)
{
    // Формируем прайс-лист прямо здесь, без загрузки из файла
    CMsgStoreGetUserDataResponse response;
    response.set_result(1);
    response.set_price_sheet_version(1729);

    // Строим бинарный KeyValue с ценами и категориями
    KeyValue storeRoot("store");
    KeyValue &bannerLayout = storeRoot.AddSubkey("store_banner_layout");

    // Для каждого предмета в ITEM_PRICES добавляем запись в banner_layout
    for (const auto &pair : ITEM_PRICES)
    {
        uint32_t defIndex = pair.first;
        int price = pair.second;

        KeyValue &entry = bannerLayout.AddSubkey(std::to_string(defIndex));
        entry.AddNumber("price", price);
        // Можно добавить другие поля, если нужно
    }

    // Также добавляем категории и метаданные (можно упрощённо)
    KeyValue &metadata = storeRoot.AddSubkey("store_metadata");
    KeyValue &categories = metadata.AddSubkey("categories");
    KeyValue &misc = categories.AddSubkey("Misc");
    misc.AddString("label_token", "#Store_Misc");
    misc.AddNumber("home", 1);
    misc.AddNumber("default", 1);

    std::string binaryString;
    binaryString.reserve(1 << 17);
    storeRoot.BinaryWriteToString(binaryString);

    *response.mutable_price_sheet() = std::move(binaryString);

    SendMessageToGame(false, k_EMsgGCStoreGetUserDataResponse, response);
}

void ClientGC::StorePurchaseInit(GCMessageRead &messageRead)
{
    CMsgGCStorePurchaseInit message;
    if (!messageRead.ReadProtobuf(message))
    {
        Platform::Print("Parsing CMsgGCStorePurchaseInit failed, ignoring\n");
        return;
    }

    uint64_t transactionId = Random{}.Integer<uint64_t>();
    assert(!m_transactionId);
    m_transactionId = transactionId;
    m_transactionItemIds.reserve(message.line_items_size());

    std::vector<CMsgSOSingleObject> inventoryUpdate;

    for (const auto &item : message.line_items())
    {
        for (uint32_t i = 0; i < item.quantity(); i++)
        {
            uint64_t itemId = m_inventory.PurchaseItem(item.item_def_id(), inventoryUpdate);
            if (!itemId)
            {
                assert(false);
            }
            else
            {
                m_transactionItemIds.push_back(itemId);
            }
        }
    }

    char url[128];
    snprintf(url, sizeof(url), "https://checkout.steampowered.com/checkout/approvetxn/%llu/?returnurl=steam", transactionId);

    CMsgGCStorePurchaseInitResponse response;
    response.set_result(1);
    response.set_txn_id(transactionId);
    response.set_url(url);
    response.mutable_item_ids()->Assign(m_transactionItemIds.begin(), m_transactionItemIds.end());

    SendMessageToGame(false, k_EMsgGCStorePurchaseInitResponse, response, messageRead.JobId());

    for (auto &newItem : inventoryUpdate)
    {
        SendMessageToGame(true, k_ESOMsg_Create, newItem);
    }

    PostToHost(HostEvent::MicroTransactionResponse, 0, nullptr, 0);
}

void ClientGC::StorePurchaseFinalize(GCMessageRead &messageRead)
{
    CMsgGCStorePurchaseFinalize message;
    if (!messageRead.ReadProtobuf(message))
    {
        Platform::Print("Parsing CMsgGCStorePurchaseFinalize failed, ignoring\n");
        return;
    }

    assert(m_transactionId);

    CMsgGCStorePurchaseFinalizeResponse response;
    response.set_result(1);
    response.mutable_item_ids()->Assign(m_transactionItemIds.begin(), m_transactionItemIds.end());
    SendMessageToGame(false, k_EMsgGCStorePurchaseFinalizeResponse, response, messageRead.JobId());

    m_transactionId = 0;
}

void ClientGC::PartySearch(GCMessageRead &messageRead)
{
    CMsgGCCStrike15_v2_Party_Search message;
    if (!messageRead.ReadProtobuf(message))
    {
        Platform::Print("Parsing CMsgGCCStrike15_v2_Party_Search failed, ignoring\n");
        return;
    }

    CMsgGCCStrike15_v2_Party_SearchResults response;

    CMsgGCCStrike15_v2_Party_SearchResults::Entry *entry = response.add_entries();
    entry->set_id(AccountId());
    entry->set_grp(3);
    entry->set_game_type(message.game_type());
    entry->set_apr(1);
    entry->set_ark(std::rand() % 18 + 1);
    entry->set_loc(30066);
    entry->set_accountid(EffectiveAccountId());

    // Друзья из конфига (заглушка, можно оставить)
    for (uint32_t player_id : GetConfig().GetFriends())
    {
        if (AccountId() == player_id) continue;

        entry = response.add_entries();
        entry->set_id(player_id);
        entry->set_grp(3);
        entry->set_game_type(message.game_type());
        entry->set_apr(std::rand() % 40 + 1);
        entry->set_ark(std::rand() % 18 + 1);
        entry->set_loc(30066);
        entry->set_accountid(player_id);
    }

    SendMessageToGame(false, k_EMsgGCCStrike15_v2_Party_Search, response);
}

void ClientGC::RequestCoPlays(GCMessageRead &messageRead)
{
    CMsgGCCStrike15_v2_Account_RequestCoPlays message;
    if (!messageRead.ReadProtobuf(message))
    {
        Platform::Print("Parsing CMsgGCCStrike15_v2_Account_RequestCoPlays failed, ignoring\n");
        return;
    }

    CMsgGCCStrike15_v2_Account_RequestCoPlays_Player *player = message.add_players();
    player->set_accountid(EffectiveAccountId());
    player->set_online(true);
    player->set_rtcoplay(1771263169);

    for (uint32_t player_id : GetConfig().GetFriends())
    {
        if (AccountId() == player_id) continue;

        player = message.add_players();
        player->set_accountid(player_id);
        player->set_online(true);
        player->set_rtcoplay(1771262169);
    }

    message.set_servertime(1771263169);

    SendMessageToGame(false, k_EMsgGCCStrike15_v2_Account_RequestCoPlays, message);
}

void ClientGC::ClientRequestPlayersProfile(GCMessageRead &messageRead)
{
    CMsgGCCStrike15_v2_ClientRequestPlayersProfile message;
    if (!messageRead.ReadProtobuf(message))
    {
        Platform::Print("Parsing CMsgGCCStrike15_v2_ClientRequestPlayersProfile failed, ignoring\n");
        return;
    }

    Platform::Print("Requested accountId: %u\n", message.account_id());

    CMsgGCCStrike15_v2_PlayersProfile response;
    response.set_request_id(message.account_id());

    CMsgGCCStrike15_v2_MatchmakingGC2ClientHello* mmHello = response.add_account_profiles();
    mmHello->set_account_id(message.account_id());
    mmHello->mutable_commendation()->set_cmd_friendly(CONFIG_COMMENDED_FRIENDLY);
    mmHello->mutable_commendation()->set_cmd_teaching(CONFIG_COMMENDED_TEACHING);
    mmHello->mutable_commendation()->set_cmd_leader(CONFIG_COMMENDED_LEADER);
    mmHello->set_player_level(CONFIG_PLAYER_LEVEL);
    mmHello->set_player_cur_xp(CONFIG_PLAYER_XP);

    std::vector<int> friends = GetConfig().GetFriends();
    bool requestedInFriends = false;
    for (int id : friends) {
        if (static_cast<uint32_t>(id) == message.account_id()) {
            requestedInFriends = true;
            break;
        }
    }
    if (!requestedInFriends) {
        friends.push_back(static_cast<int>(message.account_id()));
    }

    SendMessageToGame(false, k_EMsgGCCStrike15_v2_PlayersProfile, response);
}

void ClientGC::CasketItemLoadContents(GCMessageRead &messageRead)
{
    CMsgCasketItem message;
    if (!messageRead.ReadProtobuf(message))
    {
        Platform::Print("Parsing CasketItemLoadContents::CMsgCasketItem failed, ignoring\n");
        return;
    }

    CMsgGCItemCustomizationNotification notification;
    notification.set_request(k_EGCItemCustomizationNotification_CasketContents);
    notification.add_item_id(message.casket_item_id());

    SendMessageToGame(false, k_EMsgGCItemCustomizationNotification, notification);
}

void ClientGC::CasketItemAdd(GCMessageRead &messageRead)
{
    CMsgCasketItem message;
    if (!messageRead.ReadProtobuf(message))
    {
        Platform::Print("Parsing CasketItemAdd::CMsgCasketItem failed, ignoring\n");
        return;
    }

    CMsgSOSingleObject updateItem, updateCasket;
    CMsgGCItemCustomizationNotification notification;
    if (m_inventory.CasketItemAdd(message.casket_item_id(), message.item_item_id(), updateItem, updateCasket, notification))
    {
        SendMessageToGame(false, k_ESOMsg_Update, updateItem);
        SendMessageToGame(false, k_ESOMsg_Update, updateCasket);
        SendMessageToGame(false, k_EMsgGCItemCustomizationNotification, notification);
    }
}

void ClientGC::CasketItemExtract(GCMessageRead &messageRead)
{
    CMsgCasketItem message;
    if (!messageRead.ReadProtobuf(message))
    {
        Platform::Print("Parsing CasketItemExtract::CMsgCasketItem failed, ignoring\n");
        return;
    }

    CMsgSOSingleObject updateItem, updateCasket;
    CMsgGCItemCustomizationNotification notification;
    if (m_inventory.CasketItemRemove(message.casket_item_id(), message.item_item_id(), updateItem, updateCasket, notification))
    {
        SendMessageToGame(false, k_ESOMsg_Update, updateItem);
        SendMessageToGame(false, k_ESOMsg_Update, updateCasket);
        SendMessageToGame(false, k_EMsgGCItemCustomizationNotification, notification);
    }
}

void ClientGC::StatTrakSwap(GCMessageRead &messageRead)
{
    CMsgApplyStatTrakSwap message;
    if (!messageRead.ReadProtobuf(message))
    {
        Platform::Print("Parsing StatTrakSwap::CMsgApplyStatTrakSwap failed, ignoring\n");
        return;
    }

    CMsgSOSingleObject destroy, updateItem1, updateItem2;
    CMsgGCItemCustomizationNotification notification;

    if (m_inventory.StatTrakSwap(
            message.tool_item_id(),
            message.item_1_item_id(),
            message.item_2_item_id(),
            destroy,
            updateItem1,
            updateItem2,
            notification))
    {
        SendMessageToGame(true, k_ESOMsg_Destroy, destroy);
        SendMessageToGame(true, k_ESOMsg_Update, updateItem1);
        SendMessageToGame(true, k_ESOMsg_Update, updateItem2);

        SendMessageToGame(false, k_EMsgGCItemCustomizationNotification, notification);
    }
}

void ClientGC::DeleteItem(GCMessageRead &messageRead)
{
    uint64_t itemId = messageRead.ReadUint64();
    if (!messageRead.IsValid())
    {
        Platform::Print("Parsing CMsgGCDelete failed, ignoring\n");
        return;
    }

    CMsgSOSingleObject destroyed;
    if (m_inventory.RemoveItem(itemId, destroyed))
    {
        SendMessageToGame(true, k_ESOMsg_Destroy, destroyed);
    }
    else
    {
        assert(false);
    }
}

void ClientGC::UnlockCrate(GCMessageRead &messageRead)
{
    uint64_t keyId = messageRead.ReadUint64();
    uint64_t crateId = messageRead.ReadUint64();
    if (!messageRead.IsValid())
    {
        Platform::Print("Parsing CMsgGCUnlockCrate failed, ignoring\n");
        return;
    }

    Platform::Print("CASE OPENING %llu with %llu\n", crateId, keyId);

    CMsgSOSingleObject destroyCrate, destroyKey, newItem;
    CMsgGCItemCustomizationNotification notification;

    if (m_inventory.UnlockCrate(
            crateId,
            keyId,
            destroyCrate,
            destroyKey,
            newItem,
            notification))
    {
        SendMessageToGame(true, k_ESOMsg_Destroy, destroyCrate);
        SendMessageToGame(true, k_ESOMsg_Destroy, destroyKey);
        SendMessageToGame(true, k_ESOMsg_Create, newItem);

        SendMessageToGame(false, k_EMsgGCItemCustomizationNotification, notification);
    }
    else
    {
        assert(false);
    }
}

void ClientGC::NameItem(GCMessageRead &messageRead)
{
    uint64_t nameTagId = messageRead.ReadUint64();
    uint64_t itemId = messageRead.ReadUint64();
    messageRead.ReadData(1); // skip the sentinel
    std::string_view name = messageRead.ReadString();

    if (!messageRead.IsValid())
    {
        Platform::Print("Parsing CMsgGCNameItem failed, ignoring\n");
        return;
    }

    CMsgSOSingleObject update, destroy;
    CMsgGCItemCustomizationNotification notification;
    if (m_inventory.NameItem(nameTagId, itemId, name, update, destroy, notification))
    {
        SendMessageToGame(true, k_ESOMsg_Update, update);
        SendMessageToGame(true, k_ESOMsg_Destroy, destroy);

        SendMessageToGame(false, k_EMsgGCItemCustomizationNotification, notification);
    }
    else
    {
        assert(false);
    }
}

void ClientGC::NameBaseItem(GCMessageRead &messageRead)
{
    uint64_t nameTagId = messageRead.ReadUint64();
    uint32_t defIndex = messageRead.ReadUint32();
    messageRead.ReadData(1); // skip the sentinel
    std::string_view name = messageRead.ReadString();

    if (!messageRead.IsValid())
    {
        Platform::Print("Parsing CMsgGCNameBaseItem failed, ignoring\n");
        return;
    }

    CMsgSOSingleObject create, destroy;
    CMsgGCItemCustomizationNotification notification;
    if (m_inventory.NameBaseItem(nameTagId, defIndex, name, create, destroy, notification))
    {
        SendMessageToGame(true, k_ESOMsg_Create, create);
        SendMessageToGame(true, k_ESOMsg_Destroy, destroy);

        SendMessageToGame(false, k_EMsgGCItemCustomizationNotification, notification);
    }
    else
    {
        assert(false);
    }
}

void ClientGC::RemoveItemName(GCMessageRead &messageRead)
{
    uint64_t itemId = messageRead.ReadUint64();
    if (!messageRead.IsValid())
    {
        Platform::Print("Parsing CMsgGCRemoveItemName failed, ignoring\n");
        return;
    }

    CMsgSOSingleObject update, destroy;
    CMsgGCItemCustomizationNotification notification;
    if (m_inventory.RemoveItemName(itemId, update, destroy, notification))
    {
        if (update.has_type_id())
        {
            SendMessageToGame(true, k_ESOMsg_Update, update);
        }

        if (destroy.has_type_id())
        {
            SendMessageToGame(true, k_ESOMsg_Destroy, destroy);
        }

        SendMessageToGame(false, k_EMsgGCItemCustomizationNotification, notification);
    }
    else
    {
        assert(false);
    }
}

void ClientGC::SendInventoryUpdate()
{
    CMsgSOCacheSubscribed message;
    m_inventory.BuildCacheSubscription(message, CONFIG_PLAYER_LEVEL, false);
    SendMessageToGame(false, k_ESOMsg_CacheSubscribed, message);
}

void ClientGC::CheckFileReloads()
{
    // Файлы больше не перезагружаем, так как всё захардкожено
    // Но оставляем возможность для обратной совместимости (если нужно)
    UpdateCooldown();
}

void ClientGC::ReloadInventory()
{
    m_inventory.ReloadFromFile();
    SendInventoryUpdate();
}

void ClientGC::ReloadConfig()
{
    // Ничего не делаем, все параметры захардкожены
}

void ClientGC::ReloadPriceSheet()
{
    // Ничего не делаем, прайс-лист строится на лету
}

void ClientGC::ReloadPasses()
{
    // Ничего не делаем, пропуски захардкожены (можно оставить пустым)
}

void ClientGC::ReloadUnusualLootLists()
{
    // Ничего не делаем, списки необычных предметов захардкожены (можно оставить пустым)
}

void ClientGC::FetchOverwatchCases()
{
    ISteamHTTP *http = SteamHTTP();
    if (!http) return;

    HTTPRequestHandle hRequest = http->CreateHTTPRequest(k_EHTTPMethodGET, "https://sasha190409.github.io/csgo/overwatch");
    if (hRequest == k_uAPICallInvalid) return;

    http->SetHTTPRequestHeaderValue(hRequest, "User-Agent", "csgo_gc/1.0");

    SteamAPICall_t hCall;
    if (!http->SendHTTPRequest(hRequest, &hCall))
        Platform::Print("Overwatch: failed to send request\n");
}

void ClientGC::OnOverwatchHTTPResponse(HTTPRequestCompleted_t *pCallback)
{
    if (!pCallback->m_bRequestSuccessful || pCallback->m_eStatusCode != k_EHTTPStatusCode200OK)
    {
        Platform::Print("Overwatch: HTTP request failed (status %d, success %d)\n",
                        pCallback->m_eStatusCode, pCallback->m_bRequestSuccessful);
        return;
    }

    ISteamHTTP *http = SteamHTTP();
    if (!http) return;

    uint32_t bodySize;
    if (!http->GetHTTPResponseBodySize(pCallback->m_hRequest, &bodySize) || bodySize == 0)
        return;

    std::vector<uint8_t> body(bodySize + 1, 0);
    if (!http->GetHTTPResponseBodyData(pCallback->m_hRequest, body.data(), bodySize))
        return;

    std::string json(reinterpret_cast<char*>(body.data()), bodySize);
    Platform::Print("Overwatch: received JSON: %s\n", json.c_str());

    std::vector<uint32_t> suspects;
    size_t pos = 0;
    size_t start = json.find('{');
    if (start == std::string::npos) return;
    size_t end = json.rfind('}');
    if (end == std::string::npos || end <= start) return;
    std::string content = json.substr(start + 1, end - start - 1);

    size_t comma = 0;
    while (comma != std::string::npos)
    {
        size_t next = content.find(',', comma);
        std::string pair = content.substr(comma, (next == std::string::npos) ? std::string::npos : next - comma);
        pair.erase(0, pair.find_first_not_of(" \t\n\r"));
        pair.erase(pair.find_last_not_of(" \t\n\r") + 1);
        if (pair.empty()) break;

        size_t colon = pair.find(':');
        if (colon == std::string::npos) continue;
        std::string key = pair.substr(0, colon);
        std::string value = pair.substr(colon + 1);
        auto trimQuotes = [](std::string& s) {
            if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
                s = s.substr(1, s.size() - 2);
        };
        trimQuotes(key);
        trimQuotes(value);
        if (key.find("case") == 0 && !value.empty())
        {
            uint32_t accId = SteamIDStringToAccountId(value);
            if (accId != 0)
                suspects.push_back(accId);
        }

        comma = (next == std::string::npos) ? std::string::npos : next + 1;
    }

    {
        std::lock_guard<std::mutex> lock(m_overwatchMutex);
        m_overwatchSuspects = std::move(suspects);
        m_nextOverwatchIndex = 0;
        Platform::Print("Overwatch: loaded %zu suspects\n", m_overwatchSuspects.size());
    }
}

uint32_t ClientGC::SteamIDStringToAccountId(const std::string &str)
{
    unsigned int x, y;
    if (sscanf(str.c_str(), "STEAM_%*u:%u:%u", &x, &y) != 2)
        return 0;
    return y * 2 + x;
}

void ClientGC::SendOverwatchCaseAssignment(uint32_t suspectAccountId)
{
    CMsgGCCStrike15_v2_PlayerOverwatchCaseAssignment assignment;
    assignment.set_caseid(m_nextCaseId++);
    assignment.set_suspectid(suspectAccountId);
    assignment.set_fractionid(0);
    assignment.set_numrounds(8);
    assignment.set_fractionrounds(8);
    assignment.set_streakconvictions(0);
    assignment.set_reason(0);
    assignment.set_verdict(0);
    assignment.set_timestamp(static_cast<uint32_t>(time(nullptr)));
    assignment.set_throttleseconds(0);

    std::string url = "https://sasha190409.github.io/csgo/demos/imposter_"
                    + std::to_string(suspectAccountId) + ".dem";
    assignment.set_caseurl(url);

    SendMessageToGame(false, k_EMsgGCCStrike15_v2_PlayerOverwatchCaseAssignment, assignment);

    Platform::Print("Overwatch: assigned case %llu for suspect %u, URL: %s\n",
                    assignment.caseid(), suspectAccountId, url.c_str());
}

void ClientGC::OnOverwatchCaseStatus(GCMessageRead& messageRead)
{
    CMsgGCCStrike15_v2_PlayerOverwatchCaseStatus msg;
    if (!messageRead.ReadProtobuf(msg))
    {
        Platform::Print("Failed to parse OverwatchCaseStatus\n");
        return;
    }

    if (msg.caseid() == 0)
    {
        std::lock_guard<std::mutex> lock(m_overwatchMutex);
        if (m_overwatchSuspects.empty())
        {
            Platform::Print("Overwatch: no suspects available\n");
            return;
        }
        uint32_t suspect = m_overwatchSuspects[m_nextOverwatchIndex];
        m_nextOverwatchIndex = (m_nextOverwatchIndex + 1) % m_overwatchSuspects.size();
        SendOverwatchCaseAssignment(suspect);
    }
    else
    {
        Platform::Print("Overwatch: status check for case %llu (status %u)\n",
                        msg.caseid(), msg.statusid());
    }
}

void ClientGC::OnOverwatchCaseUpdate(GCMessageRead& messageRead)
{
    CMsgGCCStrike15_v2_PlayerOverwatchCaseUpdate msg;
    if (!messageRead.ReadProtobuf(msg)) {
        Platform::Print("Failed to parse OverwatchCaseUpdate\n");
        return;
    }

    Platform::Print("Overwatch: verdict for case %llu, suspect %u, reason %u\n",
                    msg.caseid(), msg.suspectid(), msg.reason());

    SendVerdictToCloudflare(msg);
}

void ClientGC::SendVerdictToCloudflare(const CMsgGCCStrike15_v2_PlayerOverwatchCaseUpdate &msg)
{
    ISteamHTTP *http = SteamHTTP();
    if (!http) return;

    std::string json = "{"
        "\"caseid\":" + std::to_string(msg.caseid()) + ","
        "\"suspectid\":" + std::to_string(msg.suspectid()) + ","
        "\"reason\":" + std::to_string(msg.reason()) + ","
        "\"timestamp\":" + std::to_string(time(nullptr)) +
    "}";

    HTTPRequestHandle hRequest = http->CreateHTTPRequest(k_EHTTPMethodPOST, "https://still-dawn-e090.ivanhihlov4.workers.dev/verdict");
    if (hRequest == k_uAPICallInvalid) return;

    http->SetHTTPRequestHeaderValue(hRequest, "Content-Type", "application/json");

    std::vector<uint8_t> postData(json.begin(), json.end());
    http->SetHTTPRequestRawPostBody(hRequest, "application/json", postData.data(), static_cast<uint32_t>(postData.size()));

    http->SendHTTPRequest(hRequest, nullptr);
}

uint32_t ClientGC::EffectiveAccountId() const
{
    return AccountId();
}
