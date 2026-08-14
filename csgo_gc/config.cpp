#include "stdafx.h"
#include "config.h"

GCConfig::GCConfig()
{
    // Инициализация вектора весов прямо в конструкторе — это безопасно для MSVC
    m_rarityWeights = {
        { ItemSchema::RarityCommon, 10000000.0f },
        { ItemSchema::RarityUncommon, 2000000.0f },
        { ItemSchema::RarityRare, 400000.0f },
        { ItemSchema::RarityMythical, 80000.0f },
        { ItemSchema::RarityLegendary, 16000.0f },
        { ItemSchema::RarityAncient, 3200.0f },
        { ItemSchema::RarityImmortal, 640.0f },
        { ItemSchema::RarityUnusual, 1280.0f },
    };
}

float GCConfig::GetRarityWeight(uint32_t rarity) const
{
    for (const RarityWeight &weight : m_rarityWeights)
    {
        if (weight.rarity == rarity)
        {
            return weight.weight;
        }
    }
    return 0;
}

GCConfig &GetConfig()
{
    static GCConfig config;
    return config;
}
