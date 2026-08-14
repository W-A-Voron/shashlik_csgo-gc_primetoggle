#include "stdafx.h"
#include "gc_client.h"
#include "graffiti.h"
#include "keyvalue.h"
#include <filesystem>
#include "case_opening.h"
#include <steam/isteamhttp.h>
#include "steam_hook.h"

// ------------------------------------------------
// Встроенные данные (заменяют config.txt и price_sheet.txt)
// ------------------------------------------------

// Реалистичные ранги (НЕ круглые числа)
// Competitive: Legendary Eagle Master (12), но вы хотите Captain (24) -> ставим 24
constexpr int CONFIG_COMPETITIVE_RANK = 24;   // Captain (ваш запрос)
constexpr int CONFIG_COMPETITIVE_WINS = 151;
// Wingman: Gold Nova Master (10)
constexpr int CONFIG_WINGMAN_RANK = 10;
constexpr int CONFIG_WINGMAN_WINS = 52;
// Danger Zone: Scout Elite (7)
constexpr int CONFIG_DANGERZONE_RANK = 7;
constexpr int CONFIG_DANGERZONE_WINS = 37;

// Уничтожать ли предметы после использования
constexpr bool CONFIG_DESTROY_USED_ITEMS = true;

// Реалистичные награды (не круглые)
constexpr int CONFIG_COMMENDED_FRIENDLY = 247;
constexpr int CONFIG_COMMENDED_TEACHING = 84;
constexpr int CONFIG_COMMENDED_LEADER = 41;
constexpr int CONFIG_PLAYER_LEVEL = 24;       // Уровень 24, а не 25
constexpr int CONFIG_PLAYER_XP = 3568;

// Веса редкости (реальные шансы, как у Valve)
static const std::vector<RarityWeight> CONFIG_RARITY_WEIGHTS = {
    { ItemSchema::RarityCommon,   10000000 },
    { ItemSchema::RarityUncommon, 2000000 },
    { ItemSchema::RarityRare,     400000 },
    { ItemSchema::RarityMythical, 80000 },
    { ItemSchema::RarityLegendary,16000 },
    { ItemSchema::RarityAncient,  3200 },
    { ItemSchema::RarityUnusual,  1280 },
};

// Цены (в рублях) и ПРАВИЛЬНЫЕ СТРОКОВЫЕ ИДЕНТИФИКАТОРЫ для магазина
struct StoreItem {
    uint32_t defIndex;
    const char* itemLink;  // строковый ID, который ожидает магазин CS:GO
    int priceRub;
};

