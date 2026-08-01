# csgo_gc (prime-toggle fork)

> [!WARNING]
> This project is incomplete and not ready for general use.

## What is this?
In Valve games, the Game Coordinator (GC) is a backend service most notably responsible for matchmaking and inventory management (like loadouts and skins). This project redirects the GC traffic to a custom, in-process implementation.

## Why would you want this?
While it's still possible to connect CS:GO to CS2's GC by spoofing the version number, this may break in the future if Valve updates the GC protocol. This project aims to restore most GC-related functionality without relying on a centralized server.

## Current features
- Editable inventory (inventory.txt)
- Item equipping
- Opening cases (including sticker capsules, patch packs, graffiti boxes and music kit boxes)
- Graffiti support
- Weapon StatTrak support
- Stickers and patches
- Name tags
- StatTrak swaps
- Storage Units (caskets)
- Majors & operations passes (passes.txt)
- Prime toggle (`has_prime` in config.txt)
- Recent teammates & Party search (configurable via `friends` block in config.txt)
- Competitive cooldown emulation (`competitive_cooldown_seconds` in config.txt) (buggy!!!!!!!!!!!)
- Overwatch case support (fetches cases from remote JSON)
- In-game store
- Works without full Steam API emulation
- Full Windows support
- Functional lobbies
- Dedicated server support
- Functional server browser (only shows csgo_gc servers)
- Networking using Steam's P2P interface

## Planned features
- Rest of the core features (trade ups, souvenirs...)
- Graphical inventory editor
- A tool to copy your CS2 inventory over

## Not planned
- Matchmaking (can't be implemented without a centralized server)

## Installation
- Download [CS:GO from Steam](steam://install/4465480)
- Download the latest release for your platform from the [releases page](https://github.com/sasha190409/shashlik_csgo-gc_primetoggle/releases/latest) (or the prime-toggle fork releases)
- Navigate to the game's installation directory
- Back up your existing launcher executables as they'll be overwritten (i.e. csgo.exe, srcds.exe)
- Extract the contents of the downloaded archive to your game directory, replace the executables when prompted
- Launch the game. If you get the annoying VAC message box, launch the game with the -steam argument

## Inventory editing
Use [this](https://github.com/dricotec/csgo_gc_inventory-editor/releases/latest)

## Building
Requirements:
- Git
- CMake 3.20 or newer
- C++ compiler with C++17 support (VS 2017 or later, Clang 5 or later, GCC 7 or later)

The game is 32-bit on Windows so you need to build as 32-bit:

`cmake -A Win32 -B build`

## License
This project is licensed under the 2-Clause BSD License. See [LICENSE.md](LICENSE.md) for details.

## Credits
* **Mikko Kokko** - Author
* **Theeto** - Code reused from the predecessor project, unusual loot lists
* **Shashlik226** - For making most of the features work (passes, stattrak swaps, etc)
* **sasha190409** - Prime toggle & Overwatch integration

## Third party dependencies
- [Crypto++](https://github.com/weidai11/cryptopp) ([Boost Software License](https://github.com/weidai11/cryptopp/blob/master/License.txt))
- [funchook](https://github.com/kubo/funchook) ([GPL v2 with Classpath Exception](https://github.com/kubo/funchook/blob/master/LICENSE))
- [diStorm3](https://github.com/gdabah/distorm) ([3-Clause BSD License](https://github.com/gdabah/distorm/blob/master/COPYING))
- [protobuf](https://github.com/protocolbuffers/protobuf) ([3-Clause BSD License](https://github.com/protocolbuffers/protobuf/blob/main/LICENSE))
