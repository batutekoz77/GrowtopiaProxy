#include "Player.hpp"
#include "../Packets/Packets.hpp"
#include "../Network/Handler.hpp"
#include "../Network/Network.hpp"
#include "../Items/Items.hpp"

#include <random>

PlayerState Player;


void PlayerState::Reset() {
    userID = -1;

    tankIDName.clear();
    player_age = 0;
    klv.clear();
    meta.clear();
    rid.clear();
    mac.clear();
    wk.clear();

    netID = -1;
    country.clear();
    world.clear();
    lastWorld.clear();
    saveWorld.clear();
    worldPlayers.clear();

    dropMode = false;
    trashMode = false;
    wrenchMode = false;
    autoChange = false;
    hideClothes = false;

    position.x = 0.0f;
    position.y = 0.0f;

    clothes.hair_shirt_pants = { 0,0,0 };
    clothes.feet_face_hand = { 0,0,0 };
    clothes.back_mask_necklace = { 0,0,0 };
    clothes.skinColor = 31;
    clothes.ances_unknown_unknown2 = { 0,0,0 };

    inventory.items.clear();
    inventory.size = 16;

    {
        std::lock_guard<std::mutex> lock(dialogSkipper.Skipper);
        dialogSkipper.DROP.clear();
        dialogSkipper.TRASH = false;
        dialogSkipper.SALES_MAN = false;
    }

    design.OnDialogRequest_RED = 0;
    design.OnDialogRequest_GREEN = 0;
    design.OnDialogRequest_BLUE = 0;
    design.OnDialogRequest_ALPHA = 160;

    autofarm.running = false;
    autofarm.id = -1;
    autofarm.X = -1.0f;
    autofarm.Y = -1.0f;
    autofarm.punchX = -1;
    autofarm.punchY = -1;

    autospam.running = false;
    autospam.text1 = "Hello World!";
    autospam.text2.clear();
    autospam.delay = 3600;
    autospam.rainbow = false;
}

// ---------------- Inventory ----------------
int PlayerState::Inventory::GetCount(int id) const {
    auto it = items.find(id);
    if (it != items.end()) return it->second;
    return 0;
}

bool PlayerState::Inventory::HasItem(int id) const {
    return items.find(id) != items.end();
}

void PlayerState::Inventory::AddItem(int id, int count) {
    if (count <= 0) return;
    auto it = items.find(id);
    if (it != items.end()) {
        int newCount = it->second + count;
        if (newCount > MAX_STACK) newCount = MAX_STACK;
        it->second = newCount;
        return;
    }
    if (items.size() >= static_cast<size_t>(size)) return;
    if (count > MAX_STACK) count = MAX_STACK;
    items[id] = count;
}

void PlayerState::Inventory::RemoveItem(int id, int count) {
    if (count <= 0) return;
    auto it = items.find(id);
    if (it == items.end()) return;
    int newCount = it->second - count;
    if (newCount > 200) newCount = 200;
    if (newCount <= 0) items.erase(it);
    else it->second = newCount;
}

void PlayerState::Inventory::ResetInventory(int newSize, const std::vector<std::pair<int, int>>& itemsVec) {
    items.clear();
    size = newSize;
    for (const auto& [id, count] : itemsVec) AddItem(id, count);
}


// ---------------- PlayerState public methods ----------------
std::string PlayerState::InventoryToString() const {
    std::string text;
    int buttonCount = 0;

    for (const auto& pair : inventory.items) {
        uint16_t id = pair.first;
        int amount = pair.second;

        auto it = std::find_if(items.begin(), items.end(), [id](const Items& i) { return i.id == id; });

        if (it != items.end()) {
            text += "text_scaling_string|The Best World Lock Sow!|\n";
            text += "add_button_with_icon||" + it->raw_name + "|staticBlueFrame|" + std::to_string(id) + "|" + std::to_string(amount) + "|\n";
            buttonCount++;
            if (buttonCount % 5 == 0) text += "add_button_with_icon||END_LIST|noflags|0|0|\n";
        }
    }

    return text;
}

int PlayerState::GetItemCount(int id) const {
    return inventory.GetCount(id);
}

bool PlayerState::HasItem(int id) const {
    return inventory.HasItem(id);
}

void PlayerState::AddItem(int id, int count) {
    inventory.AddItem(id, count);
}

void PlayerState::RemoveItem(int id, int count) {
    inventory.RemoveItem(id, count);
}

void PlayerState::ResetInventory(int newSize, const std::vector<std::pair<int, int>>& items) {
    inventory.ResetInventory(newSize, items);
}



