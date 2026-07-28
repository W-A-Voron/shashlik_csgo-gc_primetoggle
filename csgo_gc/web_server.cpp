#include "stdafx.h"
#include "web_server.h"
#include "httplib.h"
#include "platform.h"
#include "config.h" // если нужен доступ к конфигу

#include <chrono>
#include <thread>

WebServer::WebServer() = default;
WebServer::~WebServer() { Stop(); }

void WebServer::WorkerThread(int port)
{
    if (!m_server) return;
    if (!m_server->listen("0.0.0.0", port))
    {
        Platform::Print("WebServer: не удалось запустить на порту %d\n", port);
        m_running = false;
    }
    // listen блокирует, пока сервер не остановят
}

int WebServer::Start(int startPort)
{
    if (m_running) return m_port;

    int port = startPort;
    const int maxPort = 65535;

    while (port <= maxPort)
    {
        auto server = std::make_unique<httplib::Server>();
        // Настроим маршруты
        server->Get("/", [](const httplib::Request&, httplib::Response& res) {
            res.set_content(R"({"status":"ok","message":"CS:GO GC web interface"})", "application/json");
        });
        server->Get("/status", [](const httplib::Request&, httplib::Response& res) {
            // Здесь можно подставить реальные данные
            res.set_content(R"({"players":0,"items":0})", "application/json");
        });

        // Проверим порт, запустив поток с таймаутом.
        // Идея: запустить listen в отдельном потоке и подождать 100 мс, если не упал – считаем успех.
        std::atomic<bool> started{ false };
        std::thread worker([&server, port, &started]() {
            if (server->listen("0.0.0.0", port))
                started = true;
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(150));

        if (started)
        {
            // Успешно запустился
            m_server = std::move(server);
            m_port = port;
            m_running = true;
            // Открепляем поток, он будет жить, пока не вызовут stop()
            worker.detach();
            // Показываем попап
            std::string msg = "Server is running at http://localhost:" + std::to_string(port) + "/";
            Platform::Notify(msg.c_str()); // см. ниже
            return port;
        }
        else
        {
            // Возможно, порт занят; остановим сервер и поток
            server->stop();
            worker.join();
            port++;
        }
    }

    Platform::Print("WebServer: не найден свободный порт в диапазоне %d-%d\n", startPort, maxPort);
    return 0;
}

void WebServer::Stop()
{
    if (!m_running) return;
    m_running = false;
    if (m_server)
    {
        m_server->stop();
        if (m_thread.joinable())
            m_thread.join();
    }
    m_server.reset();
    m_port = 0;
}
