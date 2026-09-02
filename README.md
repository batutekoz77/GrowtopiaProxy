# 📋 Growtopia Proxy
Growtopia Bridge Proxy is a powerful tool that sits between the Growtopia client and server, allowing full control over network traffic. With this proxy, you can monitor, modify, and inject packets to customize your gameplay experience.

---

## 🛠 Features  
- **No Shadow Ban At All!**
- **No Account Ban At All (LEGACY PROXY)!** 

- **Bidirectional traffic handling:**
Client -> Proxy -> Server
and
Server -> Proxy -> Client

- **Packet logging for monitoring and debugging** 
- **Packet modification to alter game behavior in real-time** 
- **Custom packet injection to send your own crafted packets** 
---

## 🚀 Tech Stack  

- **Language:** ISO C++23 Standard (Visual Studio 2026 Community)    
- **Compiler Standards:**  
  - C++23 (`/std:c++23preview`)  
  - C17 (`/std:c17`)  
  - Target: `x64/Release`

---

## 🎮 Roadmap  
- [x] Change Settings from Config.hpp file
- [x] Fetching the real Growtopia server credentials
- [x] In-Built HTTP Server
- [x] In-Built Network Server and Client:
    - [x] Debugging all the packets
    - [x] Modifying the real-time packets
    - [x] Creating and sending custom packets
- [x] Lua Script Support
- [x] Custom Socks5 Support (optional, see [Docs/socks5.md](Docs/socks5.md))
- [x] Advanced log system

---

## Info:
- [x] Slash command  (/proxy) : Shows the proxy feautures.
- [x] Slash command  (/news) : Shows the real Growtopia news.
- [x] Slash command  (/ping) : Shows your current Client and Server pings.

## Customize:
- [x] Slash command  (/ui) : Opens UI customizer menu.
- [x] Slash command  (/country code) : Changes your country flag.

## Main Features:
- [x] Slash command  (/lua) : Opens lua script settings menu.
- [x] Slash command  (/stoplua) : Stops the lua script.
- [x] Slash command  (/farm) : Starts auto farming (legacy, anti-ban).
- [x] Slash command  (/spam) : Opens Spam Settings menu.
- [x] Slash command  (/wrench) : Activates/Deactivates the auto wrench (left click = pull, right click = kick).

## Information:
- [x] Slash command  (/balance) : Shows your current lock balance.
- [x] Slash command  (/account) : Opens Account Informations menu.
- [x] Slash command  (/inventory) : Opens Inventory menu.

## Shortcuts:
- [x] Slash command  (/drop) : Activates/Deactivates the Fast-Drop.
- [x] Slash command  (/trash) : Activates/Deactivates the Fast-Trash.
- [x] Slash command  (/pullall) : Pulls everyone in the world.
- [x] Slash command  (/kickall) : Kicks everyone in the world.
- [x] Slash command  (/banall) : Bans everyone in the world.
- [x] Slash command  (/warp name) : Warps to the chozen world.
- [x] Slash command  (/back) : Warps to the last world.
- [x] Slash command  (/setsave name) : Set the save world.
- [x] Slash command  (/save) : Warps to the save world.
- [x] Slash command  (/relog) : Leaves and joins to the same world.
- [x] Slash command  (/change) : Activates/Deactivates the Fast BGL Change.

## Drop Feautures:
- [x] Slash command  (/cd amount) : Drops the amount of locks.
- [x] Slash command  (/wd amount) : Drops the amount of World Locks.
- [x] Slash command  (/dd amount) : Drops the amount of Diamond Locks.
- [x] Slash command  (/bd amount) : Drops the amount of Blue Gem Locks.
- [x] Slash command  (/daw) : Drops your all locks.

## Anti Lag:
- [x] Slash command  (/hideclothes) : Hides the clothes of everyone and removes the effects.

## Extra
- [x] Fast system :       Removes delay in GamePackets.
- [x] Roulette Verifier : Shows if the roulette shows the real log.
- [x] Custom Design :     Changes all of the dialog colors of the game.

--- 

## 📸 Screenshots

![Info & Customize & Main Feautures](Docs/Proxy1.png)
![Information & Shortcuts](Docs/Proxy2.png)
![Drop Feautures & Anti Lag](Docs/Proxy3.png)

