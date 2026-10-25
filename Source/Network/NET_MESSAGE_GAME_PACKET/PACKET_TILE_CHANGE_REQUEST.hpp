#pragma once

#include "../Network.hpp"
#include "../../Main/Config.hpp"
#include "../../Packets/Packets.hpp"
#include "../../Logger/Logger.hpp"
#include "../../Functions/Functions.hpp"

#include <variant>
#include <vector>
#include <cstring>

class PACKET_TILE_CHANGE_REQUEST {
public:
    static inline bool Inject(std::string Method, int Type, ENetPacket* packet, ENetPeer* CLIENT, ENetPeer* SERVER, const PlayerMovingView& pm) {
        /* @debug
            [06:03:2026 16:19:46] [INFO ] [NETWORK] [CLIENT -> PROXY -> SERVER] [NET_MESSAGE_GAME_PACKET] [PACKET_TILE_CHANGE_REQUEST:3]
            PlantingTree:2|X:709.00|Y:738.00|PunchX:23|PunchY:23|

            [06:03:2026 16:19:46] [INFO ] [NETWORK] [CLIENT -> PROXY -> SERVER] [NET_MESSAGE_GAME_PACKET] [PACKET_STATE:0]
            CharacterState:3104|PlantingTree:2|X:709.00|Y:738.00|PunchX:23|PunchY:23|
        */

        if (Method == "SERVER -> PROXY -> CLIENT" && pm.NetID() == Player.netID && 
            pm.PlantingTree() > 0 &&  
            pm.X() > 0.00f && pm.Y() > 0.00f && 
            pm.PunchX() > 0 && pm.PunchY() > 0
        ) Player.RemoveItem(pm.PlantingTree(), 1);


        if (Player.autofarm.running == true && Player.autofarm.id == -1 && Player.autofarm.X == -1.00f && Player.autofarm.Y == -1.00f && Player.autofarm.punchX == -1 && Player.autofarm.punchY == -1) {
            if (Method == "SERVER -> PROXY -> CLIENT" && pm.NetID() == Player.netID && pm.PlantingTree() > 0 && pm.X() > 0.00f && pm.Y() > 0.00f && pm.PunchX() > 0 && pm.PunchY() > 0) {
                Player.autofarm.id = pm.PlantingTree();
                Player.autofarm.X = pm.X(), Player.autofarm.Y = pm.Y();
                Player.autofarm.punchX = pm.PunchX(), Player.autofarm.punchY = pm.PunchY();

                std::string text = "Autofarm Started!";

                {
                    GamePacket<OnConsoleMessage> p;
                    p.text = text;
                    SendGamePacket(CLIENT, p);
                }

                {
                    GamePacket<OnTalkBubble> p;
                    p.netId = Player.netID;
                    p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + text;
                    SendGamePacket(CLIENT, p);
                }

                {
                    GamePacket<OnSetFreezeState> p;
                    p.seconds = UINT32_MAX;
                    SendGamePacket(CLIENT, p, Player.netID);
                }
            }
        }

        return true;
    }
};