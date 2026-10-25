#pragma once

#include "../Network.hpp"
#include "../../Main/Config.hpp"
#include "../../Packets/Packets.hpp"
#include "../../Logger/Logger.hpp"
#include "../../Functions/Functions.hpp"

#include <regex>
#include <variant>
#include <vector>
#include <cstring>

class PACKET_CALL_FUNCTION {
public:
    static inline bool Inject(std::string Method, int Type, ENetPacket* packet, ENetPeer* CLIENT, ENetPeer* SERVER) {
        std::string debug, text;

        size_t cursor = 61;
        int variantCount = packet->data[60];
        if (variantCount < 0 || variantCount > 100) variantCount = 100;
        int readCount = 0;

        struct Vec2 { float x; float y; };
        struct Vec3 { float x; float y; float z; };

        using Variant = std::variant<std::string, int, unsigned int, float, long long, Vec2, Vec3>;
        std::vector<Variant> param;

        if (packet->dataLength > 61) {
            while (cursor + 2 <= packet->dataLength && readCount < variantCount) {

                uint8_t idx = packet->data[cursor];
                uint8_t vtype = packet->data[cursor + 1];
                cursor += 2;
                readCount++;

                switch (vtype) {

                case 0x2: { // STRING
                    if (cursor + sizeof(int) > packet->dataLength) return true;

                    int slen = ReadInt(packet->data, cursor);
                    if (slen < 0 || cursor + slen > packet->dataLength) return true;

                    std::string value((char*)packet->data + cursor, slen);
                    cursor += slen;

                    // OnClearItemTransforms

                    if (ShouldLogNetMessage(Type, Network.LOG_NET_MESSAGE_GAME_PACKET_BASIC_CLEAR_SPAM)&&
                        Packet::Contains(value, "OnClearItemTransforms")
                    ) return true;

                    debug += "[" + std::to_string(idx) + "][STRING] " + value + "\n";
                    param.push_back(value);
                    break;
                }

                case 0x1: { // FLOAT
                    if (cursor + sizeof(float) > packet->dataLength) return true;

                    float v = ReadFloat(packet->data, cursor);
                    debug += "[" + std::to_string(idx) + "][FLOAT] " + std::to_string(v) + "\n";
                    param.push_back(v);
                    break;
                }

                case 0x3: { // Vec2
                    if (cursor + sizeof(float) * 2 > packet->dataLength) return true;

                    Vec2 custom;
                    custom.x = ReadFloat(packet->data, cursor);
                    custom.y = ReadFloat(packet->data, cursor);

                    debug += "[" + std::to_string(idx) + "][FLOAT2] "
                        + std::to_string(custom.x) + ", "
                        + std::to_string(custom.y) + "\n";

                    param.push_back(custom);
                    break;
                }

                case 0x4: { // Vec3
                    if (cursor + sizeof(float) * 3 > packet->dataLength) return true;

                    Vec3 custom;
                    custom.x = ReadFloat(packet->data, cursor);
                    custom.y = ReadFloat(packet->data, cursor);
                    custom.z = ReadFloat(packet->data, cursor);

                    debug += "[" + std::to_string(idx) + "][FLOAT3] "
                        + std::to_string(custom.x) + ", "
                        + std::to_string(custom.y) + ", "
                        + std::to_string(custom.z) + "\n";

                    param.push_back(custom);
                    break;
                }

                case 0x5: { // UINT
                    if (cursor + sizeof(unsigned int) > packet->dataLength) return true;

                    unsigned int v = ReadUInt(packet->data, cursor);
                    debug += "[" + std::to_string(idx) + "][UINT] " + std::to_string(v) + "\n";
                    param.push_back(v);
                    break;
                }

                case 0x6: { // INT64
                    if (cursor + sizeof(long long) > packet->dataLength) return true;

                    long long v = ReadInt64(packet->data, cursor);
                    debug += "[" + std::to_string(idx) + "][INT64] " + std::to_string(v) + "\n";
                    param.push_back(v);
                    break;
                }

                case 0x9: { // INT
                    if (cursor + sizeof(int) > packet->dataLength) return true;

                    int v = ReadInt(packet->data, cursor);
                    debug += "[" + std::to_string(idx) + "][INT] " + std::to_string(v) + "\n";
                    param.push_back(v);
                    break;
                }

                default:
                    return true;
                }
            }
        }

        if (!debug.empty()) {
            if (ShouldLogNetMessage(Type, Network.LOG_NET_MESSAGE_GAME_PACKET_BASIC)) LOG_INFO("[{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), debug);

            /* @info: Edit here  */
            if (!param.empty()) {
                if (auto pStr = std::get_if<std::string>(&param[0])) {
                    if (std::get<std::string>(param[0]) == "OnDialogRequest" && param.size() == 2) {
                        std::string dialog = std::get<std::string>(param[1]);

                        if (!Player.dialogSkipper.DROP.empty() && Packet::Contains(dialog, "add_label_with_icon|big|`wDrop ") && Packet::Contains(dialog, "add_textbox|How many to drop?|left|")) {
                            for (size_t i = 0; i < Player.dialogSkipper.DROP.size(); ++i) {
                                if (Packet::Contains(dialog, Player.dialogSkipper.DROP[i])) {
                                    {
                                        std::lock_guard<std::mutex> lock(Player.dialogSkipper.Skipper);
                                        Player.dialogSkipper.DROP.erase(Player.dialogSkipper.DROP.begin() + i);
                                    }
                                    return false;
                                }
                            }
                        }
                        else if (Player.dialogSkipper.TRASH && Packet::Contains(dialog, "add_label_with_icon|big|`4Trash`` ") && Packet::Contains(dialog, "add_textbox|How many to `4destroy``?")) {
                            {
                                std::lock_guard<std::mutex> lock(Player.dialogSkipper.Skipper);
                                Player.dialogSkipper.TRASH = false;
                            }
                            return false;
                        }
                        else if (Player.autoChange && Packet::Contains(dialog, "add_label_with_icon|big|`wTelephone``|left|3898|")) {
                            int tilex = Packet::ExtractValue<int>(dialog, "tilex", -1), tiley = Packet::ExtractValue<int>(dialog, "tiley", -1);
                            
                            if (tilex != -1 && tiley != -1 && Player.GetItemCount(1796) >= 100 && Player.GetItemCount(7188) < 200) {
                                Player.dialogSkipper.SALES_MAN = true;

                                std::string sendNumber = "action|dialog_return\ndialog_name|phonecall\ntilex|" + std::to_string(tilex) + "|\ntiley|" + std::to_string(tiley) + "|\nnum|-2|\ndial|53785\n\n";
                                {
                                    std::lock_guard<std::mutex> lock(Network.peer_mutex);
                                    SendPacket(SERVER, Network.NET_MESSAGE_GENERIC_TEXT, sendNumber.c_str(), sendNumber.length());
                                }

                                return false;
                            }
                        }
                        else if (Player.dialogSkipper.SALES_MAN && Packet::Contains(dialog, "add_label_with_icon|big|`wSales-Man``|left|4358|")) {
                            int tilex = Packet::ExtractValue<int>(dialog, "tilex", -1), tiley = Packet::ExtractValue<int>(dialog, "tiley", -1);

                            if (tilex != -1 && tiley != -1 && Player.GetItemCount(1796) >= 100 && Player.GetItemCount(7188) < 200) {
                                std::string sendBGL = "action|dialog_return\ndialog_name|phonecall\ntilex|" + std::to_string(tilex) + "|\ntiley|" + std::to_string(tiley) + "|\nnum|53785|\nbuttonClicked|chc5\n\n\n";
                                {
                                    std::lock_guard<std::mutex> lock(Network.peer_mutex);
                                    SendPacket(SERVER, Network.NET_MESSAGE_GENERIC_TEXT, sendBGL.c_str(), sendBGL.length());
                                }

                                return false;
                            }
                        }
                        else if (Player.dialogSkipper.SALES_MAN && Packet::Contains(dialog, "add_label_with_icon|big|`wBlue Gem Lock``|left|7188|") && Packet::Contains(dialog, "add_textbox|Wow, that's fast delivery. Excellent!")) {
                            Player.dialogSkipper.SALES_MAN = false;
                            return false;
                        }
                        else if (Player.dialogSkipper.SALES_MAN && Packet::Contains(dialog, "add_label_with_icon|big|`wBlue Gem Lock``|left|7188|")) {
                            int tilex = Packet::ExtractValue<int>(dialog, "tilex", -1), tiley = Packet::ExtractValue<int>(dialog, "tiley", -1);

                            if (tilex != -1 && tiley != -1 && Player.GetItemCount(1796) >= 100 && Player.GetItemCount(7188) < 200) {
                                std::string sendBGL = "action|dialog_return\ndialog_name|phonecall\ntilex|" + std::to_string(tilex) + "|\ntiley|" + std::to_string(tiley) + "|\nnum|-34|\nbuttonClicked|chc0\n\n\n";
                                {
                                    std::lock_guard<std::mutex> lock(Network.peer_mutex);
                                    SendPacket(SERVER, Network.NET_MESSAGE_GENERIC_TEXT, sendBGL.c_str(), sendBGL.length());
                                }

                                return false;
                            }
                        }

                        GamePacket<OnDialogRequest> p;
                        p.text = std::get<std::string>(param[1]);
                        SendGamePacket(CLIENT, p);

                        return false;
                    }
                    else if (std::get<std::string>(param[0]) == "OnTalkBubble" && param.size() >= 3) {
                        /* @debug:
                            [26:02:2026 16:22:02] [INFO ] [NETWORK] [SERVER -> PROXY -> CLIENT] [NET_MESSAGE_GAME_PACKET]
                            [0][STRING] OnTalkBubble
                            [1][UINT] 2
                            [2][STRING] `7[```wGrowtopiaProxy`` spun the wheel and got `20``!`7]``
                            [3][UINT] 0

                            [26:02:2026 16:24:23] [INFO ] [NETWORK] [SERVER -> PROXY -> CLIENT] [NET_MESSAGE_GAME_PACKET]
                            [0][STRING] OnTalkBubble
                            [1][UINT] 4
                            [2][STRING] `7[```2GrowtopiaProxy`` spun the wheel and got `47``!`7]``
                            [3][UINT] 0
                        */

                        std::string textParam = "", param2Text = std::get<std::string>(param[2]);
                        if (Method == "SERVER -> PROXY -> CLIENT" && Packet::Contains(param2Text, "`` spun the wheel and got `") && param2Text.starts_with("`7[```") && Packet::Contains(param2Text, "`` spun the wheel and got `") && param2Text.ends_with("``!`7]``")) textParam += "`4[REAL]`w`w ";
                        textParam += param2Text;

                        GamePacket<OnTalkBubble> p;
                        p.netId = std::get<unsigned int>(param[1]);
                        p.text = textParam;
                        if (param.size() >= 4) p.unknown = std::get<unsigned int>(param[3]);
                        if (param.size() >= 5) p.unknown2 = std::get<unsigned int>(param[4]);
                        SendGamePacket(CLIENT, p);

                        return false;
                    }
                    else if (std::get<std::string>(param[0]) == "OnConsoleMessage" && param.size() == 2) {
                        /* @debug: for collect item from a tile
                            [23:02:2026 14:26:40] [INFO ] [NETWORK] [SERVER -> PROXY -> CLIENT] [NET_MESSAGE_GAME_PACKET]
                            [0][STRING] OnConsoleMessage
                            [1][STRING] Collected `w5 Donation Box``. Rarity: `w65``
                        */
                        auto& packet = std::get<std::string>(param[1]);

                        if (Method == "SERVER -> PROXY -> CLIENT" && Packet::Contains(packet, "Collected `w")) {
                            std::regex re(R"(^Collected `w([1-9]|[1-9][0-9]|1[0-9]{2}|200)\s+(.+?)``.*)");

                            if (auto match = std::smatch{}; std::regex_match(packet, match, re)) {
                                int number = std::stoi(match[1].str());
                                std::string itemName = match[2].str();

                                auto it = std::find_if(items.begin(), items.end(), [&itemName](const Items& i) {
                                    return i.raw_name == itemName;
                                });

                                if (it != items.end()) Player.AddItem(it->id, number);
                                else LOG_ERROR("Item Name: {} couldn't find in the database!", itemName);
                            }
                        }

                        GamePacket<OnConsoleMessage> p;
                        p.text = packet;
                        SendGamePacket(CLIENT, p);
                        return false;
                    }
                    else if (std::get<std::string>(param[0]) == "OnSendToServer" && param.size() >= 6) {
                        /* @debug:
                            [0][STRING] OnSendToServer
                            [1][INT] 17040
                            [2][INT] 4744778
                            [3][INT] 184573909
                            [4][STRING] 213.179.209.175|0|02560A89C0D794A90985ADA5C18D8AA6
                            [5][INT] 1
                            [6][STRING] GrowtopiaProxy
                        */

                        if (SERVER) enet_peer_disconnect_now(SERVER, 0);

                        {
                            uint16_t newServerUDP = static_cast<uint16_t>(std::get<int>(param[1]));
                            std::string newServerIP = Packet::ExtractCustom<std::string>(std::get<std::string>(param[4]), "", 0, "|");

                            if (enet_address_set_host_ip(&Network.GetServerAddress(), newServerIP.c_str()) < 0) {
                                return "Invalid newServerIP IP for OnSendToServer!";
                            }
                            Network.GetServerAddress().port = newServerUDP;
                        }

                        {
                            GamePacket<OnSendToServer> p;
                            if (param.size() >= 2) p.port = Local.UDP;
                            if (param.size() >= 3) p.token = std::get<int>(param[2]);
                            if (param.size() >= 4) p.userId = std::get<int>(param[3]);
                            if (param.size() >= 5) p.ip = Local.IP;
                            if (param.size() >= 5) p.doorID = Packet::ExtractCustom<std::string>(std::get<std::string>(param[4]), "|", 1, "|");
                            if (param.size() >= 5) p.UUIDToken = Packet::ExtractCustom<std::string>(std::get<std::string>(param[4]), "|", 2, "|");
                            if (param.size() >= 6) p.lmode = std::get<int>(param[5]);
                            if (param.size() == 7) p.tankIDName = std::get<std::string>(param[6]);
                            SendGamePacket(CLIENT, p);
                        }

                        return false;
                    }
                    else if (std::get<std::string>(param[0]) == "OnSpawn" && param.size() == 2) {
                        /* @debug:
                        * [SOMEONE ELSE]
                            [05:03:2026 19:29:55] [INFO ] [NETWORK] [SERVER -> PROXY -> CLIENT] [NET_MESSAGE_GAME_PACKET]
                            [0][STRING] OnSpawn
                            [1][STRING] spawn|avatar
                            netID|1
                            userID|151764836
                            eid|151764836|mnwtToZ6CWhfISs3tXlDQSF2pbTpkxM4E9Sw/jquGTQ=|DYMUDSDIWpy14Sgl5jNzKGG==
                            ip|ar7JmdEY5ZMGHWWuofmGGGNn4RtftGkeE9NuzJZ8X4=
                            colrect|0|0|20|30
                            posXY|2336|738
                            name|`2GrowtopiaProxy``
                            titleIcon|{"PlayerWorldID":1,"WrenchCustomization":{"WrenchForegroundCanRotate":false,"WrenchForegroundID":-1,"WrenchIconID":-1}}
                            country|us
                            invis|0
                            mstate|0
                            smstate|0
                            onlineID|


                        * [MAIN CHARACTER]
                            [05:03:2026 19:29:55] [INFO ] [NETWORK] [SERVER -> PROXY -> CLIENT] [NET_MESSAGE_GAME_PACKET]
                            [0][STRING] OnSpawn
                            [1][STRING] spawn|avatar
                            netID|5
                            userID|28467728
                            eid|28467728|AyhnDSGZPSLwtHdHAe4w==|OO6FJZZNhs3KWAH1DSQATw==
                            ip|ar7JmdEYJKGHFSWuofmIUNn4RtVGftGkeE9NuzJZ8X4=
                            colrect|0|0|20|30
                            posXY|2368|736
                            name|`w2GrowtopiaProxy2``
                            titleIcon|{"PlayerWorldID":5,"WrenchCustomization":{"WrenchForegroundCanRotate":false,"WrenchForegroundID":-1,"WrenchIconID":-1}}
                            country|us
                            invis|0
                            mstate|0
                            smstate|0
                            onlineID|
                            type|local
                        */

                        if (Packet::ExtractCustom<std::string>(std::get<std::string>(param[1]), "|", 1, "\n") == "avatar") {
                            if (Packet::Contains(std::get<std::string>(param[1]), "type|local")) {
                                Player.netID = Packet::ExtractCustom<int>(std::get<std::string>(param[1]), "|", 2, "\n");
                                Player.userID = Packet::ExtractCustom<int>(std::get<std::string>(param[1]), "|", 3, "\n");

                                std::string name = Packet::ExtractCustom<std::string>(std::get<std::string>(param[1]), "|", 14, "\n");
                                Player.tankIDName = name.substr(2, name.size() - 4);

                                std::string customPacket = std::get<std::string>(param[1]);
                                
                                if (Player.country.length() == 2 && Packet::Change(customPacket, "country|", 1, "\n", Player.country)) {
                                    {
                                        GamePacket<OnSpawn> p;
                                        p.text = customPacket;
                                        SendGamePacket(CLIENT, p);
                                    }

                                    return false;
                                }
                            }
                            else {
                                int thatNetID = Packet::ExtractCustom<int>(std::get<std::string>(param[1]), "|", 2, "\n");
                                
                                std::string name = Packet::ExtractCustom<std::string>(std::get<std::string>(param[1]), "|", 14, "\n");
                                std::string thatName = name.substr(2, name.size() - 4);

                                Player.worldPlayers[thatNetID] = thatName;
                            }
                        }
                        return true;
                    }
                    else if (std::get<std::string>(param[0]) == "OnRemove" && param.size() == 3) {
                        int thatNetID = Packet::ExtractCustom<int>(std::get<std::string>(param[1]), "|", 1, "\n");
                        Player.worldPlayers.erase(thatNetID);
                        return true;
                    }
                    else if (std::get<std::string>(param[0]) == "OnCountryState" && param.size() == 2) {
                        std::string customPacket = std::get<std::string>(param[1]);

                        if (customPacket.length() >= 2 && Player.country.length() == 2) {
                            customPacket.replace(0, 2, Player.country);

                            {
                                GamePacket<OnCountryState> p;
                                p.text = customPacket;
                                SendGamePacket(CLIENT, p);
                            }

                            std::thread([CLIENT]() {
                                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                                GamePacket<OnSetClothing> p;
                                if (Player.hideClothes) {
                                    p.hair_shirt_pants = { 0.000000, 0.000000, 0.000000 };
                                    p.feet_face_hand = { 0.000000, 0.000000, 0.000000 };
                                    p.back_mask_necklace = { 0.000000, 0.000000, 0.000000 };
                                    p.skinColor = 3033464831;
                                    p.ances_unknown_unknown2 = { 0.000000, 0.000000, 0.000000 };
                                }
                                else {
                                    p.hair_shirt_pants = { Player.clothes.hair_shirt_pants.x, Player.clothes.hair_shirt_pants.y, Player.clothes.hair_shirt_pants.z };
                                    p.feet_face_hand = { Player.clothes.feet_face_hand.x, Player.clothes.feet_face_hand.y, Player.clothes.feet_face_hand.z };
                                    p.back_mask_necklace = { Player.clothes.back_mask_necklace.x, Player.clothes.back_mask_necklace.y, Player.clothes.back_mask_necklace.z };
                                    p.skinColor = Player.clothes.skinColor;
                                    p.ances_unknown_unknown2 = { Player.clothes.ances_unknown_unknown2.x, Player.clothes.ances_unknown_unknown2.y, Player.clothes.ances_unknown_unknown2.z };
                                }
                                SendGamePacket(CLIENT, p, Player.netID);
                            }).detach();

                            return false;
                        }

                        return true;
                    }
                    else if (std::get<std::string>(param[0]) == "OnSetClothing" && param.size() == 6) {
                        /* @debug: 
                            [26:02:2026 20:00:19] [INFO ] [NETWORK] [SERVER -> PROXY -> CLIENT] [NET_MESSAGE_GAME_PACKET]
                            [0][STRING] OnSetClothing
                            [1][FLOAT3] 0.000000, 0.000000, 0.000000
                            [2][FLOAT3] 3074.000000, 0.000000, 12356.000000
                            [3][FLOAT3] 10332.000000, 0.000000, 0.000000
                            [4][UINT] 2190853119
                            [5][FLOAT3] 0.000000, 0.000000, 0.000000
                        */

                        int netID;
                        std::memcpy(&netID, packet->data + 8, sizeof(int));

                        auto hair_shirt_pants = std::get<Vec3>(param[1]);
                        auto feet_face_hand = std::get<Vec3>(param[2]);
                        auto back_mask_necklace = std::get<Vec3>(param[3]);
                        uint32_t skinColor = std::get<unsigned int>(param[4]);
                        auto ances_unknown_unknown2 = std::get<Vec3>(param[5]);

                        if (netID == Player.netID) {
                            Player.clothes.hair_shirt_pants = { hair_shirt_pants.x, hair_shirt_pants.y, hair_shirt_pants.z };
                            Player.clothes.feet_face_hand = { feet_face_hand.x, feet_face_hand.y, feet_face_hand.z };
                            Player.clothes.back_mask_necklace = { back_mask_necklace.x, back_mask_necklace.y, back_mask_necklace.z };
                            Player.clothes.skinColor = skinColor;
                            Player.clothes.ances_unknown_unknown2 = { ances_unknown_unknown2.x, ances_unknown_unknown2.y, ances_unknown_unknown2.z };
                        }

                        GamePacket<OnSetClothing> p;
                        if (Player.hideClothes) {
                            p.hair_shirt_pants = { 0.000000, 0.000000, 0.000000 };
                            p.feet_face_hand = { 0.000000, 0.000000, 0.000000 };
                            p.back_mask_necklace = { 0.000000, 0.000000, 0.000000 };
                            p.skinColor = 3033464831;
                            p.ances_unknown_unknown2 = { 0.000000, 0.000000, 0.000000 };
                        }
                        else {
                            p.hair_shirt_pants = { hair_shirt_pants.x, hair_shirt_pants.y, hair_shirt_pants.z };
                            p.feet_face_hand = { feet_face_hand.x, feet_face_hand.y, feet_face_hand.z };
                            p.back_mask_necklace = { back_mask_necklace.x, back_mask_necklace.y, back_mask_necklace.z };
                            p.skinColor = skinColor;
                            p.ances_unknown_unknown2 = { ances_unknown_unknown2.x, ances_unknown_unknown2.y, ances_unknown_unknown2.z };
                        }
                        SendGamePacket(CLIENT, p, netID);

                        return false;
                    }
                }
                else if (ShouldLogNetMessage(Type, Network.LOG_NET_MESSAGE_GAME_PACKET_BASIC)) LOG_ERROR("param[0] is not std::string, type: {}", param[0].index());
            }
        }
        return true;
    }
private:
    static inline int ReadInt(const uint8_t* data, size_t& cursor) {
        int v;
        std::memcpy(&v, data + cursor, sizeof(int));
        cursor += sizeof(int);
        return v;
    }

    static inline float ReadFloat(const uint8_t* data, size_t& cursor) {
        float v;
        std::memcpy(&v, data + cursor, sizeof(float));
        cursor += sizeof(float);
        return v;
    }

    static inline unsigned int ReadUInt(const uint8_t* data, size_t& cursor) {
        unsigned int v;
        std::memcpy(&v, data + cursor, sizeof(unsigned int));
        cursor += sizeof(unsigned int);
        return v;
    }

    static inline long long ReadInt64(const uint8_t* data, size_t& cursor) {
        long long v;
        std::memcpy(&v, data + cursor, sizeof(long long));
        cursor += sizeof(long long);
        return v;
    }
};