// ---------------- Functions ----------------
void PlayerState::Warp(std::string worldName, ENetPeer* CLIENT, ENetPeer* SERVER) {
    std::string newWorld = worldName;
    std::transform(newWorld.begin(), newWorld.end(), newWorld.begin(), [](unsigned char c) { return std::toupper(c); });

    if (newWorld == "EXIT") {
        GamePacket<OnConsoleMessage> p;
        p.text = "Exit from what? Press back if you're done playing.";
        SendGamePacket(CLIENT, p);
        return;
    }
    for (char c : newWorld) if ((c < 'A' || c>'Z') && (c < '0' || c>'9')) {
        GamePacket<OnConsoleMessage> p;
        p.text = "Sorry, spaces and special characters are not allowed in world or door names.  Try again.";
        SendGamePacket(CLIENT, p);
        return;
    }
    if (newWorld.size() < 1 || newWorld.size() >= 24) {
        GamePacket<OnConsoleMessage> p;
        p.text = "`4To reduce confusion, that is not a valid world name.``  Try another?";
        SendGamePacket(CLIENT, p);
        return;
    }
    if (newWorld == "") newWorld = "START";

    GamePacket<OnConsoleMessage> p;
    p.text = "Warping to the world [" + newWorld + "]. This may take up to 2 seconds!";
    SendGamePacket(CLIENT, p);

    std::thread([newWorld = std::move(newWorld), SERVER]() {
        if (!Player.world.empty()) {
            std::string quitPacket = "action|quit_to_exit\n";
            {
                std::lock_guard<std::mutex> lock(Network.peer_mutex);
                SendPacket(SERVER, Network.NET_MESSAGE_GAME_MESSAGE, quitPacket.c_str(), static_cast<int>(quitPacket.length()));
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        }
        std::string joinPacket = "action|join_request\nname|" + newWorld + "\ninvitedWorld|0\n\n";
        {
            std::lock_guard<std::mutex> lock(Network.peer_mutex);
            SendPacket(SERVER, Network.NET_MESSAGE_GAME_MESSAGE, joinPacket.c_str(), static_cast<int>(joinPacket.length()));
        }
    }).detach();
}

void PlayerState::Drop(int id, int count, ENetPeer* CLIENT, ENetPeer* SERVER) {
    auto it = std::find_if(items.begin(), items.end(), [&id](const Items& i) { return i.id == id; });
    if (it != items.end()) {
        if (HasItem(id) && GetItemCount(id) >= count) {
            std::string firstPacket = "action|drop\n|itemID|" + std::to_string(id) + "\n";
            {
                std::lock_guard<std::mutex> lock(Network.peer_mutex);
                SendPacket(SERVER, Network.NET_MESSAGE_GENERIC_TEXT, firstPacket.c_str(), static_cast<int>(firstPacket.length()));
            }

            {
                std::lock_guard<std::mutex> lock(Player.dialogSkipper.Skipper);
                dialogSkipper.DROP.push_back(it->raw_name);
            }

            std::string secondPacket = "action|dialog_return\ndialog_name|drop_item\nitemID|" + std::to_string(id) + "|\ncount|" + std::to_string(count) + "\n";
            {
                std::lock_guard<std::mutex> lock(Network.peer_mutex);
                SendPacket(SERVER, Network.NET_MESSAGE_GENERIC_TEXT, secondPacket.c_str(), static_cast<int>(secondPacket.length()));
            }

            {
                GamePacket<OnConsoleMessage> p;
                p.text = "Dropping " + std::to_string(count) + " " + it->raw_name;
                SendGamePacket(CLIENT, p);
            }

            return;
        }
        else LOG_DEBUG("You don't have {}x {}", count, it->raw_name);
    }
    else LOG_ERROR("Item ID: {} couldn't find in the database!", id);
}

void PlayerState::Trash(int id, int count, ENetPeer* CLIENT, ENetPeer* SERVER) {
    auto it = std::find_if(items.begin(), items.end(), [&id](const Items& i) { return i.id == id; });
    if (it != items.end()) {
        if (HasItem(id) && GetItemCount(id) >= count) {
            std::string firstPacket = "action|trash\n|itemID|" + std::to_string(id) + "\n";
            {
                std::lock_guard<std::mutex> lock(Network.peer_mutex);
                SendPacket(SERVER, Network.NET_MESSAGE_GENERIC_TEXT, firstPacket.c_str(), static_cast<int>(firstPacket.length()));
            }

            {
                std::lock_guard<std::mutex> lock(Player.dialogSkipper.Skipper);
                dialogSkipper.TRASH = true;
            }

            std::string secondPacket = "action|dialog_return\ndialog_name|trash_item\nitemID|" + std::to_string(id) + "|\ncount|" + std::to_string(count) + "\n";
            {
                std::lock_guard<std::mutex> lock(Network.peer_mutex);
                SendPacket(SERVER, Network.NET_MESSAGE_GENERIC_TEXT, secondPacket.c_str(), static_cast<int>(secondPacket.length()));
            }

            {
                GamePacket<OnConsoleMessage> p;
                p.text = "Trashing " + std::to_string(count) + " " + it->raw_name;
                SendGamePacket(CLIENT, p);
            }

            return;
        }
        else LOG_DEBUG("You don't have {}x {}", count, it->raw_name);
    }
    else LOG_ERROR("Item ID: {} couldn't find in the database!", id);
}

void PlayerState::Punch(float X, float Y, int punchX, int punchY, ENetPeer* SERVER) {
    /* @debug
        [06:03:2026 17:05:26] [INFO ] [NETWORK] [CLIENT -> PROXY -> SERVER] [NET_MESSAGE_GAME_PACKET] [PACKET_TILE_CHANGE_REQUEST:3]
        PlantingTree:18|X:1772.00|Y:578.00|PunchX:56|PunchY:18|

        [06:03:2026 17:05:26] [INFO ] [NETWORK] [CLIENT -> PROXY -> SERVER] [NET_MESSAGE_GAME_PACKET] [PACKET_STATE:0]
        CharacterState:2592|X:1772.00|Y:578.00|PunchX:56|PunchY:18|
    */

    PlayerMovingPacket firstPunch;
    firstPunch
        .PacketType(Network.PACKET_TILE_CHANGE_REQUEST)
        .PlantingTree(18)
        .Position(X, Y)
        .Punch(punchX, punchY)
        .Send(SERVER);

    PlayerMovingPacket secondPunch;
    secondPunch
        .PacketType(Network.PACKET_STATE)
        .CharacterState(2592)
        .Position(X, Y)
        .Punch(punchX, punchY)
        .Send(SERVER);
}

void PlayerState::Place(int id, float X, float Y, int punchX, int punchY, ENetPeer* SERVER) {
    /* @debug
    * Place
        [06:03:2026 16:19:46] [INFO ] [NETWORK] [CLIENT -> PROXY -> SERVER] [NET_MESSAGE_GAME_PACKET] [PACKET_TILE_CHANGE_REQUEST:3]
        PlantingTree:2|X:709.00|Y:738.00|PunchX:23|PunchY:23|

        [06:03:2026 16:19:46] [INFO ] [NETWORK] [CLIENT -> PROXY -> SERVER] [NET_MESSAGE_GAME_PACKET] [PACKET_STATE:0]
        CharacterState:3104|PlantingTree:2|X:709.00|Y:738.00|PunchX:23|PunchY:23|
    */

    PlayerMovingPacket firstPlace;
    firstPlace
        .PacketType(Network.PACKET_TILE_CHANGE_REQUEST)
        .PlantingTree(id)
        .Position(X, Y)
        .Punch(punchX, punchY)
        .Send(SERVER);

    PlayerMovingPacket secondPlace;
    secondPlace
        .PacketType(Network.PACKET_STATE)
        .CharacterState(3104)
        .PlantingTree(id)
        .Position(X, Y)
        .Punch(punchX, punchY)
        .Send(SERVER);
}

bool PlayerState::AutofarmReady() {
    return autofarm.id > 0 &&
        autofarm.X > 0 &&
        autofarm.Y > 0 &&
        autofarm.punchX > 0 &&
        autofarm.punchY > 0;
}

void PlayerState::AutoFarm(ENetPeer* CLIENT, ENetPeer* SERVER) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> punchDelay(200, 250);
    std::uniform_int_distribution<int> placeDelay(520, 570);

    while (autofarm.running) {
        if (AutofarmReady() && SERVER) {
            for (int i = 0; i < 3; i++) {
                {
                    std::lock_guard<std::mutex> lock(Network.peer_mutex);
                    Punch(autofarm.X, autofarm.Y, autofarm.punchX, autofarm.punchY, SERVER);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(punchDelay(gen)));
            }
            
            if (HasItem(autofarm.id)) {
                std::lock_guard<std::mutex> lock(Network.peer_mutex);
                Place(autofarm.id, autofarm.X, autofarm.Y, autofarm.punchX, autofarm.punchY, SERVER);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(placeDelay(gen)));
        }
        else std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void PlayerState::AutoSpam(ENetPeer* CLIENT, ENetPeer* SERVER) {
    static const std::vector<std::string> colors = {
        "`0","`1","`2","`3","`4","`5","`6","`7","`8","`9",
        "`b","`c","`e","`g","`j","`o","`p","`s","`w"
    };

    static std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<size_t> dis(0, colors.size() - 1);

    while (autospam.running) {
        std::string text1 = "action|input\n|text|" + (autospam.rainbow ? colors[dis(gen)] : "") + autospam.text1 + "\n";
        {
            std::lock_guard<std::mutex> lock(Network.peer_mutex);
            SendPacket(SERVER, Network.NET_MESSAGE_GENERIC_TEXT, text1.c_str(), static_cast<int>(text1.length()));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(autospam.delay));


        std::string text2 = "action|input\n|text|" + (autospam.rainbow ? colors[dis(gen)] : "") + (autospam.text2.empty() ? autospam.text1 : autospam.text2) + "\n";
        {
            std::lock_guard<std::mutex> lock(Network.peer_mutex);
            SendPacket(SERVER, Network.NET_MESSAGE_GENERIC_TEXT, text2.c_str(), static_cast<int>(text2.length()));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(autospam.delay));
    }
}