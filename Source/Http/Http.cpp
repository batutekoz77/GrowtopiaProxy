#include "Http.hpp"
#include "../Logger/Logger.hpp"
#include "../Main/Config.hpp"
#include "../Functions/Functions.hpp"
#include "../Network/Network.hpp"

#include <exception>
#include <thread>
#include <chrono>

HttpManager Http;

static void replace_field(std::string& src, const std::string& key, const std::string& value) {
    const std::string prefix = key + "|";
    size_t pos = 0;
    while (pos < src.size()) {
        size_t line_end = src.find('\n', pos);
        if (line_end == std::string::npos) line_end = src.size();
        if (src.compare(pos, prefix.size(), prefix) == 0) {
            src.replace(pos, line_end - pos, prefix + value);
            return;
        }
        pos = (line_end == src.size()) ? src.size() : line_end + 1;
    }
}

static void install_root_cert(const char* path) {
    std::string cmd = std::string("certutil -addstore -f ROOT \"") + path + "\"";
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi{};
    std::string mutable_cmd = cmd;
    if (CreateProcessA(nullptr, mutable_cmd.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 15000);
        DWORD exit_code = 1;
        GetExitCodeProcess(pi.hProcess, &exit_code);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        if (exit_code == 0) LOG_INFO("Installed local HTTPS certificate into Windows Root store");
        else LOG_ERROR("certutil failed with code {}", exit_code);
    }
    else {
        LOG_ERROR("Failed to launch certutil. The game will reject the proxy HTTPS certificate.");
    }
}

bool HttpManager::Fetcher() {
    FastLog::Logger::set_thread_name("HTTP");

    if (official_ip.empty()) {
        official_ip = System::resolveHostA("www.growtopia2.com");
        if (official_ip.empty()) official_ip = System::resolveHostA("www.growtopia1.com");
        if (official_ip.empty()) {
            LOG_ERROR("Could not resolve Growtopia HTTP servers (DNS bypass failed)");
            return false;
        }
        LOG_INFO("Official Growtopia HTTP IP: {}", official_ip);
    }

    httplib::Client cli("https://www.growtopia2.com");
    cli.enable_server_certificate_verification(false);
    cli.set_hostname_addr_map({ { "www.growtopia2.com", official_ip } });
    cli.set_connection_timeout(10, 0);
    cli.set_read_timeout(10, 0);
    cli.set_write_timeout(10, 0);

    httplib::Headers headers = {
        { "User-Agent", "UbiServices_SDK_2022.Release.9_PC64_ansi_stati" },
        { "Accept", "*/*" }
    };

    httplib::Result res;
    int retry_count = 0;
    const int max_retries = 3;

    const std::string body =
        "version=" + Client.VERSION + "&protocol=" + Client.PROTOCOL + "&platform=" + Client.PLATFORM;

    while (!(res = cli.Post("/growtopia/server_data.php", headers, body, "application/x-www-form-urlencoded"))) {
        if (++retry_count >= max_retries) {
            LOG_ERROR("Failed to fetch server data after {} attempts: {}", max_retries, httplib::to_string(res.error()));
            return false;
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    if (res->status != 200 || res->body.find("server|") == std::string::npos) {
        LOG_ERROR("Unexpected server_data response (status {}): {}", res->status, res->body.substr(0, 200));
        return false;
    }

    std::string server = getValue(res->body, "server");
    std::string port = getValue(res->body, "port");
    std::string loginurl = getValue(res->body, "loginurl");
    std::string meta = getValue(res->body, "meta");

    if (server.empty() || port.empty()) {
        LOG_ERROR("server_data.php missing server/port");
        LOG_DEBUG("body: {}", res->body);
        return false;
    }
    if (loginurl.empty()) loginurl = "login.growtopiagame.com";

    LOG_DEBUG("Fetching www.growtopia2.com...");
    LOG_DEBUG("server: {}", server);
    LOG_DEBUG("port: {}", port);
    LOG_DEBUG("loginurl: {}", loginurl);
    LOG_DEBUG("meta: {}", meta);

    Server.IP = server;
    Server.UDP = static_cast<uint16_t>(std::stoi(port));

    if (enet_address_set_host_ip(&Network.GetServerAddress(), Server.IP.c_str()) != 0) {
        LOG_ERROR("Invalid server IP from server_data.php: {}", Server.IP);
        return false;
    }
    Network.GetServerAddress().port = Server.UDP;

    std::string rewritten = res->body;
    replace_field(rewritten, "server", Local.IP);
    replace_field(rewritten, "port", std::to_string(Local.UDP));

    {
        std::lock_guard<std::mutex> lock(cache_mutex);
        server_data_cache = rewritten;
    }

    LOG_INFO("Fetched the api www.growtopia2.com");
    return true;
}

void HttpManager::Injector() {
    using namespace httplib;

    FastLog::Logger::set_thread_name("HTTP");

    ensure_cert_files_exist();

    const char* cert_file = "growtopia.pem";
    const char* key_file = "growtopia.key.pem";

    while (!std::filesystem::exists(cert_file) || !std::filesystem::exists(key_file)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    install_root_cert(cert_file);

    if (!Fetcher()) {
        LOG_ERROR("Could not fetch official server_data.php. The game will fail to log in.");
        return;
    }

    SSLServer svr("growtopia.pem", "growtopia.key.pem");
    if (!svr.is_valid()) {
        LOG_ERROR("Invalid SSL configuration!");
        return;
    }

    svr.new_task_queue = [] { return new httplib::ThreadPool(8); };

    svr.Get("/", [&](const Request& req, Response& res) {
        res.set_content("Simple C++ HTTP Server\nOptimized Version", "text/plain");
    });

    svr.Post("/growtopia/server_data.php", [&](const Request& req, Response& res) {
        FastLog::Logger::set_thread_name("HTTP");
        LOG_WARN("Request: /growtopia/server_data.php");

        std::string cache;
        {
            std::lock_guard<std::mutex> lock(cache_mutex);
            cache = server_data_cache;
        }

        if (cache.empty()) {
            fetching = true;
            bool ok = Fetcher();
            fetching = false;
            std::lock_guard<std::mutex> lock(cache_mutex);
            cache = server_data_cache;
            if (!ok || cache.empty()) {
                res.status = 500;
                res.set_content("Internal Server Error: No server data available.", "text/plain");
                return;
            }
        }

        res.set_content(cache, "text/plain");
    });

    if (!svr.bind_to_port(Local.IP, Local.TCP)) {
        LOG_ERROR("Failed to bind to port {}! Close anything using HTTPS (IIS, other proxies) and retry.", Local.TCP);
        return;
    }

    System::editHosts(Local.IP);
    listening = true;
    LOG_INFO("Started HTTP server on {}:{}", Local.IP, Local.TCP);

    if (!svr.listen_after_bind()) {
        LOG_ERROR("HTTP server listen failed on port {}!", Local.TCP);
    }
}
