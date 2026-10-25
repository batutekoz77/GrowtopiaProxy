#pragma once

#include <cstdint>
#include <cstring>
#include "ENET/enet.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <string>
#include <vector>
#include <array>

#include "../Logger/Logger.hpp"
#include "../Main/Config.hpp"

using BYTE = unsigned char;
struct Vec2 { float x; float y; };
struct Vec3 { float x; float y; float z; };

class GamePacketBuilder {
private:
    int index = 0, len = 0;
    std::vector<BYTE> packet_data;

public:
    GamePacketBuilder(int delay = 0, int NetID = -1);

    GamePacketBuilder& Insert(const std::string& a);
    GamePacketBuilder& Insert(int a);
    GamePacketBuilder& Insert(unsigned int a);
    GamePacketBuilder& Insert(float a);
    GamePacketBuilder& Insert(float a, float b);
    GamePacketBuilder& Insert(float a, float b, float c);
    GamePacketBuilder& Insert(long long int a);

    void CreatePacket(ENetPeer* peer);

private:
    template<typename T>
    GamePacketBuilder& InsertValue(const T& value, BYTE type);
};

enum : uint8_t {
    OnConsoleMessage = 1,
    OnTalkBubble,
    OnTextOverlay,

    OnDialogRequest,
    OnSendToServer,
    OnSpawn,
    OnCountryState,
    OnSetClothing,
    OnSetFreezeState
};

inline const char* PacketName(uint8_t type) {
    switch (type) {
        case OnConsoleMessage:                       return "OnConsoleMessage";
        case OnTalkBubble:                           return "OnTalkBubble";
        case OnTextOverlay:                          return "OnTextOverlay";

        case OnDialogRequest:                        return "OnDialogRequest";
        case OnSendToServer:                         return "OnSendToServer";
        case OnSpawn:                                return "OnSpawn";
        case OnCountryState:                         return "OnCountryState";
        case OnSetClothing:                          return "OnSetClothing";
        case OnSetFreezeState:                       return "OnSetFreezeState";

        default:                                     return "Unknown";
    }
}

template<uint8_t T>
struct GamePacket;

template<> struct GamePacket<OnConsoleMessage> {
    static constexpr uint8_t type = OnConsoleMessage;
    std::string text;
};
template<> struct GamePacket<OnTalkBubble> {
    static constexpr uint8_t type = OnTalkBubble;
    uint32_t  netId = 0;
    std::string text;
    uint32_t unknown = 0;
    uint32_t unknown2 = -31;
};
template<> struct GamePacket<OnTextOverlay> {
    static constexpr uint8_t type = OnTextOverlay;
    std::string text;
};

template<> struct GamePacket<OnDialogRequest> {
    static constexpr uint8_t type = OnDialogRequest;
    std::string text;
};
template<> struct GamePacket<OnSendToServer> {
    static constexpr uint8_t type = OnSendToServer;
    int port = 0;
    int token = 0;
    int userId = 0;
    std::string ip;
    std::string doorID;
    std::string UUIDToken;
    int lmode = 0;
    std::string tankIDName;
};
template<> struct GamePacket<OnSpawn> {
    static constexpr uint8_t type = OnSpawn;
    std::string text;
};
template<> struct GamePacket<OnCountryState> {
    static constexpr uint8_t type = OnCountryState;
    std::string text;
};
template<> struct GamePacket<OnSetClothing> {
    static constexpr uint8_t type = OnSetClothing;
    Vec3 hair_shirt_pants{ 0.000000, 0.000000, 0.000000 };
    Vec3 feet_face_hand{ 0.000000, 0.000000, 0.000000 };
    Vec3 back_mask_necklace{ 0.000000, 0.000000, 0.000000 };
    uint32_t skinColor = 3033464831;
    Vec3 ances_unknown_unknown2{ 0.000000, 0.000000, 0.000000 };
};
template<> struct GamePacket<OnSetFreezeState> {
    static constexpr uint8_t type = OnSetFreezeState;
    uint32_t seconds = 0;
};


class GamePacketBuilder;

