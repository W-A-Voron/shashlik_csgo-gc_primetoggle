// steam_hook.cpp
#include "stdafx.h"
#include "steam_hook.h"
#include "appid.h"
#include "gc_client.h"
#include "gc_server.h"
#include "platform.h"
#include <funchook.h>

// ... (остальные инклуды и дефайны остаются без изменений) ...

// ============================================================
// ПРОКСИ ДЛЯ ПОДМЕНЫ СТАТИСТИКИ
// ============================================================

// Реалистичная статистика (не круглые числа)
constexpr int REAL_TOTAL_GAMES = 307;       // Всего игр
constexpr int REAL_TOTAL_WINS = 151;        // Побед (как в ранге)
constexpr int REAL_TOTAL_LOSSES = 156;      // Поражений
constexpr int REAL_TOTAL_KILLS = 2147;      // Всего убийств
constexpr int REAL_TOTAL_DEATHS = 2147;     // Всего смертей
constexpr int REAL_TOTAL_ASSISTS = 2147;    // Всего ассистов

// Имена статистик, которые использует CS:GO 360
const char* STAT_COMPETITIVE_WINS = "competitive_wins";
const char* STAT_WINGMAN_WINS = "wingman_wins";
const char* STAT_DANGERZONE_WINS = "dangerzone_wins";
const char* STAT_TOTAL_KILLS = "total_kills";
const char* STAT_TOTAL_DEATHS = "total_deaths";
const char* STAT_TOTAL_ASSISTS = "total_assists";
const char* STAT_TOTAL_GAMES = "total_games_played";
const char* STAT_TOTAL_WINS = "total_wins";
const char* STAT_TOTAL_LOSSES = "total_losses";

class SteamUserStatsProxy : public ISteamUserStats
{
    ISteamUserStats *const m_original;

public:
    SteamUserStatsProxy(ISteamUserStats *original) : m_original(original) {}

    // Перехват запроса статистики
    bool GetStat(const char *pchName, int32 *pData) override
    {
        // Если это наша статистика, возвращаем реалистичные значения
        if (strcmp(pchName, STAT_COMPETITIVE_WINS) == 0) {
            *pData = REAL_TOTAL_WINS;
            return true;
        }
        if (strcmp(pchName, STAT_WINGMAN_WINS) == 0) {
            *pData = 52; // ваши данные
            return true;
        }
        if (strcmp(pchName, STAT_DANGERZONE_WINS) == 0) {
            *pData = 37;
            return true;
        }
        if (strcmp(pchName, STAT_TOTAL_KILLS) == 0) {
            *pData = REAL_TOTAL_KILLS;
            return true;
        }
        if (strcmp(pchName, STAT_TOTAL_DEATHS) == 0) {
            *pData = REAL_TOTAL_DEATHS;
            return true;
        }
        if (strcmp(pchName, STAT_TOTAL_ASSISTS) == 0) {
            *pData = REAL_TOTAL_ASSISTS;
            return true;
        }
        if (strcmp(pchName, STAT_TOTAL_GAMES) == 0) {
            *pData = REAL_TOTAL_GAMES;
            return true;
        }
        if (strcmp(pchName, STAT_TOTAL_WINS) == 0) {
            *pData = REAL_TOTAL_WINS;
            return true;
        }
        if (strcmp(pchName, STAT_TOTAL_LOSSES) == 0) {
            *pData = REAL_TOTAL_LOSSES;
            return true;
        }

        // Иначе передаём оригиналу
        return m_original->GetStat(pchName, pData);
    }

    bool GetStat(const char *pchName, float *pData) override
    {
        return m_original->GetStat(pchName, pData);
    }

    bool SetStat(const char *pchName, int32 nData) override
    {
        return m_original->SetStat(pchName, nData);
    }

    bool SetStat(const char *pchName, float fData) override
    {
        return m_original->SetStat(pchName, fData);
    }

    bool RequestCurrentStats() override
    {
        if (!AppId::IsOriginal()) {
            Platform::Print("Spoofing RequestCurrentStats\n");
            QueueUserStatsCallback(); // отправляем фейковый callback
            return true;
        }
        return m_original->RequestCurrentStats();
    }

    // ... (остальные методы проксируются через m_original) ...
};

// ============================================================
// Остальной код steam_hook.cpp (без изменений, кроме добавления
// прокси для ISteamUserStats в SteamInterfaceProxy)
// ============================================================

// В классе SteamInterfaceProxy добавляем:
void *GetInterface(const char *version, void *original)
{
    if (InterfaceMatches(version, STEAMGAMECOORDINATOR_INTERFACE_VERSION)) {
        uint64_t steamId = (SteamGameServer_GetHSteamPipe() != m_pipe) ? SteamUser()->GetSteamID().ConvertToUint64() : 0;
        return GetOrCreate<ISteamGameCoordinator>(m_steamGameCoordinator, steamId);
    }
    else if (InterfaceMatches(version, STEAMUTILS_INTERFACE_VERSION)) {
        return GetOrCreate<ISteamUtils>(m_steamUtils, static_cast<ISteamUtils *>(original));
    }
    else if (InterfaceMatches(version, STEAMUSERSTATS_INTERFACE_VERSION)) {
        return GetOrCreate<ISteamUserStats>(m_steamUserStats, static_cast<ISteamUserStats *>(original)); // <-- ВАЖНО!
    }
    // ... остальные интерфейсы ...
}
