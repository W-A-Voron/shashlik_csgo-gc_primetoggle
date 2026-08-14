#include "stdafx.h"
#include "config.h"

GCConfig::GCConfig()
{
    // Все поля инициализированы прямо в заголовке, здесь ничего не нужно делать.
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
