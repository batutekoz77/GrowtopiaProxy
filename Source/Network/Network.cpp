#include "Network.hpp"
#include "Handler.hpp"
#include "../Main/Config.hpp"
#include "../Logger/Logger.hpp"
#include "../Functions/Functions.hpp"
#include "../Http/Http.hpp"

#include <thread>
#include <cstring>

NetworkManager Network;


ENetHost* NetworkManager::GetProxyHost() { return PROXY_HOST; }
ENetHost* NetworkManager::GetServerHost() { return SERVER_HOST; }
ENetAddress& NetworkManager::GetProxyAddress() { return PROXY_ADDRESS; }
ENetAddress& NetworkManager::GetServerAddress() { return SERVER_ADDRESS; }


std::string NetworkManager::Setup(int which) {
    if (which == 1) {
        memset(&PROXY_ADDRESS, 0, sizeof(PROXY_ADDRESS));
        PROXY_ADDRESS.type = ENET_ADDRESS_TYPE_IPV4;
        if (enet_address_set_host_ip(&PROXY_ADDRESS, Local.IP.c_str()) != 0) return "Failed to set proxy host IP (127.0.0.1)";
        PROXY_ADDRESS.port = Local.UDP;

        PROXY_HOST = enet_host_create(ENET_ADDRESS_TYPE_IPV4, &PROXY_ADDRESS, 1024, 2, 0, 0);
        if (!PROXY_HOST) return "Couldn't create the PROXY_HOST!";

        PROXY_HOST->usingNewPacketForServer = 1;
        PROXY_HOST->checksum = enet_crc32;
        PROXY_HOST->mtu = 1400;
        enet_host_compress_with_range_coder(PROXY_HOST);
        enet_host_bandwidth_limit(PROXY_HOST, 0, 0);

        STATUS = 1;
    }
    else if (which == 2) {
        memset(&SERVER_ADDRESS, 0, sizeof(SERVER_ADDRESS));
        SERVER_ADDRESS.type = ENET_ADDRESS_TYPE_IPV4;
        if (enet_address_set_host_ip(&SERVER_ADDRESS, Server.IP.c_str()) != 0) return "Invalid server IP!";
        SERVER_ADDRESS.port = Server.UDP;

        SERVER_HOST = enet_host_create(ENET_ADDRESS_TYPE_IPV4, nullptr, 1, 2, 0, 0);
        if (!SERVER_HOST) return "Couldn't create the SERVER_HOST!";

        SERVER_HOST->usingNewPacketForClient = 1;
        SERVER_HOST->checksum = enet_crc32;
        SERVER_HOST->mtu = 1400;
        enet_host_compress_with_range_coder(SERVER_HOST);
        enet_host_bandwidth_limit(SERVER_HOST, 0, 0);

        STATUS = 2;
    }

    return "";
}

