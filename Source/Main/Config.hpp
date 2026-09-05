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
};
inline gServer Server;


struct gClient {
    std::string VERSION = "5.44";
    std::string PROTOCOL = "225";
    std::string PLATFORM = "0,1,1"; /* @info: Don't change if you are on 'Windows' */
};
inline gClient Client;


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
                /* @important: movement is per-player, per-tick. One player is
                   nothing; a crowded world is a different order of magnitude,
                   and it arrives all at once on world entry. Logging it is the
                   fireshose that stalls the logger -- and through it, the
                   network thread (see Logger::push). */
                case Network.LOG_NET_MESSAGE_GAME_PACKET_PLAYER_MOVING: return false;
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