![LUA MENU](Docs/LuaMenu.png)
![UI MENU](Docs/UiMenu.png)
![SPAM MENU](Docs/SpamMenu.png)

---

## Getting Started

### 1. Install Git
Download and install Git:  
https://github.com/git-for-windows/git/releases/download/v2.50.1.windows.1/Git-2.50.1-64-bit.exe  

### 2. Setup VCPKG & Dependencies
Open **CMD** and run:
```
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.bootstrap-vcpkg.bat
.vcpkg install openssl:x64-windows
.vcpkg install lua:x64-windows
.vcpkg install sol2:x64-windows
.vcpkg integrate install
```

### 3. Configure in Visual Studio
- Go to Project → Properties  
- Select Configuration Properties → vcpkg  
- Set Use Vcpkg to "Yes"  

## Building the Game
1. Open the project in **Visual Studio Community**
2. Edit the (`Main/Config.hpp`) file
3. Build the solution (`Ctrl + Shift + B`)  
4. Run the `Source.exe` from (`x64/Release`)

Note: If you want to use Debug, use 'Local Windows Debugger' while 'x64/Release' is selected.
Note: It will ask permission to restart the visual studio with administrator mode, allow it!

---

## 🌙 Lua Scripting

Scripts are located in `x64/Release/Scripts/`. Use `/lua` in-game to open the script hub.

> ⚠️ **Important:** Always wrap loops with `isRunning()` so `/stop` works correctly.

---

### 📁 Script Structure

```lua
-- runs once at the start
print("Script started!")

-- main loop
while isRunning() do
    -- your code here
    sleep(1000)
end

print("Script stopped.")
```

---

### 🔧 Core Functions

| Function | Description |
|---|---|
| `print(...)` | Logs a message to the proxy console |
| `sleep(ms)` | Pauses the script (supports `/stop` mid-sleep) |
| `isRunning()` | Returns `true` if the script is still running |

---

### 👤 Player Info

| Function | Returns | Description |
|---|---|---|
| `getUserID()` | `int` | Player's user ID |
| `getTankIDName()` | `string` | Player's tank ID name |
| `getPlayerAge()` | `int` | Player's age |
| `getNetID()` | `int` | Player's net ID |
| `getCountry()` | `string` | Player's country code |
| `getWorld()` | `string` | Current world name |
| `getLastWorld()` | `string` | Last visited world |
| `getSaveWorld()` | `string` | Saved world name |
| `getPosition()` | `float, float` | Player's X and Y position |
| `getKlv()` | `string` | KLV token |
| `getMeta()` | `string` | Meta token |
| `getRid()` | `string` | RID token |
| `getMac()` | `string` | MAC address |
| `getWk()` | `string` | WK token |

---

### 🎒 Inventory

| Function | Returns | Description |
|---|---|---|
| `hasItem(id)` | `bool` | Returns `true` if the item exists in inventory |
| `getItemCount(id)` | `int` | Returns the count of the item |

```lua
if hasItem(242) then
    print("Dirt count: " .. getItemCount(242))
end
```

---

### 🌍 Player Actions

| Function | Description |
|---|---|
| `warp(worldName)` | Warps to the given world |
| `drop(id, count)` | Drops the specified item |
| `trash(id, count)` | Trashes the specified item |

```lua
warp("START")
drop(242, 10)
trash(242, 5)
```

---

### 📦 Variant Packets (Client-side)

| Function | Description |
|---|---|
| `OnConsoleMessage(text)` | Sends a console message to the client |
| `OnTalkBubble(text)` | Shows a talk bubble above the player |
| `OnTextOverlay(text)` | Shows an overlay text on screen |
| `OnDialogRequest(text)` | Opens a dialog box |
| `OnSetFreezeState(seconds)` | Freezes the player |

```lua
OnConsoleMessage("Hello from Lua!")
OnTalkBubble(getNetID(), "Hey!")
OnTextOverlay("Overlay text")
OnSetFreezeState(3)
```

---

### 📡 Net Message Packets

