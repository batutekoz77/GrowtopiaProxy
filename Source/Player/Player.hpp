#pragma once

#include <string>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <mutex>

#include "ENET/enet.h"

struct PlayerVec2 { float x; float y; };
struct PlayerVec3 { float x; float y; float z; };


class PlayerState {
public:
    int userID = -1;


    std::string tankIDName = "";
    int player_age = 0;
    std::string klv = "", meta = "", rid = "", mac = "", wk = "";


    int netID = -1;
    std::string country = "";
    std::string world = "", lastWorld = "", saveWorld = "";
    std::unordered_map<int, std::string> worldPlayers;


    bool dropMode = false;
    bool trashMode = false;
    bool wrenchMode = false;
    bool autoChange = false;
    bool hideClothes = false;

    struct Position {
        float x = 0.0f;
        float y = 0.0f;
    }; Position position;


    struct Clothes {
        PlayerVec3 hair_shirt_pants{ 0.000000f, 0.000000f, 0.000000f };
        PlayerVec3 feet_face_hand{ 0.000000f, 0.000000f, 0.000000f };
        PlayerVec3 back_mask_necklace{ 0.000000f, 0.000000f, 0.000000f };
        uint32_t skinColor = 31;
        PlayerVec3 ances_unknown_unknown2{ 0.000000f, 0.000000f, 0.000000f };
    }; Clothes clothes;


    struct Inventory {
        static constexpr int MAX_STACK = 200;
        int size = 16;
        std::unordered_map<int, int> items;

        int GetCount(int id) const;
        bool HasItem(int id) const;
        void AddItem(int id, int count);
        void RemoveItem(int id, int count);
        void ResetInventory(int newSize, const std::vector<std::pair<int, int>>& items);
    };


    struct DialogSkipper {
        std::mutex Skipper;
        std::vector<std::string> DROP;
        bool TRASH = false;
        bool SALES_MAN = false;
    }; DialogSkipper dialogSkipper;


    struct Design {
        int OnDialogRequest_RED = 0;
        int OnDialogRequest_GREEN = 0;
        int OnDialogRequest_BLUE = 0;
        int OnDialogRequest_ALPHA = 160;
    }; Design design;


    struct Autofarm {
        std::atomic<bool> running = false;
        int id = -1;
        float X = -1.00f, Y = -1.00f;
        int punchX = -1, punchY = -1;
        std::thread worker;
    }; Autofarm autofarm;


    struct Autospam {
        std::atomic<bool> running = false;
        std::string text1 = "Hello World!";
        std::string text2 = "";
        int delay = 3600;
        bool rainbow = false;
        std::thread worker;
    }; Autospam autospam;

private:
    Inventory inventory;

public:
    void Reset();

    std::string InventoryToString() const;
    int GetItemCount(int id) const;
    bool HasItem(int id) const;
    void AddItem(int id, int count);
    void RemoveItem(int id, int count);
    void ResetInventory(int newSize, const std::vector<std::pair<int, int>>& items);

public:
    void Warp(std::string worldName, ENetPeer* CLIENT, ENetPeer* SERVER);

    void Drop(int id, int count, ENetPeer* CLIENT, ENetPeer* SERVER);
    void Trash(int id, int count, ENetPeer* CLIENT, ENetPeer* SERVER);

    void Punch(float X, float Y, int punchX, int punchY, ENetPeer* SERVER);
    void Place(int id, float X, float Y, int punchX, int punchY, ENetPeer* SERVER);

    bool AutofarmReady();
    void AutoFarm(ENetPeer* CLIENT, ENetPeer* SERVER);

    void AutoSpam(ENetPeer* CLIENT, ENetPeer* SERVER);
};

extern PlayerState Player;