#include "stdafx.h"
#include "config.h"

GCConfig::GCConfig()
{
    // Включение удаления использованных предметов (кейсы, ключи, ярлыки)
    m_destroyUsedItems = true;

    // Выключение принудительного выпадения самых редких предметов
    m_forceMaxRarity = false;

    // Включение рандомизации износа (float)
    m_randomizeFloat = true;

    // Кулдаун по умолчанию (0 = отключён)
    m_competitiveCooldownSeconds = 0;

    // Реалистичные веса выпадения для каждого уровня редкости
    m_rarityWeights = {
        { ItemSchema::RarityCommon,   10000000 }, // 10 млн  (обычные)
        { ItemSchema::RarityUncommon, 2000000  }, // 2 млн   (необычные)
        { ItemSchema::RarityRare,     400000   }, // 400 тыс (редкие)
        { ItemSchema::RarityMythical, 80000    }, // 80 тыс  (мифические)
        { ItemSchema::RarityLegendary,16000    }, // 16 тыс  (легендарные)
        { ItemSchema::RarityAncient,  3200     }, // 3.2 тыс (древние)
        { ItemSchema::RarityImmortal, 640      }, // 640     (бессмертные - ножи)
        { ItemSchema::RarityUnusual,  1280     }, // 1280    (необычные - ножи с паттерном)
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