```lua
-- sendPacket(target, type, text)
-- target: "server" or "client"
-- type:   see below

sendPacket("server", 2, "action|join_request\nname|START\ninvitedWorld|0\n")
sendPacket("client", 2, "action|log\nmsg|`oHello!\n")
```

---

### 📋 Net Message Type Constants

| Constant | Value |
|---|---|
| `NET_MESSAGE_UNKNOWN` | 0 |
| `NET_MESSAGE_SERVER_HELLO` | 1 |
| `NET_MESSAGE_GENERIC_TEXT` | 2 |
| `NET_MESSAGE_GAME_MESSAGE` | 3 |
| `NET_MESSAGE_GAME_PACKET` | 4 |
| `NET_MESSAGE_ERROR` | 5 |
| `NET_MESSAGE_TRACK` | 6 |
| `NET_MESSAGE_CLIENT_LOG_REQUEST` | 7 |
| `NET_MESSAGE_CLIENT_LOG_RESPONSE` | 8 |

---

### 🕹️ PlayerMoving Packets

```lua
-- sendPlayerMoving(table)
sendPlayerMoving({
    packetType   = 10,        -- PACKET_ITEM_ACTIVATE_REQUEST
    plantingTree = 242,       -- item ID
    target       = "server"   -- "server" or "client"
})

-- punch a tile
sendPlayerMoving({
    packetType   = 3,         -- PACKET_TILE_CHANGE_REQUEST
    plantingTree = 32,
    punch        = {18, 8},   -- {punchX, punchY}
    position     = {576.0, 256.0},
    target       = "server"
})
```

#### Available fields for `sendPlayerMoving`:

| Field | Type | Description |
|---|---|---|
| `packetType` | `int` | Packet type (see PACKET_* constants) |
| `netID` | `int` | Net ID |
| `characterState` | `int` | Character state flags |
| `plantingTree` | `int` | Item ID to use/place |
| `punch` | `{int, int}` | Punch tile coordinates `{x, y}` |
| `position` | `{float, float}` | Player position `{x, y}` |
| `speed` | `{float, float}` | Player speed `{xs, ys}` |
| `target` | `string` | `"server"` (default) or `"client"` |

---

### 📋 Packet Type Constants

| Constant | Value |
|---|---|
| `PACKET_STATE` | 0 |
| `PACKET_CALL_FUNCTION` | 1 |
| `PACKET_UPDATE_STATUS` | 2 |
| `PACKET_TILE_CHANGE_REQUEST` | 3 |
| `PACKET_SEND_MAP_DATA` | 4 |
| `PACKET_SEND_TILE_UPDATE_DATA` | 5 |
| `PACKET_SEND_TILE_UPDATE_DATA_MULTIPLE` | 6 |
| `PACKET_TILE_ACTIVATE_REQUEST` | 7 |
| `PACKET_TILE_APPLY_DAMAGE` | 8 |
| `PACKET_SEND_INVENTORY_STATE` | 9 |
| `PACKET_ITEM_ACTIVATE_REQUEST` | 10 |
| `PACKET_ITEM_ACTIVATE_OBJECT_REQUEST` | 11 |
| `PACKET_SEND_TILE_TREE_STATE` | 12 |
| `PACKET_MODIFY_ITEM_INVENTORY` | 13 |
| `PACKET_ITEM_CHANGE_OBJECT` | 14 |
| `PACKET_SEND_LOCK` | 15 |
| `PACKET_SEND_ITEM_DATABASE_DATA` | 16 |
| `PACKET_SEND_PARTICLE_EFFECT` | 17 |
| `PACKET_SET_ICON_STATE` | 18 |
| `PACKET_ITEM_EFFECT` | 19 |
| `PACKET_SET_CHARACTER_STATE` | 20 |
| `PACKET_PING_REPLY` | 21 |
| `PACKET_PING_REQUEST` | 22 |
| `PACKET_GOT_PUNCHED` | 23 |
| `PACKET_APP_CHECK_RESPONSE` | 24 |
| `PACKET_APP_INTEGRITY_FAIL` | 25 |
| `PACKET_DISCONNECT` | 26 |
| `PACKET_BATTLE_JOIN` | 27 |
| `PACKET_BATTLE_EVENT` | 28 |
| `PACKET_USE_DOOR` | 29 |
| `PACKET_SEND_PARENTAL` | 30 |
| `PACKET_GONE_FISHIN` | 31 |
| `PACKET_STEAM` | 32 |
| `PACKET_PET_BATTLE` | 33 |
| `PACKET_NPC` | 34 |
| `PACKET_SPECIAL` | 35 |
| `PACKET_SEND_PARTICLE_EFFECT_V2` | 36 |
| `PACKET_ACTIVE_ARROW_TO_ITEM` | 37 |
| `PACKET_SELECT_TILE_INDEX` | 38 |
| `PACKET_SEND_PLAYER_TRIBUTE_DATA` | 39 |
| `PACKET_FTUE_SET_ITEM_TO_QUICK_INVENTORY` | 40 |
| `PACKET_PVE_NPC` | 41 |
| `PACKET_PVP_CARD_BATTLE` | 42 |
| `PACKET_PVE_APPLY_PLAYER_DAMAGE` | 43 |
| `PACKET_PVE_NPC_POSITION_DAMAGE` | 44 |
| `PACKET_SET_EXTRA_MODS` | 45 |
| `PACKET_ON_STEP_ON_TILE_MOD` | 46 |

---

### 📝 Example Scripts

#### Auto TalkBubble (10 times, 3s interval)
```lua
local i = 1
while isRunning() and i <= 10 do
    OnTalkBubble(getNetID(), "Message " .. i)
    print("Sent bubble: " .. i)
    sleep(3000)
    i = i + 1
