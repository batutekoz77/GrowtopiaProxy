#pragma once

#include "../Network.hpp"
#include "../../Main/Config.hpp"
#include "../../Packets/Packets.hpp"
#include "../../Logger/Logger.hpp"
#include "../../Functions/Functions.hpp"
#include "../../Player/Player.hpp"

#include <variant>
#include <vector>
#include <cstring>

class PROXY_DIALOGS {
public:
	static inline bool Inject(std::string packet, ENetPeer* CLIENT, ENetPeer* SERVER) {
        if (Packet::Contains(packet, "dialog_name|PROXY_DIALOGS_UI") &&
            Packet::ContainsValue<int>(packet, "input_red") && Packet::ContainsValue<int>(packet, "input_green") &&
            Packet::ContainsValue<int>(packet, "input_blue") && Packet::ContainsValue<int>(packet, "input_alpha") ) {

            Player.design.OnDialogRequest_RED = Packet::ExtractValue<int>(packet, "input_red", 0);
            Player.design.OnDialogRequest_GREEN = Packet::ExtractValue<int>(packet, "input_green", 0);
            Player.design.OnDialogRequest_BLUE = Packet::ExtractValue<int>(packet, "input_blue", 0);
            Player.design.OnDialogRequest_ALPHA = Packet::ExtractValue<int>(packet, "input_alpha", 160);

            std::string text = "Changed the dialog colors!";

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

            return false;
        }
        else if (Packet::Contains(packet, "dialog_name|PROXY_DIALOGS_SPAM") && Packet::Contains(packet, "text1") && 
            Packet::Contains(packet, "text2") && Packet::Contains(packet, "delay") && Packet::Contains(packet, "rainbow")) {

            if (Packet::Contains(packet, "buttonClicked|start")) {
                if (Packet::ContainsValue<std::string>(packet, "text1")) Player.autospam.text1 = Packet::ExtractValue<std::string>(packet, "text1", "Hello World!");
                if (Packet::ContainsValue<std::string>(packet, "text2")) Player.autospam.text2 = Packet::ExtractValue<std::string>(packet, "text2", "");
                if (Packet::ContainsValue<int>(packet, "delay")) Player.autospam.delay = Packet::ExtractValue<int>(packet, "delay", 3600);
                if (Packet::ContainsValue<int>(packet, "rainbow")) Player.autospam.rainbow = Packet::ExtractValue<int>(packet, "rainbow", 0);
                
                if (Player.autospam.delay < 100) Player.autospam.delay = 100;

                if (!Player.autospam.text1.empty()) {
                    std::string text = "Starting autospam!";

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

                    Player.autospam.running = true;
                    Player.autospam.worker = std::thread(&PlayerState::AutoSpam, &Player, CLIENT, SERVER);
                    Player.autospam.worker.detach();
                }
                else {
                    std::string text = "Text1 can't be empty!";

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

                    Player.autospam.running = false;
                    Player.autospam.text1 = "Hello World!";
                    Player.autospam.text2.clear();
                    Player.autospam.delay = 3600;
                    Player.autospam.rainbow = false;
                }
                return false;
            }
            else if (Packet::Contains(packet, "buttonClicked|stop")) {
                Player.autospam.running = false;
                Player.autospam.text1 = "Hello World!";
                Player.autospam.text2.clear();
                Player.autospam.delay = 3600;
                Player.autospam.rainbow = false;

                std::string text = "Stopped autospam!";

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
            }
            else return true;
        }
		return true;
	}
};