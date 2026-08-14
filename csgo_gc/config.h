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
    uint32_t AppIdOverride() const { return 4465480; }
    bool ShowCsgoGCServersOnly() const { return true; }

    // === РЕАЛИСТИЧНЫЙ РАНГ (Золотая Звезда 3 / Master Guardian 1) ===
    RankId CompetitiveRank() const { return RankMasterGuardian1; }
    int CompetitiveWins() const { return 98; }

    RankId WingmanRank() const { return RankMasterGuardian1; }
    int WingmanWins() const { return 42; }

    DangerZoneRankId DangerZoneRank() const { return DangerZoneRankScoutElite; }
    int DangerZoneWins() const { return 15; }

    // === НАСТРОЙКИ ИГРЫ ===
    bool ForceMaxRarity() const { return false; }       // Реальные шансы выпадения
    bool DestroyUsedItems() const { return true; }      // Предметы исчезают после использования
    bool RandomizeFloat() const { return true; }

    bool VacBanned() const { return false; }
    bool HasPrime() const { return true; }
    uint32_t CompetitiveCooldownSeconds() const { return 0; }

    // === РЕАЛИСТИЧНЫЕ ДАННЫЕ СТАТИСТИКИ ===
    int CommendedFriendly() const { return 12; }   // Дружелюбный
    int CommendedTeaching() const { return 5; }    // Наставник
    int CommendedLeader() const { return 3; }      // Лидер
    int Level() const { return 28; }               // 28 уровень
    int Xp() const { return 3200; }                // 3200/5000 до следующего уровня

    std::string Country() const { return "RU"; }
    int Currency() const { return 3; } // RUB

    // === ШАНСЫ ВЫПАДЕНИЯ (как в реальном CS:GO) ===
    float GetRarityWeight(uint32_t rarity) const;

    std::vector<int> GetFriends() const { return { 1140104601, 7656119801234567, 7656119807654321 }; };

private:
    bool m_forceMaxRarity;
    bool m_destroyUsedItems;
    bool m_randomizeFloat;
    uint32_t m_competitiveCooldownSeconds;

    std::vector<RarityWeight> m_rarityWeights;
};

GCConfig &GetConfig();
