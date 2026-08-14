#pragma once

#include "gc_const_csgo.h"
#include "item_schema.h"

struct RarityWeight
{
    uint32_t rarity;
    float weight;
};

class GCConfig
{
public:
    GCConfig();
    void ReloadFromFile() {} // Файлы не используются

    // === ЖЁСТКИЕ НАСТРОЙКИ ===
    uint32_t AppIdOverride() const { return m_appIdOverride; }
    bool ShowCsgoGCServersOnly() const { return m_showCsgoGCServersOnly; }

    // === РЕАЛИСТИЧНЫЙ РАНГ ===
    RankId CompetitiveRank() const { return m_competitiveRank; }
    int CompetitiveWins() const { return m_competitiveWins; }

    RankId WingmanRank() const { return m_wingmanRank; }
    int WingmanWins() const { return m_wingmanWins; }

    DangerZoneRankId DangerZoneRank() const { return m_dangerZoneRank; }
    int DangerZoneWins() const { return m_dangerZoneWins; }

    // === НАСТРОЙКИ ИГРЫ ===
    bool ForceMaxRarity() const { return m_forceMaxRarity; }
    bool DestroyUsedItems() const { return m_destroyUsedItems; }
    bool RandomizeFloat() const { return m_randomizeFloat; }

    bool VacBanned() const { return m_vacBanned; }
    bool HasPrime() const { return m_hasPrime; }
    uint32_t CompetitiveCooldownSeconds() const { return m_competitiveCooldownSeconds; }

    // === РЕАЛИСТИЧНЫЕ ДАННЫЕ ===
    int CommendedFriendly() const { return m_commendedFriendly; }
    int CommendedTeaching() const { return m_commendedTeaching; }
    int CommendedLeader() const { return m_commendedLeader; }
    int Level() const { return m_level; }
    int Xp() const { return m_xp; }

    std::string Country() const { return m_country; }
    int Currency() const { return m_currency; }

    float GetRarityWeight(uint32_t rarity) const;
    std::vector<int> GetFriends() const { return m_friends; };

private:
    // Настройки
    uint32_t m_appIdOverride = 4465480;
    bool m_showCsgoGCServersOnly = true;

    // Ранги
    RankId m_competitiveRank = RankMasterGuardian1;
    int m_competitiveWins = 98;
    RankId m_wingmanRank = RankMasterGuardian1;
    int m_wingmanWins = 42;
    DangerZoneRankId m_dangerZoneRank = DangerZoneRankScoutElite;
    int m_dangerZoneWins = 15;

    // Игровые настройки
    bool m_forceMaxRarity = false;
    bool m_destroyUsedItems = true;
    bool m_randomizeFloat = true;
    bool m_vacBanned = false;
    bool m_hasPrime = true;
    uint32_t m_competitiveCooldownSeconds = 0;

    // Статистика
    int m_commendedFriendly = 12;
    int m_commendedTeaching = 5;
    int m_commendedLeader = 3;
    int m_level = 28;
    int m_xp = 3200;

    std::string m_country = "RU";
    int m_currency = 3;

    // Друзья
    std::vector<int> m_friends = { 1140104601, 7656119801234567, 7656119807654321 };

    // Веса (объявление без инициализации, инициализируется в .cpp)
    std::vector<RarityWeight> m_rarityWeights;
};

GCConfig &GetConfig();
