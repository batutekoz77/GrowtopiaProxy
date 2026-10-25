#include "Handler.hpp"
#include "Network.hpp"
#include "../Main/Config.hpp"
#include "../Logger/Logger.hpp"
#include "../Functions/Functions.hpp"
#include "../Packets/Packets.hpp"
#include "../Player/Player.hpp"
#include "../Items/Items.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#include <iomanip>
#include <sstream>
#include <vector>
#include <variant>

#include "NET_MESSAGE_GENERIC_TEXT/COMMANDS.HPP"
#include "NET_MESSAGE_GENERIC_TEXT/PROXY_DIALOGS.HPP"
#include "NET_MESSAGE_GENERIC_TEXT/WRENCH.HPP"

#include "NET_MESSAGE_GAME_PACKET/PACKET_STATE.hpp"
#include "NET_MESSAGE_GAME_PACKET/PACKET_CALL_FUNCTION.hpp"
#include "NET_MESSAGE_GAME_PACKET/PACKET_TILE_CHANGE_REQUEST.hpp"
#include "NET_MESSAGE_GAME_PACKET/PACKET_SEND_MAP_DATA.hpp"
#include "NET_MESSAGE_GAME_PACKET/PACKET_SEND_INVENTORY_STATE.hpp"
#include "NET_MESSAGE_GAME_PACKET/PACKET_MODIFY_ITEM_INVENTORY.hpp"

int GetPacketType(const ENetPacket * packet) {
    if (!packet || packet->dataLength < 1) return -1;
    return *(int*)(packet->data);
}

std::string GetPacketText(ENetPacket* packet) {
    if (!packet) return "Unknown Packet Text";
    char zero = 0;
    memcpy(packet->data + packet->dataLength - 1, &zero, 1);
    return std::string((char*)(packet->data + 4));
}

std::string GetPacketHex(const ENetPacket* packet) {
    if (!packet || packet->dataLength == 0) return "Unknown Packet Hex";
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < packet->dataLength; ++i) oss << std::setw(2) << static_cast<unsigned>(packet->data[i]) << " ";
    return oss.str();
}

void SendPacket(ENetPeer* peer, int num, const char* data, size_t len) {
    ENetPacket* packet = enet_packet_create(nullptr, len + 5, ENET_PACKET_FLAG_RELIABLE);
    if (!packet) return;
    memcpy(packet->data, &num, 4);
    if (data && len > 0) memcpy(packet->data + 4, data, len);
    packet->data[4 + len] = 0x00;
    enet_peer_send(peer, 0, packet);
    /* SendPacket(event.peer, type, string.c_str(), string.length()); */
}

