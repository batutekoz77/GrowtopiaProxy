#pragma once

#include "ENET/enet.h"
#include <string>
#include <unordered_map>
#include <mutex>

class NetworkManager {
public:
    std::string Setup(int which);
    void Injector();

    ENetHost* GetProxyHost();
    ENetHost* GetServerHost();

    ENetAddress& GetProxyAddress();
    ENetAddress& GetServerAddress();


public:
    std::unordered_map<ENetPeer*, ENetPeer*> client_to_server;
    std::unordered_map<ENetPeer*, ENetPeer*> server_to_client;

    std::mutex peer_mutex;

public:
    enum {
        CLIENT_PROXY_SERVER = 0,
        SERVER_PROXY_CLIENT = 1,
    };

    enum {
        NET_MESSAGE_UNKNOWN = 0,
        NET_MESSAGE_SERVER_HELLO = 1,
        NET_MESSAGE_GENERIC_TEXT = 2,
        NET_MESSAGE_GAME_MESSAGE = 3,
        NET_MESSAGE_GAME_PACKET = 4,
        NET_MESSAGE_ERROR = 5,
        NET_MESSAGE_TRACK = 6,
        NET_MESSAGE_CLIENT_LOG_REQUEST = 7,
        NET_MESSAGE_CLIENT_LOG_RESPONSE = 8,

        NET_MESSAGE_MAXVAL = 9 // fake, overflow guard
    };

    std::string RECEIVE_TYPE_STRING(int type) {
        switch (type) {
            case 1: return "NET_MESSAGE_SERVER_HELLO";
            case 2: return "NET_MESSAGE_GENERIC_TEXT";
            case 3: return "NET_MESSAGE_GAME_MESSAGE";
            case 4: return "NET_MESSAGE_GAME_PACKET";
            case 5: return "NET_MESSAGE_ERROR";
            case 6: return "NET_MESSAGE_TRACK";
            case 7: return "NET_MESSAGE_CLIENT_LOG_REQUEST";
            case 8: return "NET_MESSAGE_CLIENT_LOG_RESPONSE";
            case 0:
            default:
            { return "NET_MESSAGE_UNKNOWN"; }
        }
    }

    enum {
        LOG_NET_MESSAGE_GAME_PACKET_BASIC = 0,
        LOG_NET_MESSAGE_GAME_PACKET_BASIC_CLEAR_SPAM = 1,
        LOG_NET_MESSAGE_GAME_PACKET_PLAYER_MOVING = 2,
    };

    enum {
        PACKET_STATE = 0,
        PACKET_CALL_FUNCTION = 1,
        PACKET_UPDATE_STATUS = 2,
        PACKET_TILE_CHANGE_REQUEST = 3,
        PACKET_SEND_MAP_DATA = 4,
        PACKET_SEND_TILE_UPDATE_DATA = 5,
        PACKET_SEND_TILE_UPDATE_DATA_MULTIPLE = 6,
        PACKET_TILE_ACTIVATE_REQUEST = 7,
        PACKET_TILE_APPLY_DAMAGE = 8,
        PACKET_SEND_INVENTORY_STATE = 9,
        PACKET_ITEM_ACTIVATE_REQUEST = 10,
        PACKET_ITEM_ACTIVATE_OBJECT_REQUEST = 11,
        PACKET_SEND_TILE_TREE_STATE = 12,
        PACKET_MODIFY_ITEM_INVENTORY = 13,
        PACKET_ITEM_CHANGE_OBJECT = 14,
        PACKET_SEND_LOCK = 15,
        PACKET_SEND_ITEM_DATABASE_DATA = 16,
        PACKET_SEND_PARTICLE_EFFECT = 17,
        PACKET_SET_ICON_STATE = 18,
        PACKET_ITEM_EFFECT = 19,
        PACKET_SET_CHARACTER_STATE = 20,
        PACKET_PING_REPLY = 21,
        PACKET_PING_REQUEST = 22,
        PACKET_GOT_PUNCHED = 23,
        PACKET_APP_CHECK_RESPONSE = 24,
        PACKET_APP_INTEGRITY_FAIL = 25,
        PACKET_DISCONNECT = 26,
        PACKET_BATTLE_JOIN = 27,
        PACKET_BATTLE_EVENT = 28,
        PACKET_USE_DOOR = 29,
        PACKET_SEND_PARENTAL = 30,
        PACKET_GONE_FISHIN = 31,
        PACKET_STEAM = 32,
        PACKET_PET_BATTLE = 33,
        PACKET_NPC = 34,
        PACKET_SPECIAL = 35,
        PACKET_SEND_PARTICLE_EFFECT_V2 = 36,
        PACKET_ACTIVE_ARROW_TO_ITEM = 37,
        PACKET_SELECT_TILE_INDEX = 38,
        PACKET_SEND_PLAYER_TRIBUTE_DATA = 39,
        PACKET_FTUE_SET_ITEM_TO_QUICK_INVENTORY = 40,
        PACKET_PVE_NPC = 41,
        PACKET_PVP_CARD_BATTLE = 42,
        PACKET_PVE_APPLY_PLAYER_DAMAGE = 43,
        PACKET_PVE_NPC_POSITION_DAMAGE = 44,
        PACKET_SET_EXTRA_MODS = 45,
        PACKET_ON_STEP_ON_TILE_MOD = 46,