static const std::vector<StoreItem> STORE_ITEMS = {
    // Ключи
    { 5000, "Weapon Case Key", 140 },
    { 5010, "E-Sports Weapon Case Key 1", 140 },
    { 5015, "Community Case Key 1", 140 },
    { 5020, "Falchion Case Key", 140 },
    { 5025, "Gamma 2 Case Key", 140 },
    { 5030, "Chroma 3 Case Key", 140 },
    { 5035, "Revolver Case Key", 140 },
    { 5040, "Spectrum 2 Case Key", 140 },
    { 5045, "Horizon Case Key", 140 },
    { 5050, "Danger Zone Case Key", 140 },
    { 5055, "Prisma 2 Case Key", 140 },
    { 5060, "Fracture Case Key", 140 },
    { 5065, "Operation Shattered Web Case Key", 140 },
    { 5070, "Operation Riptide Case Key", 140 },
    { 5075, "Snakebite Case Key", 140 },
    { 5080, "Dreams & Nightmares Case Key", 140 },
    { 5085, "Recoil Case Key", 140 },
    { 5090, "CS20 Case Key", 140 },
    { 5095, "Prisma 2 Case Key", 140 },
    { 5100, "Operation Broken Fang Case Key", 140 },
    // Кейсы
    { 1200, "crate_valve_1", 150 },
    { 1201, "crate_esports_2013", 150 },
    { 1202, "crate_valve_2", 150 },
    { 1203, "crate_operation_ii", 150 },
    { 1204, "crate_esports_2013_winter", 150 },
    { 1205, "crate_community_1", 150 },
    { 1206, "crate_community_2", 150 },
    { 1207, "crate_community_3", 150 },
    { 1208, "crate_community_4", 150 },
    { 1209, "crate_esports_2014_summer", 150 },
    { 1210, "crate_operation_vanguard", 150 },
    { 1211, "crate_community_6", 150 },
    { 1212, "crate_community_7", 150 },
    { 1213, "crate_community_8", 150 },
    { 1214, "crate_community_9", 150 },
    { 1215, "crate_community_10", 150 },
    { 1216, "crate_community_11", 150 },
    { 1217, "crate_community_12", 150 },
    { 1218, "crate_community_13", 150 },
    { 1219, "crate_gamma_2", 150 },
    { 1220, "crate_community_15", 150 },
    { 1221, "crate_community_16", 150 },
    { 1222, "crate_community_17", 150 },
    { 1223, "crate_community_18", 150 },
    { 1224, "crate_community_19", 150 },
    { 1225, "crate_community_20", 150 },
    { 1226, "crate_community_21", 150 },
    { 1227, "crate_community_22", 150 },
    { 1228, "crate_community_23", 150 },
    { 1229, "crate_community_24", 150 },
    { 1230, "crate_community_25", 150 },
    { 1231, "crate_community_26", 150 },
    // Наборы стикеров
    { 2000, "Community Sticker Capsule 1 Key June 2014", 10 },
    { 2001, "Community Sticker Capsule 2", 15 },
    { 2002, "Community Sticker Capsule 3", 20 },
    { 2003, "Community Sticker Capsule 4", 25 },
    { 2004, "Team Roles Sticker Capsule", 30 },
    // Премиум-наборы
    { 3000, "crate_sticker_pack_kat2014_01", 60 },
    { 3001, "crate_sticker_pack_berlin2019_legends", 55 },
    { 3002, "crate_sticker_pack_stockh2021_legends", 65 },
    { 3003, "crate_sticker_pack_antwerp2022_legends", 70 },
    { 3004, "crate_sticker_pack_rio2022_legends", 75 },
    { 3005, "crate_sticker_pack_paris2023_legends", 80 },
    // Прочее
    { 1300, "Music Kit", 200 },
    { 1301, "StatTrak Music Kit", 350 },
    { 1400, "Name Tag", 40 },
    { 1401, "StatTrak Swap Tool", 450 },
    { 1402, "Storage Unit", 600 },
    // Операционные пропуски
    { 4000, "Operation Payback Pass", 100 },
    { 4001, "Operation Bravo Pass", 150 },
    { 4002, "Operation Phoenix Pass", 150 },
    { 4003, "Operation Breakout Pass", 150 },
    { 4004, "Operation Vanguard Pass", 150 },
    { 4005, "Operation Bloodhound Pass", 150 },
    { 4006, "Operation Wildfire Pass", 150 },
    { 4007, "Operation Hydra Pass", 150 },
    { 4008, "Operation Shattered Web Pass", 150 },
    { 4009, "Operation Broken Fang Pass", 150 },
    { 4010, "Operation Riptide Pass", 150 },
    // Турнирные пропуски (зрителя)
    { 4500, "tournament_pass_katowice2019", 200 },
    { 4501, "tournament_pass_berlin2019", 180 },
    { 4502, "tournament_pass_stockh2021", 220 },
    { 4503, "tournament_pass_antwerp2022", 230 },
    { 4504, "tournament_pass_rio2022", 240 },
    { 4505, "tournament_pass_paris2023", 250 },
};

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
    penalty.set_reason(0);
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
    uint32_t cooldown = 0;
    if (cooldown > 0) {
        m_isSearching = false;
        m_isCooldownActive = true;
        m_cooldownEndTime = std::chrono::steady_clock::now() + std::chrono::seconds(cooldown);
        SendMatchmakingUpdate();
        SendCompetitiveCooldown();
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
    *reserve.mutable_reservation() = CMsgGCCStrike15_v2_MatchmakingGC2ServerReserve();

    SendMessageToGame(false, k_EMsgGCCStrike15_v2_MatchmakingGC2ClientReserve, reserve);
    m_matchmakingReservationSent = true;
}

