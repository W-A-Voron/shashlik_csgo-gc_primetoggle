#include "stdafx.h"
#include "web_server.h"
#include "httplib.h"
#include "platform.h"

#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>

static const std::vector<std::string> AllowedFiles = {
    "config.txt",
    "inventory.txt",
    "price_sheet.txt",
    "passes.txt",
    "unusual_loot_lists.txt"
};

static const char* HTML_PAGE = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>CS:GO GC Config Editor</title>
    <style>
        body { font-family: sans-serif; margin: 20px; background: #1e1e2e; color: #cdd6f4; }
        .container { max-width: 900px; margin: auto; }
        h1 { color: #89b4fa; }
        .file-selector { margin: 20px 0; }
        select, button { padding: 8px 12px; font-size: 14px; background: #313244; color: #cdd6f4; border: 1px solid #45475a; border-radius: 4px; }
        button { cursor: pointer; background: #89b4fa; color: #1e1e2e; font-weight: bold; }
        button:hover { background: #74c7ec; }
        textarea { width: 100%; height: 500px; margin: 10px 0; background: #11111b; color: #cdd6f4; border: 1px solid #45475a; border-radius: 4px; font-family: monospace; padding: 8px; }
        .status { margin: 5px 0; padding: 8px; border-radius: 4px; }
        .status.success { background: #a6e3a1; color: #1e1e2e; }
        .status.error { background: #f38ba8; color: #1e1e2e; }
        .actions { display: flex; gap: 10px; flex-wrap: wrap; }
    </style>
</head>
<body>
<div class="container">
    <h1>CS:GO GC – Config Editor</h1>
    <div class="file-selector">
        <label for="fileSelect">Select file: </label>
        <select id="fileSelect">
            <option value="config.txt">config.txt</option>
            <option value="inventory.txt">inventory.txt</option>
            <option value="price_sheet.txt">price_sheet.txt</option>
            <option value="passes.txt">passes.txt</option>
            <option value="unusual_loot_lists.txt">unusual_loot_lists.txt</option>
        </select>
        <button id="loadBtn">Load</button>
        <button id="saveBtn">Save</button>
        <span id="status" class="status"></span>
    </div>
    <textarea id="editor" spellcheck="false"></textarea>
    <div class="actions">
        <button id="reloadConfigBtn">Reload Config (in-game)</button>
        <button id="reloadInventoryBtn">Reload Inventory (in-game)</button>
    </div>
</div>
<script>
    const fileSelect = document.getElementById('fileSelect');
    const editor = document.getElementById('editor');
    const status = document.getElementById('status');
    const loadBtn = document.getElementById('loadBtn');
    const saveBtn = document.getElementById('saveBtn');

    function setStatus(msg, isError) {
        status.textContent = msg;
        status.className = 'status ' + (isError ? 'error' : 'success');
        setTimeout(() => { status.className = 'status'; }, 5000);
    }

    async function loadFile(name) {
        try {
            const resp = await fetch('/file?name=' + encodeURIComponent(name));
            if (!resp.ok) {
                const text = await resp.text();
                setStatus('Error loading: ' + text, true);
                return;
            }
            const content = await resp.text();
            editor.value = content;
            setStatus('Loaded ' + name + ' (' + content.length + ' bytes)', false);
        } catch (e) {
            setStatus('Error: ' + e.message, true);
        }
    }

    async function saveFile(name) {
        const content = editor.value;
        try {
            const resp = await fetch('/file?name=' + encodeURIComponent(name), {
                method: 'POST',
                headers: { 'Content-Type': 'text/plain' },
                body: content
            });
            if (!resp.ok) {
                const text = await resp.text();
                setStatus('Error saving: ' + text, true);
                return;
            }
            setStatus('Saved ' + name + ' (' + content.length + ' bytes)', false);
        } catch (e) {
            setStatus('Error: ' + e.message, true);
        }
    }

    loadBtn.addEventListener('click', () => loadFile(fileSelect.value));
    saveBtn.addEventListener('click', () => saveFile(fileSelect.value));

    window.addEventListener('load', () => loadFile(fileSelect.value));

    document.getElementById('reloadConfigBtn').addEventListener('click', () => {
        fetch('/reload?type=config', { method: 'POST' })
            .then(r => r.text())
            .then(msg => setStatus(msg, false))
            .catch(e => setStatus('Error: ' + e.message, true));
    });
    document.getElementById('reloadInventoryBtn').addEventListener('click', () => {
        fetch('/reload?type=inventory', { method: 'POST' })
            .then(r => r.text())
            .then(msg => setStatus(msg, false))
            .catch(e => setStatus('Error: ' + e.message, true));
    });
</script>
</body>
</html>
)";

WebServer::WebServer() = default;
WebServer::~WebServer() { Stop(); }

int WebServer::Start(int startPort)
{
    if (m_running) return m_port;

    m_server = std::make_unique<httplib::Server>();
    SetupRoutes(*m_server);

    m_running = true;
    m_port = startPort;

    m_thread = std::thread([this, startPort]() {
        if (!m_server->listen("0.0.0.0", startPort)) {
            Platform::Print("WebServer: failed to listen on port %d\n", startPort);
            m_running = false;
        } else {
            Platform::Print("WebServer: running on port %d\n", startPort);
        }
    });

    return startPort;
}

void WebServer::Stop()
{
    if (!m_running) return;
    m_running = false;
    if (m_server) {
        m_server->stop();
    }
    if (m_thread.joinable()) {
        m_thread.join();
    }
    m_server.reset();
    m_port = 0;
}

void WebServer::SetupRoutes(httplib::Server& srv)
{
    srv.Get("/", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(HTML_PAGE, "text/html");
    });

    srv.Get("/file", [](const httplib::Request& req, httplib::Response& res) {
        auto name = req.get_param_value("name");
        if (name.empty()) {
            res.status = 400;
            res.set_content("Missing 'name' parameter", "text/plain");
            return;
        }
        if (!IsAllowedFile(name)) {
            res.status = 403;
            res.set_content("Forbidden file name", "text/plain");
            return;
        }
        std::string path = "csgo_gc/" + name;
        std::string content = ReadFile(path);
        if (content.empty()) {
            res.status = 404;
            res.set_content("File not found or empty", "text/plain");
            return;
        }
        res.set_content(content, "text/plain");
    });

    srv.Post("/file", [](const httplib::Request& req, httplib::Response& res) {
        auto name = req.get_param_value("name");
        if (name.empty()) {
            res.status = 400;
            res.set_content("Missing 'name' parameter", "text/plain");
            return;
        }
        if (!IsAllowedFile(name)) {
            res.status = 403;
            res.set_content("Forbidden file name", "text/plain");
            return;
        }
        std::string path = "csgo_gc/" + name;
        if (!WriteFile(path, req.body)) {
            res.status = 500;
            res.set_content("Failed to write file", "text/plain");
            return;
        }
        res.set_content("OK", "text/plain");
    });

    srv.Post("/reload", [](const httplib::Request& req, httplib::Response& res) {
        auto type = req.get_param_value("type");
        if (type == "config") {
            res.set_content("Config reload requested (implement in ClientGC)", "text/plain");
        } else if (type == "inventory") {
            res.set_content("Inventory reload requested (implement in ClientGC)", "text/plain");
        } else {
            res.status = 400;
            res.set_content("Invalid type. Use 'config' or 'inventory'", "text/plain");
        }
    });
}

bool WebServer::IsAllowedFile(const std::string& filename)
{
    for (const auto& allowed : AllowedFiles) {
        if (allowed == filename)
            return true;
    }
    return false;
}

std::string WebServer::ReadFile(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open())
        return {};
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

bool WebServer::WriteFile(const std::string& path, const std::string& content)
{
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f.is_open())
        return false;
    f.write(content.data(), content.size());
    return f.good();
}
