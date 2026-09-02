#pragma once

#include <unordered_map>
#include <algorithm>
#include <string>
#include "../Network/Network.hpp"


struct gMain {
    std::string NAME = "Growtopia Proxy";
    std::string VERSION = "1.0";
    bool BASIC_LOGS = false;
};
inline gMain Main;


struct gLocal {
	std::string IP = "127.0.0.1";
	uint16_t TCP = 443;
	uint16_t UDP = 17123;
};
inline gLocal Local;


struct gServer {
	/* @important: Fetching from www.growtopia2.com(do not change) */
	std::string IP = ""; // empty if real gt
	uint16_t UDP = -1; // -1 if real gt

	/* @note: the REAL endpoint from server_data.php, always. On the SOCKS5
	   route IP/UDP above are rewritten to a local socket, so that Http.cpp
	   and NetworkManager::Setup both dial the route without either having
	   to know a route exists -- but something still has to remember where
	   the route is supposed to forward to, and that is these two.
	   Equal to IP/UDP on the direct route. */
	std::string REAL_IP = "";
	uint16_t REAL_UDP = 0;
};
inline gServer Server;


struct gClient {
    std::string VERSION = "5.44";
    std::string PROTOCOL = "225";
    std::string PLATFORM = "0,1,1"; /* @info: Don't change if you are on 'Windows' */
};
inline gClient Client;


/* @note: optional. Sends the proxy's own traffic through a SOCKS5 server
   instead of straight out: the server_data.php fetch over TCP, and the ENet
   game session over UDP.

   What it does NOT cover: the game client opens its login page to the
   'loginurl' address itself. That request never reaches this process and is
   not redirected by the hosts file, so it still goes out directly. Anyone
   relying on this must know that. */
struct gRoute {
    enum Mode { DIRECT = 0, SOCKS5 = 1 };
    int MODE = DIRECT;

    /* @important: read at runtime from 'socks5.cfg' next to the exe. Nothing
       here is compiled in, least of all PASS. See System::LoadRouteConfig. */
    std::string HOST = "";
    uint16_t    PORT = 0;
    std::string USER = "";
    std::string PASS = "";

    /* Local endpoints the proxy opens for itself. Change only on a clash. */
    std::string LOCAL_IP  = "127.0.0.1";
    uint16_t    HTTP_PORT = 18080;   /* CONNECT shim, for the server_data fetch */
};
inline gRoute Route;


inline bool ShouldLogNetMessage(int type, int extra = 0) {
    /* @important: Customize the Logging System
        return true = print
        return false = !print
    */

    switch (type) {
        case Network.NET_MESSAGE_SERVER_HELLO: return true;

        case Network.NET_MESSAGE_GENERIC_TEXT: return true;
        case Network.NET_MESSAGE_GAME_MESSAGE: return true;

        case Network.NET_MESSAGE_GAME_PACKET: {
            switch (extra) {
                case Network.LOG_NET_MESSAGE_GAME_PACKET_BASIC: return true;
                case Network.LOG_NET_MESSAGE_GAME_PACKET_BASIC_CLEAR_SPAM: return true;
                case Network.LOG_NET_MESSAGE_GAME_PACKET_PLAYER_MOVING: return true;
                default: return true;
            }
        }

        case Network.NET_MESSAGE_ERROR: return true;
        case Network.NET_MESSAGE_TRACK: return true;

        case Network.NET_MESSAGE_CLIENT_LOG_REQUEST: return true;
        case Network.NET_MESSAGE_CLIENT_LOG_RESPONSE: return true;

        case Network.NET_MESSAGE_UNKNOWN: 
        default:
        { return true; }
    }
}