#pragma once

#include "../Network.hpp"
#include "../../Main/Config.hpp"
#include "../../Packets/Packets.hpp"
#include "../../Logger/Logger.hpp"
#include "../../Functions/Functions.hpp"

#include <variant>
#include <vector>
#include <cstring>

class PACKET_MODIFY_ITEM_INVENTORY {
public:
    static inline bool Inject(std::string Method, int Type, ENetPacket* packet, ENetPeer* CLIENT, ENetPeer* SERVER, const PlayerMovingView& pm) {
        /* @debug
            [23:02:2026 13:43:01] [INFO ] [NETWORK] [SERVER -> PROXY -> CLIENT] [NET_MESSAGE_GAME_PACKET] [PACKET_MODIFY_ITEM_INVENTORY:13]
            PlantingTree:242|PACKET_MODIFY_ITEM_INVENTORY|ItemID:0|Gained:0|Jump:1|

            [23:02:2026 13:43:01] [INFO ] [NETWORK] [SERVER -> PROXY -> CLIENT] [NET_MESSAGE_GAME_PACKET] [PACKET_MODIFY_ITEM_INVENTORY:13]
            PlantingTree:7164|PACKET_MODIFY_ITEM_INVENTORY|ItemID:0|Gained:15|Jump:0|
        */

        if (pm.PlantingTree() > 0) {
            int id = pm.PlantingTree();
            auto modInfo = pm.GetModifyItemInfo();
            if (modInfo.valid && modInfo.item_id == 0) {
                if (modInfo.gained > 0) Player.AddItem(id, modInfo.gained);
                else if (modInfo.jump > 0) Player.RemoveItem(id, modInfo.jump);
            }
        }
        return true;
    }
};