inline void Serialize(GamePacketBuilder& p, const GamePacket<OnConsoleMessage>& v) { 
    p.Insert("`o[`4" + Main.NAME + "`o]`w`w`w " + v.text); 
}
inline void Serialize(GamePacketBuilder& p, const GamePacket<OnTalkBubble>& v) { 
    p.Insert(v.netId)
        .Insert(v.text)
        .Insert(v.unknown); 
    if (v.unknown2 != -31) p.Insert(v.unknown2);
}
inline void Serialize(GamePacketBuilder& p, const GamePacket<OnTextOverlay>& v) { 
    p.Insert(v.text); 
}
inline void Serialize(GamePacketBuilder& p, const GamePacket<OnDialogRequest>& v) { 
    p.Insert(v.text); 
}
inline void Serialize(GamePacketBuilder& p, const GamePacket<OnSendToServer>& v) { 
    p.Insert(v.port)
        .Insert(v.token)
        .Insert(v.userId)
        .Insert(v.ip + "|" + v.doorID + "|" + v.UUIDToken)
        .Insert(v.lmode); 
    if (!v.tankIDName.empty()) p.Insert(v.tankIDName); 
}
inline void Serialize(GamePacketBuilder& p, const GamePacket<OnSpawn>& v) { 
    p.Insert(v.text); 
}
inline void Serialize(GamePacketBuilder& p, const GamePacket<OnCountryState>& v) {
    p.Insert(v.text); 
}
inline void Serialize(GamePacketBuilder& p, const GamePacket<OnSetClothing>& v) {
    p.Insert(v.hair_shirt_pants.x, v.hair_shirt_pants.y, v.hair_shirt_pants.z)
        .Insert(v.feet_face_hand.x, v.feet_face_hand.y, v.feet_face_hand.z)
        .Insert(v.back_mask_necklace.x, v.back_mask_necklace.y, v.back_mask_necklace.z)
        .Insert(v.skinColor)
        .Insert(v.ances_unknown_unknown2.x, v.ances_unknown_unknown2.y, v.ances_unknown_unknown2.z);
}
inline void Serialize(GamePacketBuilder& p, const GamePacket<OnSetFreezeState>& v) {
    p.Insert(v.seconds);
}


template<uint8_t T>
void SendGamePacket(ENetPeer* peer, const GamePacket<T>& pkt, int netID = -1) {
    if (peer != nullptr && peer->state == ENET_PEER_STATE_CONNECTED) {
        if (netID == -1) {
            GamePacketBuilder pckt;
            pckt.Insert(PacketName(T));
            Serialize(pckt, pkt);
            pckt.CreatePacket(peer);
        }
        else {
            GamePacketBuilder pckt(0, netID);
            pckt.Insert(PacketName(T));
            Serialize(pckt, pkt);
            pckt.CreatePacket(peer);
        }
    }
}





namespace PMOffset {
    constexpr size_t FLAG_BYTE = 12;
    constexpr size_t EXTRA_SIZE_DWORD = 52; // 13 * 4
    constexpr size_t DELAY = 24;
    constexpr size_t DEATH_FLAG = 56;
}

class PlayerMovingPacket {
public:
    static constexpr size_t SIZE = 56;

    PlayerMovingPacket() { buffer.fill(0); }

    PlayerMovingPacket& PacketType(int v) { write<int>(0x00, v); return *this; }
    PlayerMovingPacket& NetID(int v) { write<int>(0x04, v); return *this; }
    PlayerMovingPacket& CharacterState(int v) { write<int>(0x0C, v); return *this; }
    PlayerMovingPacket& PlantingTree(int v) { write<int>(0x14, v); return *this; }
    PlayerMovingPacket& Position(float x, float y) { write<float>(0x18, x); write<float>(0x1C, y); return *this; }
    PlayerMovingPacket& Speed(float xs, float ys) { write<float>(0x20, xs); write<float>(0x24, ys); return *this; }
    PlayerMovingPacket& Punch(int x, int y) { write<int>(0x2C, x); write<int>(0x30, y); return *this; }
    PlayerMovingPacket& Packet40(int v) { write<int>(0x28, v); return *this; }
    PlayerMovingPacket& SetByte(size_t offset, BYTE v) { buffer[offset] = v; return *this; }

    BYTE* Data() { return buffer.data(); }
    const BYTE* Data() const { return buffer.data(); }

    void Send(ENetPeer* peer, int a1 = 4, int flags = ENET_PACKET_FLAG_RELIABLE, int delay = 0) const { send_raw(peer, a1, buffer.data(), SIZE, flags, delay); }

private:
    std::array<BYTE, SIZE> buffer;

    template<typename T>
    inline void write(size_t offset, const T& v) { std::memcpy(buffer.data() + offset, &v, sizeof(T)); }

    static void send_raw(ENetPeer* peer, int a1, const void* packetData, size_t packetDataSize, enet_uint32 packetFlag, int delay = 0) {
        if (!peer || !packetData || packetDataSize == 0) return;
        const BYTE* data = static_cast<const BYTE*>(packetData);
        const bool hasExtra = (data[PMOffset::FLAG_BYTE] & 8) != 0;
        size_t extraSize = 0;
        if (hasExtra) {
            if (packetDataSize > PMOffset::EXTRA_SIZE_DWORD) {
                std::memcpy(&extraSize, data + PMOffset::EXTRA_SIZE_DWORD, sizeof(int));
                if (extraSize < 0) extraSize = 0;
            }
        }
        const size_t totalSize = 4 + packetDataSize + extraSize;
        ENetPacket* p = enet_packet_create(nullptr, totalSize, packetFlag);
        if (!p) return;
        std::memcpy(p->data, &a1, 4);
        std::memcpy(p->data + 4, packetData, packetDataSize);
        if (hasExtra && extraSize > 0) std::memset(p->data + 4 + packetDataSize, 0, extraSize);
        if (delay != 0) {
            constexpr int deathFlag = 0x19;
            if (packetDataSize >= PMOffset::DELAY + 4) {
                std::memcpy(p->data + PMOffset::DELAY, &delay, 4);
                std::memcpy(p->data + PMOffset::DEATH_FLAG, &deathFlag, 4);
            }
        }
        enet_peer_send(peer, 0, p);
    }
};