void NetworkManager::Injector() {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    SetThreadAffinityMask(GetCurrentThread(), 1 << 2);

    FastLog::Logger::set_thread_name("NETWORK");

    if (STATUS == 0) {
        std::string log = Setup(1);
        if (!log.empty()) LOG_ERROR("{}", log);
    }
    if (STATUS != 1) return;

    LOG_INFO("Started ENET server");

    int process_id = -1;
    bool waiting_logged = false;
    
    while ((process_id = System::findProcess(L"Growtopia.exe")) == -1) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        if (!waiting_logged) {
            LOG_WARN("Starting Growtopia.exe");
            waiting_logged = true;
            
            std::wstring path = std::wstring(_wgetenv(L"LOCALAPPDATA")) + L"\\Growtopia\\Growtopia.exe";
            System::startProcess(path);
        }
    }

    Sleep(3000);

    LOG_INFO("Spoofed the Growtopia.exe ({})", process_id);

    LOG_WARN("Press the 'Play Online' button to fetch the server api");

    while (Http.server_data_cache.empty()) std::this_thread::sleep_for(std::chrono::seconds(3));

    Sleep(1000);

    LOG_WARN("Waiting for the Growtopia.exe to connect the proxy");

    std::string log2 = Setup(2);
    if (!log2.empty()) LOG_ERROR("{}", log2);

    while (STATUS == 2) {
        int waitingPacket = 0;
        if (PROXY_HOST) {
            while (enet_host_service(PROXY_HOST, &PROXY_EVENT, 0) > 0) {
                ENetPeer* client_peer = PROXY_EVENT.peer;

                switch (PROXY_EVENT.type) {
                    case ENET_EVENT_TYPE_CONNECT: {
                        char baseIP[64];
                        enet_address_get_host_ip(&client_peer->address, baseIP, sizeof(baseIP));
                        std::string ClientIP = std::string(baseIP);

                        LOG_INFO("Client connected to the Proxy");
                        LOG_WARN("Waiting for the Proxy to connect the Server (connects if gt servers are online)");

                        ENetPeer* server_peer = enet_host_connect(SERVER_HOST, &SERVER_ADDRESS, 2, 0);
                        if (!server_peer) {
                            LOG_ERROR("Proxy couldn't connected to the Server!");
                            enet_peer_disconnect_now(client_peer, 0);
                            break;
                        }

                        client_peer->pingInterval = 300;
                        enet_peer_throttle_configure(client_peer, 0, 0, 0);

                        {
                            std::lock_guard<std::mutex> lock(peer_mutex);

                            client_to_server.clear();
                            server_to_client.clear();

                            client_to_server[client_peer] = server_peer;
                            server_to_client[server_peer] = client_peer;
                        }

                        break;
                    }
                    case ENET_EVENT_TYPE_DISCONNECT: {
                        std::lock_guard<std::mutex> lock(peer_mutex);
                        auto it = client_to_server.find(client_peer);
                        if (it != client_to_server.end()) {
                            ENetPeer* server_peer = it->second;
                            if (server_peer) {
                                enet_peer_disconnect_now(server_peer, 0);
                                server_to_client.erase(server_peer);
                                client_to_server.erase(client_peer);
                            }
                        }

                        break;
                    }
                    case ENET_EVENT_TYPE_RECEIVE: {
                        if (!PROXY_EVENT.packet) break;

                        ENetPeer* server_peer = nullptr;
                        {
                            std::lock_guard<std::mutex> lock(peer_mutex);
                            auto it = client_to_server.find(client_peer);
                            if (it != client_to_server.end()) server_peer = it->second;
                        }

                        if (server_peer) {
                            if (HandlePacket(CLIENT_PROXY_SERVER, PROXY_EVENT.packet, server_peer)) {
                                /* @fix: enet_peer_send takes ownership only on
                                   success; on failure (< 0) we still own the
                                   packet and must free it. Leaking here is
                                   worst exactly when it happens -- under load,
                                   when memory is already tight. */
                                if (enet_peer_send(server_peer, 0, PROXY_EVENT.packet) < 0)
                                    enet_packet_destroy(PROXY_EVENT.packet);
                            }
                            else enet_packet_destroy(PROXY_EVENT.packet);
                        }

                        break;
                    }
                    default: { break; }
                }
            }
        }

        if (SERVER_HOST) {
            while (enet_host_service(SERVER_HOST, &SERVER_EVENT, 0) > 0) {
                ENetPeer* server_peer = SERVER_EVENT.peer;

                switch (SERVER_EVENT.type) {
                    case ENET_EVENT_TYPE_CONNECT: {
                        LOG_INFO("Proxy connected to the Server");
                        server_peer->pingInterval = 300;
                        enet_peer_throttle_configure(server_peer, 0, 0, 0);

                        break;
                    }
                    case ENET_EVENT_TYPE_DISCONNECT: {
                        std::lock_guard<std::mutex> lock(peer_mutex);
                        auto it = server_to_client.find(server_peer);
                        if (it != server_to_client.end()) {
                            ENetPeer* client_peer = it->second;
                            if (client_peer) {
                                enet_peer_disconnect_now(client_peer, 0);
                                client_to_server.erase(client_peer);
                                server_to_client.erase(server_peer);
                            }
                        }

                        break;
                    }
                    case ENET_EVENT_TYPE_RECEIVE: {
                        if (!SERVER_EVENT.packet) break;

                        ENetPeer* client_peer = nullptr;
                        {
                            std::lock_guard<std::mutex> lock(peer_mutex);
                            auto it = server_to_client.find(server_peer);
                            if (it != server_to_client.end()) client_peer = it->second;
                        }

                        if (client_peer) {
                            if (HandlePacket(SERVER_PROXY_CLIENT, SERVER_EVENT.packet, client_peer)) {
                                /* @fix: same ownership rule as the other
                                   direction -- free the packet if the send
                                   failed, it is still ours. */
                                if (enet_peer_send(client_peer, 0, SERVER_EVENT.packet) < 0)
                                    enet_packet_destroy(SERVER_EVENT.packet);
                            }
                            else enet_packet_destroy(SERVER_EVENT.packet);
                        }

                        break;
                    }
                    default: { break; }
                }
            }
        }

        if (PROXY_HOST) enet_host_flush(PROXY_HOST);
        if (SERVER_HOST) enet_host_flush(SERVER_HOST);

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}