end
```

#### Drop item if available
```lua
local itemID = 242
local count  = 3

if hasItem(itemID) then
    local current = getItemCount(itemID)
    print("Found item " .. itemID .. ", count: " .. current)
    if current >= count then
        drop(itemID, count)
        print("Dropped " .. count .. "x item " .. itemID)
    else
        print("Not enough! Have: " .. current)
    end
else
    print("Item " .. itemID .. " not in inventory")
end
```

#### World hopper
```lua
local worlds = {"START", "PARKOUR", "BUYITEM"}
local i = 1

while isRunning() do
    warp(worlds[i])
    print("Warped to: " .. worlds[i])
    sleep(5000)
    i = i % #worlds + 1
end
```

---

### ⚠️ Common Mistakes

#### 1. Forgetting `isRunning()` in loops
```lua
-- ❌ Wrong - /stop won't work
while true do
    OnTalkBubble(getNetID(), "Hi!")
    sleep(1000)
end

-- ✅ Correct
while isRunning() do
    OnTalkBubble(getNetID(), "Hi!")
    sleep(1000)
end
```

#### 2. Forgetting `isRunning()` in for loops
```lua
-- ❌ Wrong - /stop won't work
for i = 1, 100 do
    drop(242, 1)
    sleep(500)
end

-- ✅ Correct
local i = 1
while isRunning() and i <= 100 do
    drop(242, 1)
    sleep(500)
    i = i + 1
end
```

#### 3. Not checking inventory before drop/trash
```lua
-- ❌ Wrong - will error if item doesn't exist
drop(242, 10)

-- ✅ Correct
if hasItem(242) and getItemCount(242) >= 10 then
    drop(242, 10)
end
```

#### 4. Using `sleep(0)` or no sleep in a while loop
```lua
-- ❌ Wrong - freezes the script thread instantly, burns CPU
while isRunning() do
    OnConsoleMessage("Spam!")
end

-- ✅ Correct - always sleep inside loops
while isRunning() do
    OnConsoleMessage("Hello!")
    sleep(1000)
end
```

#### 5. Running another script while one is already active
```
-- ❌ /execute script2.lua while script1.lua is running
-- The proxy will ignore it and log a warning.
-- Use /stop first, then /execute the new script.
```

---

## 🧦 Optional: SOCKS5 routing

Choose route `[1]` at startup and the proxy sends its own traffic -- the
server_data.php fetch and the ENet game session -- through a SOCKS5 server.
Nothing extra to install: no second executable, no tunnel, no child process
left behind when you close the window.

Copy `socks5.cfg.example` next to `Source.exe`, rename it to `socks5.cfg`,
and fill it in. The server needs username/password auth and UDP ASSOCIATE.

The login page is **not** routed -- the client opens that itself -- so the
login and the game session arrive from two different addresses. Read
[Docs/socks5.md](Docs/socks5.md) before turning it on.

---

## License
This project is licensed under the **MIT License** – see the LICENSE file for details.  

## Contributing
Pull requests are welcome! Feel free to open issues for bugs, feature requests, or ideas.  

## Credits
- C++               – Project language  
- ENet Networking   – Low-latency networking library  
- Http Extension    - A Library that allows you to host HTTP/HTTPS servers and modify the packets.