bool HandlePacket(int type, ENetPacket* packet, ENetPeer* to) {
    /* @important: if you want to forward the packet without editing (the original) then use 'return true;'
        if you want to edit, after editing the packet, use 'return false;' so it won't send the original packet but your custom packet
    */

    if (!packet) return true;

    ENetPeer* CLIENT = nullptr;
    ENetPeer* SERVER = nullptr;

    std::string Method;
    if (type == Network.CLIENT_PROXY_SERVER) {
        Method = "CLIENT -> PROXY -> SERVER";
        {
            std::lock_guard<std::mutex> lock(Network.peer_mutex);
            auto it = Network.server_to_client.find(to);
            if (it != Network.server_to_client.end()) {
                SERVER = to;
                CLIENT = it->second;
            }
        }
    }
    else if (type == Network.SERVER_PROXY_CLIENT) {
        Method = "SERVER -> PROXY -> CLIENT";
        {
            std::lock_guard<std::mutex> lock(Network.peer_mutex);
            auto it = Network.client_to_server.find(to);
            if (it != Network.client_to_server.end()) {
                CLIENT = to;
                SERVER = it->second;
            }
        }
    }

    int Type = GetPacketType(packet);

    switch (Type) {
    case Network.NET_MESSAGE_SERVER_HELLO: {
        /* @info: Don't edit this packet */
        if (!Main.BASIC_LOGS && ShouldLogNetMessage(Type)) LOG_INFO("[{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), GetPacketHex(packet));
        return true;
    }

    case Network.NET_MESSAGE_GENERIC_TEXT: {
        std::string text = GetPacketText(packet);
        if (ShouldLogNetMessage(Type)) LOG_INFO("[{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), text);

        /* @info: Edit here */

        if (Packet::Contains(text, "tankIDName") && Packet::Contains(text, "tankIDPass") &&
            Packet::Contains(text, "player_age") && Packet::Contains(text, "klv") &&
            Packet::Contains(text, "meta") && Packet::Contains(text, "rid") &&
            Packet::Contains(text, "mac") && Packet::Contains(text, "wk")) {

            if (Packet::ContainsValue<std::string>(text, "tankIDName")) Player.tankIDName = Packet::ExtractValue<std::string>(text, "tankIDName", "");
            if (Packet::ContainsValue<std::string>(text, "player_age")) Player.player_age = Packet::ExtractValue<int>(text, "player_age", 0);
            if (Packet::ContainsValue<std::string>(text, "klv")) Player.klv = Packet::ExtractValue<std::string>(text, "klv", "");
            if (Packet::ContainsValue<std::string>(text, "meta")) Player.meta = Packet::ExtractValue<std::string>(text, "meta", "");
            if (Packet::ContainsValue<std::string>(text, "rid")) Player.rid = Packet::ExtractValue<std::string>(text, "rid", "");
            if (Packet::ContainsValue<std::string>(text, "mac")) Player.mac = Packet::ExtractValue<std::string>(text, "mac", "");
            if (Packet::ContainsValue<std::string>(text, "wk")) Player.wk = Packet::ExtractValue<std::string>(text, "wk", "");

            return true;
        }
        else if (Packet::Contains(text, "action|dialog_return\ndialog_name|PROXY_DIALOGS")) return PROXY_DIALOGS::Inject(text, CLIENT, SERVER);
        else if (Packet::ContainsValue<std::string>(text, "action")) {
            std::string action = Packet::ExtractValue<std::string>(text, "action", "");

            if (Packet::Contains(text, "action|enter_game") && !Packet::Contains(text, "action|enter_game\ninvitedWorld|")) {
                /* @important: never let this kill the proxy. It only
                   builds a local item-name table -- im_data never leaves
                   this process -- so a failure costs item names and
                   nothing else. It used to be an uncaught throw on
                   enter_game. */
                if (!ALREADY_INJECTED) {
                    try { LOG_INFO("{}", InjectItems()); }
                    catch (const std::exception& e) {
                        ALREADY_INJECTED = true;
                        LOG_ERROR("items.dat injection failed: {}", e.what());
                        LOG_ERROR("Continuing without item names.");
                    }
                    catch (...) {
                        ALREADY_INJECTED = true;
                        LOG_ERROR("items.dat injection failed with an unknown error.");
                        LOG_ERROR("Continuing without item names.");
                    }
                }
                return true;
            }
            else if (action == "input") {
                std::string command = Packet::ExtractValue<std::string>(text, "text", "");
                if (!command.empty() && command[0] == '/') return COMMANDS::Inject(command, CLIENT, SERVER);
            }
            else if (action == "wrench") return WRENCH::Inject(text, CLIENT, SERVER);
            else if ((Player.dropMode && action == "drop") || (Player.trashMode && action == "trash")) {
                int itemID = Packet::ExtractValue<int>(text, "itemID", -1);
                if (itemID > 0) {
                    if (action == "drop") Player.Drop(itemID, Player.GetItemCount(itemID), CLIENT, SERVER);
                    else Player.Trash(itemID, Player.GetItemCount(itemID), CLIENT, SERVER);
                }
                return true;
            }
        }

        return true;
    }
    case Network.NET_MESSAGE_GAME_MESSAGE: {
        std::string text = GetPacketText(packet);
        if (ShouldLogNetMessage(Type)) LOG_INFO("[{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), text);

        /* @info: Edit here */

        if (Packet::Contains(text, "action|quit_to_exit")) {
            if (!Player.world.empty()) Player.lastWorld = Player.world;
            Player.world.clear();
            if (!Player.worldPlayers.empty()) Player.worldPlayers.clear();

            if (Player.autofarm.running) {
                Player.autofarm.running = false;
                if (Player.autofarm.worker.joinable()) Player.autofarm.worker.join();
                Player.autofarm.id = -1;
                Player.autofarm.X = -1.0f;
                Player.autofarm.Y = -1.0f;
                Player.autofarm.punchX = -1;
                Player.autofarm.punchY = -1;

                std::string text = "Stopped auto-farming!";

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
                    p.seconds = 0;
                    SendGamePacket(CLIENT, p, Player.netID);
                }
            }

            return true;
        }
        else if (Packet::Contains(text, "action|quit") && Method == "CLIENT -> PROXY -> SERVER") {
            Player.Reset();
            return true;
        }

        return true;
    }

    case Network.NET_MESSAGE_GAME_PACKET: {
        size_t payloadSize = 0;
        const BYTE* payload = get_packet_payload(packet, payloadSize);
        if (!payload) return true;

        PlayerMovingView pm(payload, payloadSize);

        if (!Main.BASIC_LOGS && ShouldLogNetMessage(Type, Network.LOG_NET_MESSAGE_GAME_PACKET_PLAYER_MOVING) && Type != Network.PACKET_NPC) LOG_INFO("[{}] [{}] [{}:{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), pm.PacketType(), BuildPlayerMovingLog(pm));

        switch (pm.PacketType()) {
        case Network.PACKET_STATE: {
            return PACKET_STATE::Inject(Method, Type, packet, CLIENT, SERVER, pm);
        }
        case Network.PACKET_CALL_FUNCTION: {
            return PACKET_CALL_FUNCTION::Inject(Method, Type, packet, CLIENT, SERVER);
        }
        case Network.PACKET_TILE_CHANGE_REQUEST: {
            return PACKET_TILE_CHANGE_REQUEST::Inject(Method, Type, packet, CLIENT, SERVER, pm);
        }
        case Network.PACKET_SEND_MAP_DATA: {
            return PACKET_SEND_MAP_DATA::Inject(Method, Type, packet, CLIENT, SERVER, pm);
        }
        case Network.PACKET_SEND_INVENTORY_STATE: {
            return PACKET_SEND_INVENTORY_STATE::Inject(Method, Type, packet, CLIENT, SERVER);
        }
        case Network.PACKET_MODIFY_ITEM_INVENTORY: {
            return PACKET_MODIFY_ITEM_INVENTORY::Inject(Method, Type, packet, CLIENT, SERVER, pm);
        }
        default: {
            return true;
        }
        }
        return true;
    }
    case Network.NET_MESSAGE_ERROR: {
        /* @info: Don't edit this packet */
        std::string text = GetPacketText(packet);
        if (!Main.BASIC_LOGS && ShouldLogNetMessage(Type)) LOG_WARN("[{}] [{}]\n{}\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), text, GetPacketHex(packet));
        return true;
    }
    case Network.NET_MESSAGE_TRACK: {
        /* @info: Don't edit this packet */
        std::string text = GetPacketText(packet);
        if (!Main.BASIC_LOGS && ShouldLogNetMessage(Type)) LOG_INFO("[{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), text);
        return true;
    }

    case Network.NET_MESSAGE_CLIENT_LOG_REQUEST: {
        /* @info: Don't edit this packet */
        std::string text = GetPacketText(packet);
        if (!Main.BASIC_LOGS && ShouldLogNetMessage(Type)) LOG_INFO("[{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), text);
        return true;
    }
    case Network.NET_MESSAGE_CLIENT_LOG_RESPONSE: {
        /* @info: Don't edit this packet */
        std::string text = GetPacketText(packet);
        if (!Main.BASIC_LOGS && ShouldLogNetMessage(Type)) LOG_WARN("[{}] [{}]\n{}\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), text, GetPacketHex(packet));
        return true;
    }

    case Network.NET_MESSAGE_UNKNOWN:
    default:
    {
        /* @info: We don't know what is this */
        std::string text = GetPacketText(packet);
        if (!Main.BASIC_LOGS && ShouldLogNetMessage(Network.NET_MESSAGE_UNKNOWN)) LOG_ERROR("[{}] [{}]\n{}\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), text, GetPacketHex(packet));
        return true;
    }
    }

    return true;
}