        PACKET_MAXVAL = 47 // fake, overflow guard
    };

    std::string PLAYER_MOVING_STRING(int type) {
        switch (type) {
            case 0:   return "PACKET_STATE";
            case 1:   return "PACKET_CALL_FUNCTION";
            case 2:   return "PACKET_UPDATE_STATUS";
            case 3:   return "PACKET_TILE_CHANGE_REQUEST";
            case 4:   return "PACKET_SEND_MAP_DATA";
            case 5:   return "PACKET_SEND_TILE_UPDATE_DATA";
            case 6:   return "PACKET_SEND_TILE_UPDATE_DATA_MULTIPLE";
            case 7:   return "PACKET_TILE_ACTIVATE_REQUEST";
            case 8:   return "PACKET_TILE_APPLY_DAMAGE";
            case 9:   return "PACKET_SEND_INVENTORY_STATE";
            case 10:  return "PACKET_ITEM_ACTIVATE_REQUEST";
            case 11:  return "PACKET_ITEM_ACTIVATE_OBJECT_REQUEST";
            case 12:  return "PACKET_SEND_TILE_TREE_STATE";
            case 13:  return "PACKET_MODIFY_ITEM_INVENTORY";
            case 14:  return "PACKET_ITEM_CHANGE_OBJECT";
            case 15:  return "PACKET_SEND_LOCK";
            case 16:  return "PACKET_SEND_ITEM_DATABASE_DATA";
            case 17:  return "PACKET_SEND_PARTICLE_EFFECT";
            case 18:  return "PACKET_SET_ICON_STATE";
            case 19:  return "PACKET_ITEM_EFFECT";
            case 20:  return "PACKET_SET_CHARACTER_STATE";
            case 21:  return "PACKET_PING_REPLY";
            case 22:  return "PACKET_PING_REQUEST";
            case 23:  return "PACKET_GOT_PUNCHED";
            case 24:  return "PACKET_APP_CHECK_RESPONSE";
            case 25:  return "PACKET_APP_INTEGRITY_FAIL";
            case 26:  return "PACKET_DISCONNECT";
            case 27:  return "PACKET_BATTLE_JOIN";
            case 28:  return "PACKET_BATTLE_EVENT";
            case 29:  return "PACKET_USE_DOOR";
            case 30:  return "PACKET_SEND_PARENTAL";
            case 31:  return "PACKET_GONE_FISHIN";
            case 32:  return "PACKET_STEAM";
            case 33:  return "PACKET_PET_BATTLE";
            case 34:  return "PACKET_NPC";
            case 35:  return "PACKET_SPECIAL";
            case 36:  return "PACKET_SEND_PARTICLE_EFFECT_V2";
            case 37:  return "PACKET_ACTIVE_ARROW_TO_ITEM";
            case 38:  return "PACKET_SELECT_TILE_INDEX";
            case 39:  return "PACKET_SEND_PLAYER_TRIBUTE_DATA";
            case 40:  return "PACKET_FTUE_SET_ITEM_TO_QUICK_INVENTORY";
            case 41:  return "PACKET_PVE_NPC";
            case 42:  return "PACKET_PVP_CARD_BATTLE";
            case 43:  return "PACKET_PVE_APPLY_PLAYER_DAMAGE";
            case 44:  return "PACKET_PVE_NPC_POSITION_DAMAGE";
            case 45:  return "PACKET_SET_EXTRA_MODS";
            case 46:  return "PACKET_ON_STEP_ON_TILE_MOD";
            default:  return "PACKET_UNKNOWN";
        }
    }


private:
    int STATUS = 0;

    ENetEvent PROXY_EVENT{};
    ENetEvent SERVER_EVENT{};

    ENetHost* PROXY_HOST = nullptr;
    ENetHost* SERVER_HOST = nullptr;

    ENetAddress PROXY_ADDRESS{};
    ENetAddress SERVER_ADDRESS{};

};
extern NetworkManager Network;