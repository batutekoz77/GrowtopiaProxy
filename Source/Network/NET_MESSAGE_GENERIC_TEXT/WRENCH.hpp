#pragma once

#include "../Network.hpp"
#include "../../Main/Config.hpp"
#include "../../Packets/Packets.hpp"
#include "../../Logger/Logger.hpp"
#include "../../Functions/Functions.hpp"
#include "../../Player/Player.hpp"

#include <variant>
#include <vector>
#include <cstring>

class WRENCH {
public:
    static inline bool Inject(std::string packet, ENetPeer* CLIENT, ENetPeer* SERVER) {
        std::string cmd;
        if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) cmd = "/pull ";
        else if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) cmd = "/kick ";
        else return true;

        int thatNetID = Packet::ExtractCustom<int>(packet, "|", 3, "\n");

        if (Player.wrenchMode && thatNetID > 0 && thatNetID != Player.netID) {
            auto it = Player.worldPlayers.find(thatNetID);
            if (it != Player.worldPlayers.end()) {
                std::string thatName = it->second;

                {

                    GamePacket<OnTalkBubble> p;
                    p.netId = Player.netID;
                    p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + "Sent " + cmd + thatName + "!";
                    SendGamePacket(CLIENT, p);
                }

                std::string pullPacket = "action|input\n|text|" + cmd + thatName + "\n";
                {
                    std::lock_guard<std::mutex> lock(Network.peer_mutex);
                    SendPacket(SERVER, Network.NET_MESSAGE_GENERIC_TEXT, pullPacket.c_str(), pullPacket.length());
                }
                return false;
            }
        }

        return true;
    }
};