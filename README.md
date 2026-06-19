# csgo_gc

> [!NOTE]
> This is a **fork** of [mikkokko/csgo_gc](https://github.com/mikkokko/csgo_gc) focused on getting the
> Game Coordinator working with **Counter-Strike 2**. It has diverged substantially from upstream:
> the client now completes the CS2 GC handshake, loadouts load, equipped skins/gloves/agents apply in
> offline/listen-server matches, loadout changes persist, and case opening adds the unboxed item to your
> inventory. It is still experimental — expect rough edges.

## What is this?
In Valve games, the Game Coordinator (GC) is a backend service most notably responsible for matchmaking and inventory management (like loadouts and skins). This project redirects the GC traffic to a custom, in-process implementation, so your inventory is driven by a local `inventory.txt` instead of Valve's servers.

## Why would you want this?
It restores most GC-related, inventory-side functionality (loadouts, skins, knives, gloves, agents, case opening, etc.) locally, without relying on a centralized server.

## Current features
- Editable inventory (`inventory.txt`)
- Loadout editing that **persists** (weapon/skin changes, base-weapon swaps like USP-S/P2000, save across restarts)
- Equipped items apply **in matches** (offline practice / bots / your own listen server)
- Knives, **gloves**, and **agents**
- Graffiti support
- Weapon StatTrak support
- Storage Units
- StatTrak Swaps
- Trade ups
- Stickers and patches
- Name tags
- Music kits
- Opening cases (including sticker capsules, patch packs, graffiti boxes and music kit boxes) — the
  unboxed item is added to your inventory, though CS2 shows an error dialog instead of the unbox reveal
  (see [Scope / limitations](#scope--limitations))
- In-game store
- Works without full Steam API emulation
- Windows, Linux and macOS support
- Networking using Steam's P2P interface

## Scope / limitations
- **Skins only apply on servers running csgo_gc** — i.e. offline practice, workshop maps, or your own
  listen/dedicated server. Valve matchmaking and community servers run their own GC and will show
  default weapons; this cannot be changed.
- **Matchmaking is not supported** and can't be (it requires a centralized server).
- **Case opening shows an error dialog instead of the unbox reveal.** The unbox itself works — the
  client unboxes via a Game Coordinator *job* (`k_EMsgGCOpenCrate`), the local GC creates the item and
  adds it to your inventory — but CS2 still pops a "we are unable to retrieve your item" dialog rather
  than playing the X-ray reveal. It's cosmetic; the item is yours regardless.

## Installation
- Download the game from Steam
- Download the latest build for your platform from the [releases page](https://github.com/programmer1o1/csgo_gc/releases) (the `continuous` release is built automatically from the latest commit)
- Navigate to the game's installation directory
- Back up your existing launcher executables as they'll be overwritten (e.g. `csgo.exe`, `cs2.exe`, `srcds.exe`, `csgo_linux64`, etc.)
- Extract the contents of the downloaded archive to your game directory, replacing the executables when prompted
- Launch the game. If you get the VAC message box, launch with the `-steam` argument
- macOS users: the release binaries are not notarized, so you'll have to deal with that on your end

## Inventory editing
Edit `csgo_gc/inventory.txt` (in the game's `game/` directory) directly, or use a GUI editor.
A manual-editing guide made by someone else is [here](https://gist.github.com/dricotec/1ae3deb06c42012970c00df914348e76).

> [!TIP]
> The game rewrites `inventory.txt` whenever you change your loadout in the menu, so do any manual or
> external edits **while the game is closed** — otherwise the game's save will overwrite them.

## Configuration
See [examples/config.txt](examples/config.txt) for available options.

## Building
Requirements:
- Git
- CMake 3.20 or newer
- C++ compiler with C++20 support (VS 2022, recent Clang, or recent GCC)

The CS:GO client and Linux dedicated server binaries are 32-bit; CS2 (`cs2`) is 64-bit. The build
produces both as appropriate. See [.github/workflows/build.yml](.github/workflows/build.yml) for the
exact, per-platform commands used by CI.

Windows (32-bit targets):

`cmake -A Win32 -B build`

Windows (64-bit / CS2):

`cmake -A x64 -B build_x64`

Linux dedicated server (32-bit):

`cmake -DCMAKE_C_FLAGS=-m32 -DCMAKE_CXX_FLAGS=-m32 -DCMAKE_ASM_FLAGS=-m32 -B build`

macOS (build for x86_64, not arm64):

`cmake -DCMAKE_OSX_ARCHITECTURES=x86_64 -DFUNCHOOK_CPU=x86 -B build`

For Linux clients you don't have to specify any additional options.

## License
This project is licensed under the 2-Clause BSD License. See [LICENSE.md](LICENSE.md) for details.

## Credits
* **Mikko Kokko** — Original author of [csgo_gc](https://github.com/mikkokko/csgo_gc)
* **Theeto** — Code reused from the predecessor project, unusual loot lists
* Fork maintainer — CS2 GC connectivity, in-match inventory, gloves/agents, loadout persistence, and case-opening work

## Third party dependencies
- [Crypto++](https://github.com/weidai11/cryptopp) ([Boost Software License](https://github.com/weidai11/cryptopp/blob/master/License.txt))
- [funchook](https://github.com/kubo/funchook) ([GPL v2 with Classpath Exception](https://github.com/kubo/funchook/blob/master/LICENSE))
- [diStorm3](https://github.com/gdabah/distorm) ([3-Clause BSD License](https://github.com/gdabah/distorm/blob/master/COPYING))
- [protobuf](https://github.com/protocolbuffers/protobuf) ([3-Clause BSD License](https://github.com/protocolbuffers/protobuf/blob/main/LICENSE))
