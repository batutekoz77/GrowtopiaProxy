#include "Http.hpp"
#include "../Logger/Logger.hpp"
#include "../Main/Config.hpp"
#include "../Functions/Functions.hpp"
#include "../Network/Network.hpp"

#include <exception>   /* std::exception_ptr, for the handler-threw log */
#include <thread>
#include <chrono>

HttpManager Http;

bool HttpManager::Fetcher() {
    if (System::IsTrueAdmin()) System::editHosts("");
    else return false;

    FastLog::Logger::set_thread_name("HTTP");

    httplib::Client cli("https://www.growtopia2.com");
    cli.enable_server_certificate_verification(false);

    /* @note: cpp-httplib can be pointed at an HTTP proxy but not at a
       SOCKS5 one, so it is pointed at our own CONNECT shim on loopback,
       which chains onward. That is the only reason the shim exists. */
    if (Route.MODE == gRoute::SOCKS5) {
        cli.set_proxy(Route.LOCAL_IP.c_str(), Route.HTTP_PORT);
        LOG_INFO("server_data.php fetch routed through {}:{}", Route.HOST, Route.PORT);
    }

    httplib::Headers headers = {
        { "User-Agent", "UbiServices_SDK_2022.Release.9_PC64_ansi_stati" },
        { "Accept", "*/*" }
    };

    httplib::Result res;
    int retry_count = 0;
    const int max_retries = 3;

    while (!(res = cli.Post("/growtopia/server_data.php", headers,
        "version=" + Client.VERSION + "&protocol=" + Client.PROTOCOL + "&platform=" + Client.PLATFORM + "",
        "application/x-www-form-urlencoded"))) {

        if (++retry_count >= max_retries) {
            LOG_ERROR("Failed to fetch server data after {} attempts", max_retries);
            return false;
        }

        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    std::string server = getValue(res->body, "server");
    std::string port = getValue(res->body, "port");
    std::string loginurl = getValue(res->body, "loginurl");
    std::string meta = getValue(res->body, "meta");

    if (server.empty() || port.empty() || loginurl.empty() || meta.empty()) {
        server.empty() ? LOG_ERROR("server is missing") : LOG_INFO("server: {}", server);
        port.empty() ? LOG_ERROR("port is missing") : LOG_INFO("port: {}", port);
        loginurl.empty() ? LOG_ERROR("loginurl is missing") : LOG_INFO("loginurl: {}", loginurl);
        meta.empty() ? LOG_ERROR("meta is missing") : LOG_INFO("meta: {}", meta);
        return false;
    }
    LOG_DEBUG("Fetching www.growtopia2.com...");
    LOG_DEBUG("server: {}", server);
    LOG_DEBUG("port: {}", port);
    LOG_DEBUG("loginurl: {}", loginurl);
    LOG_DEBUG("meta: {}", meta);

    /* @note: REAL_* is the address server_data.php actually gave us, kept
       separate because on the SOCKS5 route Server.IP/UDP below are
       rewritten to a local socket -- and something still has to know
       where that socket is meant to forward to. */
    Server.REAL_IP  = server;
    Server.REAL_UDP = static_cast<uint16_t>(std::stoi(port));

    if (Route.MODE == gRoute::SOCKS5) {
        /* @important: fail closed, here and below. Falling back to a direct
           dial would put the traffic on this machine's own address while the
           operator believes it is routed -- worse than not starting.
           login_route = 0 is how you say you accept that, on purpose.

           The login page goes first because it is the cheaper failure: at
           this point nothing has been started and no name has been taken
           over yet. */
        if (Route.LOGIN && !System::StartLoginRelay(loginurl)) {
            LOG_ERROR("Could not carry the login page through the SOCKS5 server.");
            LOG_ERROR("Set login_route = 0 in socks5.cfg to go on without it --");
            LOG_ERROR("Docs/socks5.md says what that costs.");
            return false;
        }

        if (!System::Socks5UdpStart(Server.REAL_IP, Server.REAL_UDP)) {
            LOG_ERROR("Could not carry the game session through the SOCKS5 server.");
            return false;
        }

        /* Everything downstream -- the enet_address_set_host_ip below and
           NetworkManager::Setup -- reads Server.IP/UDP, so pointing those
           at our local socket routes ENet without either of them needing
           to know a route exists. The port is passed through unchanged
           because the game checks it against the value carried inside the
           packet header. */
        Server.IP  = Route.LOCAL_IP;
        Server.UDP = Server.REAL_UDP;
    }
    else {
        Server.IP  = Server.REAL_IP;
        Server.UDP = Server.REAL_UDP;
    }

    {
        /* This returned a string literal from a function declared `bool`. The
           literal decays to a non-null `const char*`, which converts to `true`
           -- so an unusable server address was reported to the caller as a
           successful fetch, and the failure only surfaced later as a connection
           that never completes.

           Also `!= 0` rather than `< 0`: enet_address_set_host_ip is documented
           to return 0 on success, and does not promise a negative value on
           failure. Network.cpp already compares it that way. */
        if (enet_address_set_host_ip(&Network.GetServerAddress(), Server.IP.c_str()) != 0) {
            LOG_ERROR("Invalid server IP from server_data.php: {}", Server.IP);
            return false;
        }
        Network.GetServerAddress().port = Server.UDP;
    }

    server_data_cache =
        "server|" + Local.IP + "\n"
        "port|" + std::to_string(Local.UDP) + "\n"
        "loginurl|" + loginurl + "\n"
        "type|1\n"
        "type2|1\n"
        "#maint|Maintenance message\n"
        "\n"
        "beta_server|127.0.0.1\n"
        "beta_port|17091\n"
        "\n"
        "beta_type|1\n"
        "meta|" + meta + "\n"
        "RTENDMARKERBS1001";

    System::editHosts(Local.IP);
    Sleep(1000);

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

    SSLServer svr("growtopia.pem", "growtopia.key.pem");
    if (!svr.is_valid()) {
        LOG_ERROR("Invalid SSL configuration!");
        return;
    }

    svr.Get("/", [&](const Request& req, Response& res) {
        res.set_content("Simple C++ HTTP Server\nOptimized Version", "text/plain");
    });

    svr.Post("/growtopia/server_data.php", [&](const Request& req, Response& res) {
        FastLog::Logger::set_thread_name("HTTP");

        LOG_WARN("Request: /growtopia/server_data.php");
        
        fetching = true;
        bool ok = Fetcher();
        fetching = false;
        if (ok && server_data_cache.empty()) {
            res.status = 500;
            res.set_content("Internal Server Error: No server data available.", "text/plain");
        }
        else res.set_content(server_data_cache, "text/plain");
    });

    /* @important: a request that reaches us and matches no handler is
       answered 404 by cpp-httplib itself, and nothing is logged. From the
       log that is indistinguishable from a request that never arrived --
       and those two have nothing to do with each other. One is a routing
       or hosts-file problem, the other is a missing handler here.

       set_logger runs for every completed request whatever happened, so
       the difference is visible instead of guessed at. It is also the only
       way to see what the game asks for that this does not serve, which is
       most of the point of running a proxy in front of it. */
    svr.set_logger([](const Request& req, const Response& res) {
        FastLog::Logger::set_thread_name("HTTP");

        const std::string host = req.get_header_value("Host");
        LOG_INFO("[HTTPD] {} {} {} -> {}",
                 host.empty() ? "-" : host.c_str(), req.method, req.path, res.status);
    });

    /* A throw inside a handler otherwise becomes a bare 500 with nothing
       said anywhere about what threw. */
    svr.set_exception_handler([](const Request& req, Response& res, std::exception_ptr ep) {
        FastLog::Logger::set_thread_name("HTTP");

        std::string what = "unknown";
        try { if (ep) std::rethrow_exception(ep); }
        catch (const std::exception& e) { what = e.what(); }
        catch (...) {}

        LOG_ERROR("[HTTPD] handler threw on {} {}: {}", req.method, req.path, what);
        res.status = 500;
    });

    LOG_INFO("Started HTTP server");

    if (!svr.listen(Local.IP, Local.TCP)) {
        LOG_ERROR("Failed to bind to port {}!", Local.TCP);
        return;
    }
}