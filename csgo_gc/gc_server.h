#pragma once

#include "gc_shared.h"
#include <unordered_map>

class ServerGC final : public SharedGC
{
NetworkingServer* m_networking = nullptr;
public:
    ServerGC();
    ~ServerGC();

    bool CanHandleNetMessages() const { return m_receivedHello.load(std::memory_order_acquire); }

    // New public methods
    void SetNetworking(NetworkingServer* net) { m_networking = net; }
    void CheckPendingReservations();          // called from the main loop
    void SetNetworking(NetworkingServer* net) { m_networking = net; }

private:
    ServerGC();
    ~ServerGC();

    bool CanHandleNetMessages() const { return m_receivedHello.load(std::memory_order_acquire); }

private:
    void HandleEvent(GCEvent type, uint64_t id, const std::vector<uint8_t> &buffer) override;

    // event handlers
    void HandleMessage(uint32_t type, const void *data, uint32_t size);
    void HandleNetMessage(uint64_t steamId, const void *data, uint32_t size);
    void HandleClientSOCacheUnsubscribe(uint64_t steamId);

    void OnServerHello(GCMessageRead &messageRead);
    void IncrementKillCountAttribute(GCMessageRead &messageRead);

    std::atomic_bool m_receivedHello{};
    void OnMatchmakingServerReservationResponse(GCMessageRead &messageRead);
    struct PendingReservation {
        uint64_t exchange;
        uint32_t token;
        uint32_t stamp;
    };

    std::unordered_map<uint64_t, PendingReservation> m_pendingReservations;
    NetworkingServer* m_networking = nullptr;   // set via SetNetworking

    void SendConfirmToClient(uint64_t clientId, const PendingReservation& res);
};
