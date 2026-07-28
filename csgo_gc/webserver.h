#pragma once

#include <atomic>
#include <thread>
#include <string>

class WebServer
{
public:
    WebServer();
    ~WebServer();

    // Запустить сервер, вернуть номер порта, на котором запустились (0 при ошибке)
    int Start(int startPort = 8080);
    void Stop();

    bool IsRunning() const;

private:
    void WorkerThread(int port);
    void SetupRoutes();

    std::thread m_thread;
    std::atomic<bool> m_running{ false };
    int m_port{ 0 };
};
