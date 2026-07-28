#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <string>

namespace httplib { class Server; }

class WebServer
{
public:
    WebServer();
    ~WebServer();

    int Start(int startPort = 8080);
    void Stop();

private:
    void WorkerThread(int port);
    void SetupRoutes(httplib::Server& srv);   // теперь принимает ссылку

    static bool IsAllowedFile(const std::string& filename);
    static std::string ReadFile(const std::string& path);
    static bool WriteFile(const std::string& path, const std::string& content);

    std::unique_ptr<httplib::Server> m_server;
    std::thread m_thread;
    std::atomic<bool> m_running{ false };
    int m_port{ 0 };
};
