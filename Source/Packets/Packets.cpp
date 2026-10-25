#include "Packets.hpp"
#include "../Network/Network.hpp"
#include "../Player/Player.hpp"

std::string type;

GamePacketBuilder::GamePacketBuilder(int delay, int NetID) : packet_data(61) {
    len = 61;
    int MessageType = 0x4, PacketType = 0x1, CharState = 0x8;
    std::fill(packet_data.begin(), packet_data.end(), 0);
    std::copy(reinterpret_cast<const BYTE*>(&MessageType), reinterpret_cast<const BYTE*>(&MessageType) + sizeof(MessageType), packet_data.data());
    std::copy(reinterpret_cast<const BYTE*>(&PacketType), reinterpret_cast<const BYTE*>(&PacketType) + sizeof(PacketType), packet_data.data() + 4);
    std::copy(reinterpret_cast<const BYTE*>(&NetID), reinterpret_cast<const BYTE*>(&NetID) + sizeof(NetID), packet_data.data() + 8);
    std::copy(reinterpret_cast<const BYTE*>(&CharState), reinterpret_cast<const BYTE*>(&CharState) + sizeof(CharState), packet_data.data() + 16);
    std::copy(reinterpret_cast<const BYTE*>(&delay), reinterpret_cast<const BYTE*>(&delay) + sizeof(delay), packet_data.data() + 24);
}

GamePacketBuilder& GamePacketBuilder::Insert(const std::string& a) {
    std::string content = a;
    if (type == "OnDialogRequest") content += "\nset_border_color|180,0,0,200|\nset_bg_color|" + std::to_string(Player.design.OnDialogRequest_RED) + "," + std::to_string(Player.design.OnDialogRequest_GREEN) + "," + std::to_string(Player.design.OnDialogRequest_BLUE) + "," + std::to_string(Player.design.OnDialogRequest_ALPHA) + "|";
    type = a;

    int a_len = static_cast<int>(content.length());
    packet_data.resize(len + 2 + a_len + 4);
    packet_data[len] = index;
    packet_data[len + 1] = 0x2;
    std::copy(reinterpret_cast<const BYTE*>(&a_len), reinterpret_cast<const BYTE*>(&a_len) + sizeof(a_len), packet_data.data() + len + 2);
    std::copy(content.begin(), content.end(), packet_data.data() + len + 6);
    len += 2 + a_len + 4;
    index++;
    packet_data[60] = index;
    return *this;
}

GamePacketBuilder& GamePacketBuilder::Insert(int a) {
    return InsertValue(a, 0x9);
}

GamePacketBuilder& GamePacketBuilder::Insert(unsigned int a) {
    return InsertValue(a, 0x5);
}

GamePacketBuilder& GamePacketBuilder::Insert(float a) {
    return InsertValue(a, 0x1);
}

GamePacketBuilder& GamePacketBuilder::Insert(float a, float b) {
    packet_data.resize(len + 10);
    packet_data[len] = index;
    packet_data[len + 1] = 0x3;
    std::copy(reinterpret_cast<const BYTE*>(&a), reinterpret_cast<const BYTE*>(&a) + sizeof(a), packet_data.data() + len + 2);
    std::copy(reinterpret_cast<const BYTE*>(&b), reinterpret_cast<const BYTE*>(&b) + sizeof(b), packet_data.data() + len + 6);
    len += 10;
    index++;
    packet_data[60] = index;
    return *this;
}

GamePacketBuilder& GamePacketBuilder::Insert(float a, float b, float c) {
    packet_data.resize(len + 14);
    packet_data[len] = index;
    packet_data[len + 1] = 0x4;
    std::copy(reinterpret_cast<const BYTE*>(&a), reinterpret_cast<const BYTE*>(&a) + sizeof(a), packet_data.data() + len + 2);
    std::copy(reinterpret_cast<const BYTE*>(&b), reinterpret_cast<const BYTE*>(&b) + sizeof(b), packet_data.data() + len + 6);
    std::copy(reinterpret_cast<const BYTE*>(&c), reinterpret_cast<const BYTE*>(&c) + sizeof(c), packet_data.data() + len + 10);
    len += 14;
    index++;
    packet_data[60] = index;
    return *this;
}

GamePacketBuilder& GamePacketBuilder::Insert(long long int a) {
    return InsertValue(a, 0x6);
}

void GamePacketBuilder::CreatePacket(ENetPeer* peer) {
    ENetPacket* packet = enet_packet_create(packet_data.data(), len, 1);
    {
        std::lock_guard<std::mutex> lock(Network.peer_mutex);
        enet_peer_send(peer, 0, packet);
    }
}

template<typename T>
GamePacketBuilder& GamePacketBuilder::InsertValue(const T& value, BYTE type) {
    size_t size = sizeof(T);
    packet_data.resize(len + 2 + size);
    packet_data[len] = (BYTE)index;
    packet_data[len + 1] = type;
    std::copy(reinterpret_cast<const BYTE*>(&value), reinterpret_cast<const BYTE*>(&value) + size, packet_data.data() + len + 2);
    len += static_cast<int>(2 + size);
    index++;
    packet_data[60] = (BYTE)index;
    return *this;
}