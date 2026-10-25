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
#include <filesystem>

class COMMANDS {
public:
    static inline bool Inject(std::string command, ENetPeer* CLIENT, ENetPeer* SERVER) {
		LOG_DEBUG("Command:[{}]", command);
		
        if (command == "/proxy") {
            GamePacket<OnDialogRequest> p;
			p.text += "add_label_with_icon|big|`w" + Main.NAME + " V" + Main.VERSION + "|left|32|\n";
			p.text += "add_custom_textbox|`4Warning: `oThis proxy has been made by dc:xoidnl and using this proxy is at your own risk!|size:tiny|\n";
			p.text += "add_spacer|small|\n";
			p.text += "add_custom_textbox|`wInfo:|size:medium|\n";
			p.text += "add_custom_textbox|`4/proxy  `o(Shows Proxy Commands)|size:small|\n";
			p.text += "add_custom_textbox|`4/news  `o(Shows Growtopia News)|size:small|\n";
			p.text += "add_custom_textbox|`4/ping  `o(Shows Your Real Ping)|size:small|\n";
			p.text += "add_spacer|small|\n";
			p.text += "add_custom_textbox|`wCustomize:|size:medium|\n";
			p.text += "add_custom_textbox|`4/ui  `o(Opens UI Customizer Menu)|size:small|\n";
			p.text += "add_custom_textbox|`4/country [code]  `o(Changes Your Country Flag)|size:small|\n";
			p.text += "add_spacer|small|\n";
			p.text += "add_custom_textbox|`wMain Features:|size:medium|\n";
			p.text += "add_custom_textbox|`4/farm  `o(Starts/Stops Auto-Farming)|size:small|\n";
			p.text += "add_custom_textbox|`4/spam  `o(Opens Auto-Spam Settings Menu)|size:small|\n";
			p.text += "add_custom_textbox|`4/wrench  `o(Starts/Stops Auto-Wrench (Left = Pull & Right = Kick)|size:small|\n";
			p.text += "add_spacer|small|\n";
			p.text += "add_custom_textbox|`wInformation:|size:medium|\n";
			p.text += "add_custom_textbox|`4/balance  `o(Shows Your Current World Lock Balance)|size:small|\n";
			p.text += "add_custom_textbox|`4/account  `o(Opens a Menu That Displays All Your Account Informations)|size:small|\n";
			p.text += "add_custom_textbox|`4/inventory  `o(Opens a Menu That Displays All The Items In Your Inventory)|size:small|\n";
			p.text += "add_spacer|small|\n";
			p.text += "add_custom_textbox|`wShortcuts:|size:medium|\n";
			p.text += "add_custom_textbox|`4/drop  `o(Enables Fast Drop)|size:small|\n";
			p.text += "add_custom_textbox|`4/trash  `o(Enables Fast Trash/Recycle)|size:small|\n";
			p.text += "add_custom_textbox|`4/pullall  `o(Pulls Everyone In The World)|size:small|\n";
			p.text += "add_custom_textbox|`4/kickall  `o(Kicks Everyone In The World)|size:small|\n";
			p.text += "add_custom_textbox|`4/banall  `o(Bans Everyone In The World)|size:small|\n";
			p.text += "add_custom_textbox|`4/warp [world]  `o(Warps To A World)|size:small|\n";
			p.text += "add_custom_textbox|`4/back  `o(Warps You To A Previously Visited World)|size:small|\n";
			p.text += "add_custom_textbox|`4/setsave [World]  `o(Set Save World)|size:small|\n";
			p.text += "add_custom_textbox|`4/save  `o(Warps You To A Save World)|size:small|\n";
			p.text += "add_custom_textbox|`4/relog  `o(Fast Exit & Join Back To The World)|size:small|\n";
			p.text += "add_custom_textbox|`4/change  `o(Changes DL->BGL When Wrench The Phone)|size:small|\n";
			p.text += "add_spacer|small|\n";
			p.text += "add_custom_textbox|`wDrop Features:|size:medium|\n";
			p.text += "add_custom_textbox|`4/cd [number]  `o(Drops Custom Amount Of Lock)|size:small|\n";
			p.text += "add_custom_textbox|`4/wd [number]  `o(Drops Custom Amount Of WL)|size:small|\n";
			p.text += "add_custom_textbox|`4/dd [number]  `o(Drops Custom Amount Of DL)|size:small|\n";
			p.text += "add_custom_textbox|`4/bd [number]  `o(Drops Custom Amount Of BGL)|size:small|\n";
			p.text += "add_custom_textbox|`4/daw  `o(Drops All Your WLS/DLS/BGLS)|size:small|\n";
			p.text += "add_spacer|small|\n";
			p.text += "add_custom_textbox|`wAnti Lag:|size:medium|\n";
			p.text += "add_custom_textbox|`4/hideclothes  `o(Hides Everyone's Clothes, (Also For Anti Teleport Glitcher)|size:small|\n";
			p.text += "add_spacer|small|\n";
			p.text += "add_quick_exit|\n";
			p.text += "end_dialog|ProxyMenu|||\n";
            SendGamePacket(CLIENT, p);

			return false;
        }

		else if (command == "/ping") {
			int serverPing = SERVER ? SERVER->roundTripTime : 0, clientPing = 0;
			if (Local.IP == "127.0.0.1") clientPing = 0;
			else clientPing = CLIENT ? CLIENT->roundTripTime : 0;

			GamePacket<OnConsoleMessage> p;
			p.text = "Your ping is " + std::to_string(serverPing + clientPing) + " (C" + std::to_string(clientPing) + "+S" + std::to_string(serverPing) + ")";
			SendGamePacket(CLIENT, p);

			return false;
		}

		else if (command == "/ui") {
			GamePacket<OnDialogRequest> p;
			p.text += "add_label_with_icon|big|`wUser Interface|left|32|\n";
			p.text += "add_custom_textbox|`4Explaination: `oYou can modify the growtopia dialog packets woth your custom 'r,g,b,a' color codes!|size:small|\n";
			p.text += "add_spacer|small|\n";
			p.text += "add_text_input|input_red|`4Red `o(0-255)|0|3|\n";
			p.text += "add_text_input|input_green|`rGreen `o(0-255)|0|3|\n";
			p.text += "add_text_input|input_blue|`cBlue `o(0-255)|0|3|\n";
			p.text += "add_text_input|input_alpha|`bAlpha `o(0-255)|160|3|\n";
			p.text += "add_quick_exit|\n";
			p.text += "end_dialog|PROXY_DIALOGS_UI|Cancel|Apply|\n";
			SendGamePacket(CLIENT, p);

			return false;
		}

		else if (Packet::Contains(command, "/country ")) {
			std::string text = "", code = Packet::ExtractCustom<std::string>(command, " ", 1, "\n");
			if (code.length() == 2) {
				text = "Changed your country flag with [" + code + "]";
				Player.country = code;
			}
			else text = "Wrong country code, example: 'uk'";

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
		else if (command == "/farm") {
			std::string text;

			if (Player.autofarm.running) {
				text = "Stopped auto-farming!";

				Player.autofarm.running = false;
				Player.autofarm.id = -1;
				Player.autofarm.X = -1.0f;
				Player.autofarm.Y = -1.0f;
				Player.autofarm.punchX = -1;
				Player.autofarm.punchY = -1;
			}
			else {
				text = "Activated auto-farming! (`4place a block to start``)";

				Player.autofarm.running = true;
				Player.autofarm.worker = std::thread(&PlayerState::AutoFarm, &Player, CLIENT, SERVER);
				Player.autofarm.worker.detach();
			}

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

			return false;
		}

		else if (command == "/spam") {
			GamePacket<OnDialogRequest> p;
			p.text += "add_label_with_icon|big|`wSpam Menu|left|32|\n";
			p.text += "add_custom_textbox|`4Explaination: `oSet the message what you want to spam and click to 'Apply' button. Leave empty the second text if you want to use only 1. Rainbow mode is for random color!|size:small|\n";
			p.text += "add_spacer|small|\n";
			p.text += "add_text_input|text1|`wFirst Text: |" + Player.autospam.text1 + "|64|\n";
			p.text += "add_text_input|text2|`wSecond Text: |" + Player.autospam.text2 + "|64|\n";
			p.text += "add_text_input|delay|`wDelay(ms): |" + std::to_string(Player.autospam.delay) + "|64|\n";
			p.text += "add_checkbox|rainbow|`wRainbow Mode|" + std::string(Player.autospam.rainbow == true ? "1" : "0") + "\n";
			p.text += "add_spacer|small|\n";
			p.text += "add_button|" + std::string(Player.autospam.running ? "stop" : "start") + "|" + std::string(Player.autospam.running ? "`4STOP" : "`2START") + "||\n";
			p.text += "add_quick_exit|\n";
			p.text += "end_dialog|PROXY_DIALOGS_SPAM|||\n";
			SendGamePacket(CLIENT, p);

			return false;
		}

		else if (command == "/wrench") {
			std::string text;

			if (Player.wrenchMode) text = "Wrench mode deactivated!";
			else text = "Wrench mode activated, Left click for pull and right click for Kick!";

			Player.wrenchMode = !Player.wrenchMode;

			{
				GamePacket<OnConsoleMessage> p;
				p.text = text;
				SendGamePacket(CLIENT, p);
			}

			{
				GamePacket<OnTalkBubble> p;
				p.netId = Player.netID;
				p.text = "`o[`4" + Main.NAME + "`o]`w`w`w Changed the wrench mode!";
				SendGamePacket(CLIENT, p);
			}

			return false;
		}

		else if (command == "/balance") {
			struct Item { int id; const char* name; };

			Item items[] = {
				{7188, "Blue Gem Lock"},
				{1796, "Diamond Lock"},
				{242,  "World Lock"}
			};

			std::string text;

			for (auto& item : items) {
				int count = Player.GetItemCount(item.id);
				if (count > 0) text += std::to_string(count) + "x " + item.name + ". ";
			}

			if (text.empty()) text = "You don't have any locks.";

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

		else if (command == "/account") {
			GamePacket<OnDialogRequest> p;
			p.text += "add_label_with_icon|big|`wAccount Menu|left|32|\n";
			p.text += "add_custom_textbox|`4Explaination: `oThis dialog shows all of your account informations!|size:small|\n";
			p.text += "add_spacer|small|\n";
			p.text += "add_custom_textbox|`oName: `$" + Player.tankIDName + "|size:small|\n";
			p.text += "add_custom_textbox|`oAge: `$" + std::to_string(Player.player_age) + "|size:small|\n";
			p.text += "add_custom_textbox|`oKLV Address: `$" + Player.klv + "|size:small|\n";
			p.text += "add_custom_textbox|`oMETA Address: `$" + Player.meta + "|size:small|\n";
			p.text += "add_custom_textbox|`oRID Address: `$" + Player.rid + "|size:small|\n";
			p.text += "add_custom_textbox|`oMAC Address: `$" + Player.mac + "|size:small|\n";
			p.text += "add_custom_textbox|`oWK Address: `$" + Player.wk + "|size:small|\n";
			p.text += "add_quick_exit|\n";
			p.text += "end_dialog|PROXY_DIALOGS_ACCOUNT|||\n";
			SendGamePacket(CLIENT, p);

			return false;
		}

		else if (command == "/inventory") {
			GamePacket<OnDialogRequest> p;
			p.text += "add_label_with_icon|big|`wInventory Menu|left|32|\n";
			p.text += "add_custom_textbox|`4Explaination: `oThis dialog shows all of your inventory!|size:small|\n";
			p.text += "add_spacer|small|\n";
			

			p.text += Player.InventoryToString();

			p.text += "add_quick_exit|\n";
			p.text += "end_dialog|PROXY_DIALOGS_ACCOUNT|||\n";
			SendGamePacket(CLIENT, p);

			return false;
		}

		else if (command == "/drop") {
			std::string text;
			if (Player.dropMode) text = "Drop mode is deactivated!";
			else text = "Drop mode is activated!";

			Player.dropMode = !Player.dropMode;
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

		else if (command == "/trash") {
			std::string text;
			if (Player.trashMode) text = "Trash mode is deactivated!";
			else text = "Trash mode is activated!";

			Player.trashMode = !Player.trashMode;

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

		else if (command == "/pullall" || command == "/kickall" || command == "/banall") {
			std::string action = "pull";
			if (command == "/kickall") action = "kick";
			else if (command == "/banall") action = "ban";

			std::string text = "Everyone in the world is auto " + action + "ing!";
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

			std::thread([action, SERVER]() {
				for (const auto& pair : Player.worldPlayers) {
					std::string packetNew = "action|input\n|text|/" + action + " " + pair.second + "\n";
					{
						std::lock_guard<std::mutex> lock(Network.peer_mutex);
						SendPacket(SERVER, Network.NET_MESSAGE_GENERIC_TEXT, packetNew.c_str(), static_cast<int>(packetNew.length()));
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
				}
			}).detach();

			return false;
		}

		else if (Packet::Contains(command, "/warp ")) {
			std::string newWorld = Packet::ExtractCustom<std::string>(command, " ", 1, "\n");
			if (newWorld.empty()) return true;
			Player.Warp(newWorld, CLIENT, SERVER);

			return false;
		}

		else if (Packet::Contains(command, "/setsave ")) {
			std::string newSave = Packet::ExtractCustom<std::string>(command, " ", 1, "\n");
			if (newSave.empty()) return true;
			Player.saveWorld = newSave;

			std::string text = "Changed the saving world with [" + newSave + "]";

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

		else if (command == "/save") {
			if (Player.saveWorld.empty()) {
				std::string text = "Couldn't find the save world, use '/setsave world] command!";

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
			else {
				GamePacket<OnConsoleMessage> p;
				p.text = "Warping to the save world!";
				SendGamePacket(CLIENT, p);
				Player.Warp(Player.saveWorld, CLIENT, SERVER);
			}

			return false;
		}

        else if (command == "/back") {
			if (Player.lastWorld.empty()) {
				std::string text = "Couldn't find the last world!";

				{
					GamePacket<OnConsoleMessage> p;
					p.text = text;
					SendGamePacket(CLIENT, p);
				}

				{
					GamePacket<OnTalkBubble> p;
					p.netId = Player.netID;
					p.text = "`o[`4" + Main.NAME + "`o]`w`w`w " + "Changed the wrench mode!";
					SendGamePacket(CLIENT, p);
				}
			}
			else {
				std::string newWorld = Player.lastWorld;
				Player.Warp(newWorld, CLIENT, SERVER);
			}

			return false;
		}

        else if (command == "/relog") {
			std::string newWorld = Player.world;
			Player.Warp(newWorld, CLIENT, SERVER);

			return false;
		}

		else if (command == "/change") {
			std::string text;
			if (Player.autoChange) text = "Change mode is deactivated!";
			else text = "Change mode is activated!";

			Player.autoChange = !Player.autoChange;

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

		else if (Packet::Contains(command, "/cd ")) {
			int NEEDS = Packet::ExtractCustom<int>(command, " ", 1, "\n");
			if (NEEDS < 1) return false;
			if (NEEDS > 2020200) NEEDS = 2020200;

			int myBGl = Player.HasItem(7188) ? Player.GetItemCount(7188) : 0;
			int myDL = Player.HasItem(1796) ? Player.GetItemCount(1796) : 0;
			int myWL = Player.HasItem(242) ? Player.GetItemCount(242) : 0;

			int myTOTAL = myWL + (myDL * 100) + (myBGl * 10000);

			if (myTOTAL < NEEDS) {
				GamePacket<OnConsoleMessage> p;
				p.text = "Not enough locks!";
				SendGamePacket(CLIENT, p);
				return false;
			}

			std::thread([NEEDS, myBGl, myDL, myWL, CLIENT, SERVER]() mutable {
				int dropBGL = NEEDS / 10000;
				if (dropBGL > myBGl) dropBGL = myBGl;

				int remain = NEEDS - (dropBGL * 10000);

				int dropDL = remain / 100;
				int dropWL = remain % 100;

				while (myWL >= 100 && myDL < dropDL && myDL < 200) {
					LOG_DEBUG("Converting WL -> DL");

					PlayerMovingPacket convertPacket;
					convertPacket
						.PacketType(Network.PACKET_ITEM_ACTIVATE_REQUEST)
						.PlantingTree(242)
						.Send(SERVER);

					std::this_thread::sleep_for(std::chrono::milliseconds(225));

					myWL -= 100;
					myDL += 1;
				}

				if (dropBGL > 0) {
					LOG_DEBUG("Dropping {} BGL", dropBGL);
					Player.Drop(7188, dropBGL, CLIENT, SERVER);
					myBGl -= dropBGL;
					std::this_thread::sleep_for(std::chrono::milliseconds(120));
				}

				while ((myDL < dropDL || dropWL > myWL) && myBGl > 0 && myDL <= 200) {
					LOG_DEBUG("Converting BGL -> DL");

					PlayerMovingPacket convertPacket;
					convertPacket
						.PacketType(Network.PACKET_ITEM_ACTIVATE_REQUEST)
						.PlantingTree(7188)
						.Send(SERVER);

					std::this_thread::sleep_for(std::chrono::milliseconds(225));

					myBGl -= 1;
					myDL += 100;
				}

				if (dropDL > 0) {
					int sendDL = dropDL;
					if (sendDL > myDL) sendDL = myDL;
					LOG_DEBUG("Dropping {} DL", sendDL);
					Player.Drop(1796, sendDL, CLIENT, SERVER);
					myDL -= sendDL;
					std::this_thread::sleep_for(std::chrono::milliseconds(120));
				}

				while (myWL < dropWL && myDL > 0 && myWL <= 100) {
					LOG_DEBUG("Converting DL -> WL");

					PlayerMovingPacket convertPacket;
					convertPacket
						.PacketType(Network.PACKET_ITEM_ACTIVATE_REQUEST)
						.PlantingTree(1796)
						.Send(SERVER);

					std::this_thread::sleep_for(std::chrono::milliseconds(225));

					myDL -= 1;
					myWL += 100;
				}

				if (dropWL > 0) {
					int sendWL = dropWL;
					if (sendWL > myWL) sendWL = myWL;
					LOG_DEBUG("Dropping {} WL", sendWL);
					Player.Drop(242, sendWL, CLIENT, SERVER);
					myWL -= sendWL;
				}
			}).detach();

			return false;
		}

        else if (Packet::Contains(command, "/wd ") || Packet::Contains(command, "/dd ") || Packet::Contains(command, "/bd ")) {
            int item_id = 242;
            if (Packet::Contains(command, "/dd ")) item_id = 1796;
            else if (Packet::Contains(command, "/bd ")) item_id = 7188;

            int item_count = Packet::ExtractCustom<int>(command, " ", 1, "\n");
            if (item_count < 1) return false;
            if (item_count > 200) item_count = 200;

            Player.Drop(item_id, item_count, CLIENT, SERVER);

            return false;
        }

        else if (command == "/daw") {
            std::thread([CLIENT, SERVER]() {
                if (Player.HasItem(7188)) {
                    Player.Drop(7188, Player.GetItemCount(7188), CLIENT, SERVER);
                    std::this_thread::sleep_for(std::chrono::milliseconds(120));
                }

                if (Player.HasItem(1796)) {
                    Player.Drop(1796, Player.GetItemCount(1796), CLIENT, SERVER);
                    std::this_thread::sleep_for(std::chrono::milliseconds(120));
                }

                if (Player.HasItem(242)) {
                    Player.Drop(242, Player.GetItemCount(242), CLIENT, SERVER);
                }
            }).detach();

            return false;
        }

        else if (command == "/hideclothes") {
			std::string text;
			if (Player.hideClothes) text = "Hide Clothes mode is deactivated!";
			else text = "Hide Clothes mode is activated!";

			Player.hideClothes = !Player.hideClothes;

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

        return true;
    }
};