void ClientGC::HandleMessage(uint32_t type, const void *data, uint32_t size)
{
    GCMessageRead messageRead{ type, data, size };
    if (!messageRead.IsValid()) return;

    if (messageRead.IsProtobuf())
    {
        switch (messageRead.TypeUnmasked())
        {
        case k_EMsgGCClientHello: OnClientHello(messageRead); break;
        case k_EMsgGCAdjustItemEquippedState: AdjustItemEquippedState(messageRead); break;
        case k_EMsgGCCStrike15_v2_ClientPlayerDecalSign: ClientPlayerDecalSign(messageRead); break;
        case k_EMsgGCUseItemRequest: UseItemRequest(messageRead); break;
        case k_EMsgGCCStrike15_v2_ClientRequestJoinServerData: ClientRequestJoinServerData(messageRead); break;
        case k_EMsgGCSetItemPositions: SetItemPositions(messageRead); break;
        case k_EMsgGCApplySticker: ApplySticker(messageRead); break;
        case k_EMsgGCStoreGetUserData: StoreGetUserData(messageRead); break;
        case k_EMsgGCStorePurchaseInit: StorePurchaseInit(messageRead); break;
        case k_EMsgGCStorePurchaseFinalize: StorePurchaseFinalize(messageRead); break;
        case k_EMsgGCCStrike15_v2_Party_Search: PartySearch(messageRead); break;
        case k_EMsgGCCStrike15_v2_Account_RequestCoPlays: RequestCoPlays(messageRead); break;
        case k_EMsgGCCStrike15_v2_ClientRequestPlayersProfile: ClientRequestPlayersProfile(messageRead); break;
        case k_EMsgGCCasketItemLoadContents: CasketItemLoadContents(messageRead); break;
        case k_EMsgGCCasketItemAdd: CasketItemAdd(messageRead); break;
        case k_EMsgGCCasketItemExtract: CasketItemExtract(messageRead); break;
        case k_EMsgGCStatTrakSwap: StatTrakSwap(messageRead); break;
        case k_EMsgGCCStrike15_v2_MatchmakingClient2ServerPing: OnMatchmakingPing(messageRead); break;
        case k_EMsgGCCStrike15_v2_MatchmakingStart: OnMatchmakingStart(messageRead); break;
        case k_EMsgGCCStrike15_v2_MatchmakingStop: OnMatchmakingStop(messageRead); break;
        case k_EMsgGCCStrike15_v2_PlayerOverwatchCaseStatus: OnOverwatchCaseStatus(messageRead); break;
        case k_EMsgGCCStrike15_v2_PlayerOverwatchCaseUpdate: OnOverwatchCaseUpdate(messageRead); break;
        default: Platform::Print("GC: unhandled protobuf %s\n", MessageName(messageRead.TypeUnmasked())); break;
        }
    }
    else
    {
        switch (messageRead.TypeUnmasked())
        {
        case k_EMsgGCDelete: DeleteItem(messageRead); break;
        case k_EMsgGCUnlockCrate: UnlockCrate(messageRead); break;
        case k_EMsgGCNameItem: NameItem(messageRead); break;
        case k_EMsgGCNameBaseItem: NameBaseItem(messageRead); break;
        case k_EMsgGCRemoveItemName: RemoveItemName(messageRead); break;
        default: Platform::Print("GC: unhandled struct %s\n", MessageName(messageRead.TypeUnmasked())); break;
        }
    }
}

void ClientGC::HandleNetMessage(const void *data, uint32_t size)
{
    GCMessageRead messageRead{ 0, data, size };
    if (!messageRead.IsValid()) return;

    if (messageRead.IsProtobuf() && messageRead.TypeUnmasked() == k_EMsgGC_IncrementKillCountAttribute)
    {
        IncrementKillCountAttribute(messageRead);
    }
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
    if (sendToGameServer) PostToHost(HostEvent::NetMessage, 0, messageWrite.Data(), messageWrite.Size());
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
    message.set_currency(3);
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
    CMsgClientHello hello; (void)hello;
    CMsgCStrike15Welcome csWelcome; BuildCSWelcome(csWelcome);
    CMsgGCCStrike15_v2_MatchmakingGC2ClientHello mmHello; BuildMatchmakingHello(mmHello);
    CMsgClientWelcome clientWelcome; BuildClientWelcome(clientWelcome, csWelcome, mmHello);

    SendMessageToGame(false, k_EMsgGCClientWelcome, clientWelcome);
    SendMessageToGame(false, k_EMsgGCCStrike15_v2_MatchmakingGC2ClientHello, mmHello);
    SendRankUpdate();
}