class PlayerMovingView {
public:
    explicit PlayerMovingView(const BYTE* data, size_t size) : data_(data), size_(size) {}

    int PacketType() const {
        if (!data_ || size_ < 1) return 0;
        return static_cast<int>(data_[0]);
    }
    int NetID() const { return Read<int>(0x04); }
    int CharacterState() const { return Read<int>(0x0C); }
    int PlantingTree() const { return Read<int>(0x14); }
    float X() const { return Read<float>(0x18); }
    float Y() const { return Read<float>(0x1C); }
    float XSpeed() const { return Read<float>(0x20); }
    float YSpeed() const { return Read<float>(0x24); }
    int PunchX() const { return Read<int>(0x2C); }
    int PunchY() const { return Read<int>(0x30); }

    bool HasExtra() const { return (Read<BYTE>(PMOffset::FLAG_BYTE) & 8) != 0; }

    struct ModifyItemInfo {
        int32_t item_id{};
        uint8_t gained{};
        uint8_t jump{};
        bool valid = false;
    };

    ModifyItemInfo GetModifyItemInfo() const {
        ModifyItemInfo info;
        if (PacketType() == 13 && size_ >= 12) {
            info.jump = Read<uint8_t>(0x02);
            info.gained = Read<uint8_t>(0x03);
            info.item_id = Read<int32_t>(0x08);
            info.valid = true;
        }
        return info;
    }

private:
    const BYTE* data_;
    size_t size_;

    template<typename T>
    inline T Read(size_t offset) const {
        if (!data_) return T{};
        if (offset + sizeof(T) > size_) return T{};
        T v{};
        std::memcpy(&v, data_ + offset, sizeof(T));
        return v;
    }
};

struct ModifyItemInventoryPacket {
    uint8_t type;            // 0x00 -> PacketType (13)
    uint8_t unused;          // 0x01 (objtype)
    uint8_t jump_count;      // 0x02
    uint8_t gained_item_count; // 0x03
    int32_t netid;           // 0x04
    int32_t item;            // 0x08 -> hangi item
};


inline const BYTE* get_packet_payload(const ENetPacket* packet, size_t& outSize) {
    if (!packet || packet->dataLength < 56) return nullptr;
    const BYTE* data = packet->data;
    size_t packetSize = packet->dataLength;
    outSize = packetSize - 4;
    const BYTE* payload = data + 4;
    if ((data[PMOffset::FLAG_BYTE] & 8) != 0) {
        int extraSize = 0;
        std::memcpy(&extraSize, data + PMOffset::EXTRA_SIZE_DWORD, sizeof(int));
        if (extraSize < 0) return nullptr;
        if (packetSize < 60u + static_cast<size_t>(extraSize)) return nullptr;
        outSize += extraSize;
    }
    return payload;
}

inline std::string BuildPlayerMovingLog(const PlayerMovingView& pm) {
    std::ostringstream oss;

    if (pm.NetID() != 0)           oss << "NetID:" << pm.NetID() << "|";
    if (pm.CharacterState() != 0)  oss << "CharacterState:" << pm.CharacterState() << "|";
    if (pm.PlantingTree() != 0)    oss << "PlantingTree:" << pm.PlantingTree() << "|";

    if (pm.X() != 0.0f)            oss << "X:" << std::fixed << std::setprecision(2) << pm.X() << "|";
    if (pm.Y() != 0.0f)            oss << "Y:" << std::fixed << std::setprecision(2) << pm.Y() << "|";
    if (pm.XSpeed() != 0.0f)       oss << "XSpeed:" << std::fixed << std::setprecision(2) << pm.XSpeed() << "|";
    if (pm.YSpeed() != 0.0f)       oss << "YSpeed:" << std::fixed << std::setprecision(2) << pm.YSpeed() << "|";

    if (pm.PunchX() != 0)          oss << "PunchX:" << pm.PunchX() << "|";
    if (pm.PunchY() != 0)          oss << "PunchY:" << pm.PunchY() << "|";

    if (pm.HasExtra())             oss << "HasExtra|";

    auto modInfo = pm.GetModifyItemInfo();
    if (modInfo.valid) oss << "PACKET_MODIFY_ITEM_INVENTORY|" << "ItemID:" << modInfo.item_id << "|" << "Gained:" << static_cast<int>(modInfo.gained) << "|" << "Jump:" << static_cast<int>(modInfo.jump) << "|";

    return oss.str();
}