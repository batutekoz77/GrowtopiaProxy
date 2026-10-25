#pragma once

#include "../Network.hpp"
#include "../../Main/Config.hpp"
#include "../../Packets/Packets.hpp"
#include "../../Logger/Logger.hpp"
#include "../../Functions/Functions.hpp"

#include <vector>
#include <string>
#include <cstdint>

struct TileExtra {
    uint8_t type;
};

struct Tile {
    uint16_t fg;
    uint16_t bg;
    uint16_t parent;
    uint16_t flag;
    uint16_t lock_parent;
    TileExtra extra;
};

inline void parse_tile(const uint8_t*& ptr, const uint8_t* buffer_end, int x, int y) {
    Tile tile{};


}

// World packet parse sınıfı
class PACKET_SEND_MAP_DATA {
public:
    static inline bool Inject(std::string Method, int Type, ENetPacket* packet,
        ENetPeer* CLIENT, ENetPeer* SERVER, const PlayerMovingView& pm) {

        if (Type != 4 || !packet || !packet->data) return true;

        const uint8_t* data = packet->data;

        uint8_t name_len = *(uint8_t*)(data + 66);
        std::string world_name((char*)(data + 68), name_len);

        uint8_t xSize = *(uint8_t*)(data + 68 + name_len);
        uint8_t ySize = *(uint8_t*)(data + 72 + name_len);
        uint16_t block_count = *(uint16_t*)(data + 76 + name_len);

        LOG_DEBUG("World Name: {}", world_name);
        LOG_DEBUG("World Size: {} x {}", xSize, ySize);
        LOG_DEBUG("Total Blocks: {}", block_count);

        Player.world = world_name;

        const uint8_t* blc = data + 80 + name_len;
        blc += 5; // server extra skip
        const uint8_t* blc_end = data + packet->dataLength;

        for (uint16_t i = 0; i < block_count; i++) {
            int x = i % xSize;
            int y = i / xSize;

            parse_tile(blc, blc_end, x, y);
        }

        return true;
    }
};