void ClientGC::AdjustItemEquippedState(GCMessageRead &messageRead)
{
    CMsgAdjustItemEquippedState message;
    if (!messageRead.ReadProtobuf(message)) return;
    CMsgSOMultipleObjects update;
    if (m_inventory.EquipItem(message.item_id(), message.new_class(), message.new_slot(), update))
        SendMessageToGame(true, k_ESOMsg_UpdateMultiple, update);
}

void ClientGC::ClientPlayerDecalSign(GCMessageRead &messageRead)
{
    CMsgGCCStrike15_v2_ClientPlayerDecalSign message;
    if (!messageRead.ReadProtobuf(message)) return;
    if (Graffiti::SignMessage(*message.mutable_data()))
        SendMessageToGame(false, k_EMsgGCCStrike15_v2_ClientPlayerDecalSign, message);
}

void ClientGC::UseItemRequest(GCMessageRead &messageRead)
{
    CMsgUseItem message;
    if (!messageRead.ReadProtobuf(message)) return;
    uint64_t itemId = message.item_id();
    const CSOEconItem *giftItem = m_inventory.GetItem(itemId);
    if (giftItem && (giftItem->def_index() == 1210 || giftItem->def_index() == 1211 || giftItem->def_index() == 1215))
    {
        ProcessGiftUse(itemId);
        return;
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

void ClientGC::ProcessGiftUse(uint64_t giftId)
{
    const CSOEconItem *giftItem = m_inventory.GetItem(giftId);
    if (!giftItem) return;
    uint32_t defIndex = giftItem->def_index();
    int numItems = (defIndex == 1210) ? 1 : (defIndex == 1211) ? 9 : 25;
    uint32_t series = 0;
    uint32_t attrDef = m_inventory.GetItemSchema().GetAttributeDefIndex("set supply crate series");
    if (attrDef) {
        for (const auto &attr : giftItem->attribute()) {
            if (attr.def_index() == attrDef) {
                series = m_inventory.GetItemSchema().AttributeUint32(&attr);
                break;
            }
        }
    }
    if (series == 0) return;

    const LootList *lootList = m_inventory.GetItemSchema().GetLootListBySeries(series);
    if (!lootList) return;

    CMsgSOMultipleObjects updateMultiple;
    CMsgSOSingleObject destroy;
    CMsgGCItemCustomizationNotification notification;
    notification.set_request(k_EGCItemCustomizationNotification_Gift);

    CaseOpening caseOpening(m_inventory.GetItemSchema(), m_inventory.GetRandom());
    for (int i = 0; i < numItems; i++) {
        CSOEconItem newItemProto;
        if (!caseOpening.SelectItemFromLootList(*lootList, newItemProto)) continue;
        CSOEconItem &createdItem = m_inventory.CreateItem(newItemProto);
        m_inventory.AddToMultipleObjects(updateMultiple, SOTypeItem, createdItem);
        notification.add_item_id(createdItem.id());
    }

    if (CONFIG_DESTROY_USED_ITEMS) m_inventory.RemoveItem(giftId, destroy);

    if (updateMultiple.objects_modified_size() > 0) {
        SendMessageToGame(true, k_ESOMsg_UpdateMultiple, updateMultiple);
        SendMessageToGame(false, k_ESOMsg_UpdateMultiple, updateMultiple);
    }
    if (destroy.has_type_id()) {
        SendMessageToGame(true, k_ESOMsg_Destroy, destroy);
        SendMessageToGame(false, k_ESOMsg_Destroy, destroy);
    }
    if (notification.item_id_size() > 0)
        SendMessageToGame(false, k_EMsgGCItemCustomizationNotification, notification);
}

void ClientGC::ClientRequestJoinServerData(GCMessageRead &messageRead)
{
    CMsgGCCStrike15_v2_ClientRequestJoinServerData request;
    if (!messageRead.ReadProtobuf(request)) return;
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
    if (!messageRead.ReadProtobuf(message)) return;
    std::vector<CMsgItemAcknowledged> acknowledgements;
    CMsgSOMultipleObjects update;
    if (m_inventory.SetItemPositions(message, acknowledgements, update)) {
        for (const auto &ack : acknowledgements) {
            GCMessageWrite msgWrite{ k_EMsgGCItemAcknowledged, ack };
            PostToHost(HostEvent::NetMessage, 0, msgWrite.Data(), msgWrite.Size());
        }
        SendMessageToGame(true, k_ESOMsg_UpdateMultiple, update);
    }
}

void ClientGC::IncrementKillCountAttribute(GCMessageRead &messageRead)
{
    CMsgIncrementKillCountAttribute message;
    if (!messageRead.ReadProtobuf(message)) return;
    CMsgSOSingleObject update;
    if (m_inventory.IncrementKillCountAttribute(message.item_id(), message.amount(), update))
        SendMessageToGame(true, k_ESOMsg_Update, update);
}

void ClientGC::ApplySticker(GCMessageRead &messageRead)
{
    CMsgApplySticker message;
    if (!messageRead.ReadProtobuf(message)) return;
    CMsgSOSingleObject update, destroy;
    CMsgGCItemCustomizationNotification notification;
    if (!message.sticker_item_id()) {
        if (m_inventory.ScrapeSticker(message, update, destroy, notification)) {
            if (destroy.has_type_id()) SendMessageToGame(true, k_ESOMsg_Destroy, destroy);
            if (update.has_type_id()) SendMessageToGame(true, k_ESOMsg_Update, update);
            if (notification.has_request()) SendMessageToGame(false, k_EMsgGCItemCustomizationNotification, notification);
        }
    } else if (m_inventory.ApplySticker(message, update, destroy, notification)) {
        SendMessageToGame(true, k_ESOMsg_Destroy, destroy);
        SendMessageToGame(true, k_ESOMsg_Update, update);
        SendMessageToGame(false, k_EMsgGCItemCustomizationNotification, notification);
    }
}

// ------------------------------------------------
// МАГАЗИН: правильная генерация прайс-листа
// ------------------------------------------------
void ClientGC::StoreGetUserData(GCMessageRead &messageRead)
{
    KeyValue storeRoot("store");

    // 1. store_banner_layout
    KeyValue &bannerLayout = storeRoot.AddSubkey("store_banner_layout");
    for (const auto &item : STORE_ITEMS) {
        KeyValue &entry = bannerLayout.AddSubkey(std::to_string(item.defIndex));
        entry.AddString("custom_format", "single");
        entry.AddString("market_link", "1");
    }

    // 2. entries – это то, что игра показывает в магазине
    KeyValue &entries = storeRoot.AddSubkey("entries");
    for (const auto &item : STORE_ITEMS) {
        KeyValue &entry = entries.AddSubkey(item.itemLink);  // Важно: используем строковый ID!
        entry.AddString("item_link", item.itemLink);
        entry.AddString("category_tags", "Misc");
        KeyValue &prices = entry.AddSubkey("prices");
        prices.AddString("RUB", std::to_string(item.priceRub));
    }

    // 3. store_metadata – категории
    KeyValue &metadata = storeRoot.AddSubkey("store_metadata");
    KeyValue &categories = metadata.AddSubkey("categories");
    KeyValue &misc = categories.AddSubkey("Misc");
    misc.AddString("label_token", "#Store_Misc");
    misc.AddNumber("home", 1);
    misc.AddNumber("default", 1);

    // Генерация бинарного дампа
    std::string binaryString;
    binaryString.reserve(1 << 17);
    storeRoot.BinaryWriteToString(binaryString);

    CMsgStoreGetUserDataResponse response;
    response.set_result(1);
    response.set_price_sheet_version(1729);
    *response.mutable_price_sheet() = std::move(binaryString);

    SendMessageToGame(false, k_EMsgGCStoreGetUserDataResponse, response);
}

void ClientGC::StorePurchaseInit(GCMessageRead &messageRead)
{
    CMsgGCStorePurchaseInit message;
    if (!messageRead.ReadProtobuf(message)) return;

    uint64_t transactionId = Random{}.Integer<uint64_t>();
    assert(!m_transactionId);
    m_transactionId = transactionId;
    m_transactionItemIds.reserve(message.line_items_size());

    std::vector<CMsgSOSingleObject> inventoryUpdate;
    for (const auto &item : message.line_items()) {
        for (uint32_t i = 0; i < item.quantity(); i++) {
            uint64_t itemId = m_inventory.PurchaseItem(item.item_def_id(), inventoryUpdate);
            if (itemId) m_transactionItemIds.push_back(itemId);
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
    for (auto &newItem : inventoryUpdate) {
        SendMessageToGame(true, k_ESOMsg_Create, newItem);
    }
    PostToHost(HostEvent::MicroTransactionResponse, 0, nullptr, 0);
}

void ClientGC::StorePurchaseFinalize(GCMessageRead &messageRead)
{
    CMsgGCStorePurchaseFinalize message;
    if (!messageRead.ReadProtobuf(message)) return;
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
    if (!messageRead.ReadProtobuf(message)) return;
    CMsgGCCStrike15_v2_Party_SearchResults response;
    CMsgGCCStrike15_v2_Party_SearchResults::Entry *entry = response.add_entries();
    entry->set_id(AccountId());
    entry->set_grp(3);
    entry->set_game_type(message.game_type());
    entry->set_apr(1);
    entry->set_ark(std::rand() % 18 + 1);
    entry->set_loc(30066);
    entry->set_accountid(EffectiveAccountId());

    for (uint32_t player_id : GetConfig().GetFriends()) {
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
    if (!messageRead.ReadProtobuf(message)) return;
    CMsgGCCStrike15_v2_Account_RequestCoPlays_Player *player = message.add_players();
    player->set_accountid(EffectiveAccountId());
    player->set_online(true);
    player->set_rtcoplay(1771263169);
    for (uint32_t player_id : GetConfig().GetFriends()) {
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
    if (!messageRead.ReadProtobuf(message)) return;

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
    CMsgCasketItem message; (void)message;
    CMsgGCItemCustomizationNotification notification;
    notification.set_request(k_EGCItemCustomizationNotification_CasketContents);
    notification.add_item_id(message.casket_item_id());
    SendMessageToGame(false, k_EMsgGCItemCustomizationNotification, notification);
}

void ClientGC::CasketItemAdd(GCMessageRead &messageRead)
{
    CMsgCasketItem message;
    if (!messageRead.ReadProtobuf(message)) return;
    CMsgSOSingleObject updateItem, updateCasket;
    CMsgGCItemCustomizationNotification notification;
    if (m_inventory.CasketItemAdd(message.casket_item_id(), message.item_item_id(), updateItem, updateCasket, notification)) {
        SendMessageToGame(false, k_ESOMsg_Update, updateItem);
        SendMessageToGame(false, k_ESOMsg_Update, updateCasket);
        SendMessageToGame(false, k_EMsgGCItemCustomizationNotification, notification);
    }
}

void ClientGC::CasketItemExtract(GCMessageRead &messageRead)
{
    CMsgCasketItem message;
    if (!messageRead.ReadProtobuf(message)) return;
    CMsgSOSingleObject updateItem, updateCasket;
    CMsgGCItemCustomizationNotification notification;
    if (m_inventory.CasketItemRemove(message.casket_item_id(), message.item_item_id(), updateItem, updateCasket, notification)) {
        SendMessageToGame(false, k_ESOMsg_Update, updateItem);
        SendMessageToGame(false, k_ESOMsg_Update, updateCasket);
        SendMessageToGame(false, k_EMsgGCItemCustomizationNotification, notification);
    }
}

void ClientGC::StatTrakSwap(GCMessageRead &messageRead)
{
    CMsgApplyStatTrakSwap message;
    if (!messageRead.ReadProtobuf(message)) return;
    CMsgSOSingleObject destroy, updateItem1, updateItem2;
    CMsgGCItemCustomizationNotification notification;
    if (m_inventory.StatTrakSwap(message.tool_item_id(), message.item_1_item_id(), message.item_2_item_id(),
        destroy, updateItem1, updateItem2, notification)) {
        SendMessageToGame(true, k_ESOMsg_Destroy, destroy);
        SendMessageToGame(true, k_ESOMsg_Update, updateItem1);
        SendMessageToGame(true, k_ESOMsg_Update, updateItem2);
        SendMessageToGame(false, k_EMsgGCItemCustomizationNotification, notification);
    }
}

void ClientGC::DeleteItem(GCMessageRead &messageRead)
{
    uint64_t itemId = messageRead.ReadUint64();
    CMsgSOSingleObject destroyed;
    if (m_inventory.RemoveItem(itemId, destroyed))
        SendMessageToGame(true, k_ESOMsg_Destroy, destroyed);
}

void ClientGC::UnlockCrate(GCMessageRead &messageRead)
{
    uint64_t keyId = messageRead.ReadUint64();
    uint64_t crateId = messageRead.ReadUint64();
    CMsgSOSingleObject destroyCrate, destroyKey, newItem;
    CMsgGCItemCustomizationNotification notification;
    if (m_inventory.UnlockCrate(crateId, keyId, destroyCrate, destroyKey, newItem, notification)) {
        SendMessageToGame(true, k_ESOMsg_Destroy, destroyCrate);
        SendMessageToGame(true, k_ESOMsg_Destroy, destroyKey);
        SendMessageToGame(true, k_ESOMsg_Create, newItem);
        SendMessageToGame(false, k_EMsgGCItemCustomizationNotification, notification);
    }
}

void ClientGC::NameItem(GCMessageRead &messageRead)
{
    uint64_t nameTagId = messageRead.ReadUint64();
    uint64_t itemId = messageRead.ReadUint64();
    messageRead.ReadData(1);
    std::string_view name = messageRead.ReadString();
    CMsgSOSingleObject update, destroy;
    CMsgGCItemCustomizationNotification notification;
    if (m_inventory.NameItem(nameTagId, itemId, name, update, destroy, notification)) {
        SendMessageToGame(true, k_ESOMsg_Update, update);
        SendMessageToGame(true, k_ESOMsg_Destroy, destroy);
        SendMessageToGame(false, k_EMsgGCItemCustomizationNotification, notification);
    }
}

void ClientGC::NameBaseItem(GCMessageRead &messageRead)
{
    uint64_t nameTagId = messageRead.ReadUint64();
    uint32_t defIndex = messageRead.ReadUint32();
    messageRead.ReadData(1);
    std::string_view name = messageRead.ReadString();
    CMsgSOSingleObject create, destroy;
    CMsgGCItemCustomizationNotification notification;
    if (m_inventory.NameBaseItem(nameTagId, defIndex, name, create, destroy, notification)) {
        SendMessageToGame(true, k_ESOMsg_Create, create);
        SendMessageToGame(true, k_ESOMsg_Destroy, destroy);
        SendMessageToGame(false, k_EMsgGCItemCustomizationNotification, notification);
    }
}

void ClientGC::RemoveItemName(GCMessageRead &messageRead)
{
    uint64_t itemId = messageRead.ReadUint64();
    CMsgSOSingleObject update, destroy;
    CMsgGCItemCustomizationNotification notification;
    if (m_inventory.RemoveItemName(itemId, update, destroy, notification)) {
        if (update.has_type_id()) SendMessageToGame(true, k_ESOMsg_Update, update);
        if (destroy.has_type_id()) SendMessageToGame(true, k_ESOMsg_Destroy, destroy);
        SendMessageToGame(false, k_EMsgGCItemCustomizationNotification, notification);
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
    UpdateCooldown();
}

void ClientGC::ReloadInventory() { m_inventory.ReloadFromFile(); SendInventoryUpdate(); }
void ClientGC::ReloadConfig() {}
void ClientGC::ReloadPriceSheet() {}
void ClientGC::ReloadPasses() {}
void ClientGC::ReloadUnusualLootLists() {}

void ClientGC::FetchOverwatchCases()
{
    ISteamHTTP *http = SteamHTTP();
    if (!http) return;
    HTTPRequestHandle hRequest = http->CreateHTTPRequest(k_EHTTPMethodGET, "https://sasha190409.github.io/csgo/overwatch");
    if (hRequest == k_uAPICallInvalid) return;
    http->SetHTTPRequestHeaderValue(hRequest, "User-Agent", "csgo_gc/1.0");
    http->SendHTTPRequest(hRequest, nullptr);
}

void ClientGC::OnOverwatchHTTPResponse(HTTPRequestCompleted_t *pCallback)
{
    if (!pCallback->m_bRequestSuccessful || pCallback->m_eStatusCode != 200) return;
    ISteamHTTP *http = SteamHTTP();
    if (!http) return;
    uint32_t bodySize;
    if (!http->GetHTTPResponseBodySize(pCallback->m_hRequest, &bodySize) || bodySize == 0) return;
    std::vector<uint8_t> body(bodySize + 1, 0);
    if (!http->GetHTTPResponseBodyData(pCallback->m_hRequest, body.data(), bodySize)) return;
    std::string json(reinterpret_cast<char*>(body.data()), bodySize);

    std::vector<uint32_t> suspects;
    size_t start = json.find('{');
    if (start == std::string::npos) return;
    size_t end = json.rfind('}');
    if (end == std::string::npos || end <= start) return;
    std::string content = json.substr(start + 1, end - start - 1);

    size_t comma = 0;
    while (comma != std::string::npos) {
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
        if (key.find("case") == 0 && !value.empty()) {
            uint32_t accId = SteamIDStringToAccountId(value);
            if (accId != 0) suspects.push_back(accId);
        }
        comma = (next == std::string::npos) ? std::string::npos : next + 1;
    }

    {
        std::lock_guard<std::mutex> lock(m_overwatchMutex);
        m_overwatchSuspects = std::move(suspects);
        m_nextOverwatchIndex = 0;
    }
}

uint32_t ClientGC::SteamIDStringToAccountId(const std::string &str)
{
    unsigned int x, y;
    if (sscanf(str.c_str(), "STEAM_%*u:%u:%u", &x, &y) != 2) return 0;
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
    std::string url = "https://sasha190409.github.io/csgo/demos/imposter_" + std::to_string(suspectAccountId) + ".dem";
    assignment.set_caseurl(url);
    SendMessageToGame(false, k_EMsgGCCStrike15_v2_PlayerOverwatchCaseAssignment, assignment);
}

void ClientGC::OnOverwatchCaseStatus(GCMessageRead& messageRead)
{
    CMsgGCCStrike15_v2_PlayerOverwatchCaseStatus msg;
    if (!messageRead.ReadProtobuf(msg)) return;
    if (msg.caseid() == 0) {
        std::lock_guard<std::mutex> lock(m_overwatchMutex);
        if (m_overwatchSuspects.empty()) return;
        uint32_t suspect = m_overwatchSuspects[m_nextOverwatchIndex];
        m_nextOverwatchIndex = (m_nextOverwatchIndex + 1) % m_overwatchSuspects.size();
        SendOverwatchCaseAssignment(suspect);
    }
}

void ClientGC::OnOverwatchCaseUpdate(GCMessageRead& messageRead)
{
    CMsgGCCStrike15_v2_PlayerOverwatchCaseUpdate msg;
    if (!messageRead.ReadProtobuf(msg)) return;
    SendVerdictToCloudflare(msg);
}

void ClientGC::SendVerdictToCloudflare(const CMsgGCCStrike15_v2_PlayerOverwatchCaseUpdate &msg)
{
    ISteamHTTP *http = SteamHTTP();
    if (!http) return;
    std::string json = "{\"caseid\":" + std::to_string(msg.caseid()) + ",\"suspectid\":" + std::to_string(msg.suspectid()) + ",\"reason\":" + std::to_string(msg.reason()) + ",\"timestamp\":" + std::to_string(time(nullptr)) + "}";
    HTTPRequestHandle hRequest = http->CreateHTTPRequest(k_EHTTPMethodPOST, "https://still-dawn-e090.ivanhihlov4.workers.dev/verdict");
    if (hRequest == k_uAPICallInvalid) return;
    http->SetHTTPRequestHeaderValue(hRequest, "Content-Type", "application/json");
    std::vector<uint8_t> postData(json.begin(), json.end());
    http->SetHTTPRequestRawPostBody(hRequest, "application/json", postData.data(), static_cast<uint32_t>(postData.size()));
    http->SendHTTPRequest(hRequest, nullptr);
}

uint32_t ClientGC::EffectiveAccountId() const { return AccountId(); }
