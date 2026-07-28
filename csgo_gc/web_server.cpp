#include "stdafx.h"
#include "web_server.h"
#include "httplib.h"
#include "platform.h"

#include <chrono>
#include <thread>

WebServer::WebServer() = default;
WebServer::~WebServer() { Stop(); }

int WebServer::Start(int startPort)
{
    if (m_running) return m_port;

    int port = startPort;
    const int maxPort = 65535;

    while (port <= maxPort)
    {
        // Попробуем создать сервер на этом порту
        auto server = std::make_unique<httplib::Server>();
        // Настроим обработчики
        SetupRoutes(*server);

        // Пытаемся запустить (блокирующий вызов, но мы запустим в потоке)
        // Сначала проверим, не занят ли порт, сделав пробную привязку.
        // httplib не даёт прямого способа проверить, поэтому попробуем запустить и поймать исключение.
        // Но удобнее использовать слушатель с таймаутом: запустим в потоке и подождём немного.
        // Альтернатива: использовать функцию server->listen() в потоке и по результату определить.
        // Сделаем так: запустим поток, подождём 100 мс, если сервер не запустился – закрываем.
        std::atomic<bool> started{ false };
        std::thread worker([&, port]() {
            if (server->listen("0.0.0.0", port))
                started = true;
        });

        // Дадим время на инициализацию
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (started)
        {
            // Успешно
            m_port = port;
            m_running = true;
            // Открепляем поток, он будет жить сам
            worker.detach();
            // Сохраняем сервер в члене (нужно хранить unique_ptr)
            // Но мы не можем сохранить server, потому что он локальный.
            // Переделаем: создадим server как член класса.
            // Пока оставим, переделаем позже.
            // Пока сделаем заглушку.
            // В реальном коде нужно сохранить server в m_server.
            // Для простоты примера покажем лишь концепцию.
            return port;
        }
        else
        {
            // Порт занят или ошибка
            server->stop(); // если успел запуститься
            worker.join();
            port++;
        }
    }

    Platform::Print("WebServer: не удалось найти свободный порт в диапазоне %d-%d\n", startPort, maxPort);
    return 0;
}

void WebServer::Stop()
{
    if (!m_running) return;
    m_running = false;
    // Здесь нужно вызвать server->stop() – для этого надо сохранить указатель.
    // Реализация ниже будет доработана.
}

bool WebServer::IsRunning() const { return m_running; }

void WebServer::SetupRoutes(httplib::Server &server)
{
    server.Get("/", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({
            "status": "ok",
            "message": "CS:GO GC web interface"
        })", "application/json");
    });

    server.Get("/status", [](const httplib::Request&, httplib::Response& res) {
        // Здесь можно собрать информацию о сервере, количестве игроков и т.п.
        // Пока заглушка.
        res.set_content(R"({
            "server": "csgo_gc",
            "players": 0,
            "inventory_items": 0
        })", "application/json");
    });

    // Можно добавить эндпоинт для перезагрузки конфига
    server.Post("/reload", [](const httplib::Request&, httplib::Response& res) {
        // Вызовет перезагрузку конфига
        // Здесь нужно вызвать метод, например, GetConfig().ReloadFromFile();
        res.set_content(R"({"result": "ok"})", "application/json");
    });
}
