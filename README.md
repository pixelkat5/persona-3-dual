# Persona 3 Dual

A Nintendo DS demake of **Persona 3**, developed in C++ using devkitpro. This project is a personal deep-dive into embedded systems and low-level graphics programming, building a game engine from scratch without external game libraries.

The name is a nod to the online joke about a DS version of Persona 3 called "Persona 3 Dual".

### Want to help? Join the [Discord!](https://discord.gg/TPBXX8tmSG) Any help, big or small, would be greatly appreciated!

<img width="444" height="519" alt="20260417_231005" src="https://github.com/user-attachments/assets/8dd225c8-f3eb-4b80-8ec4-feb381e4b71d" />

---

## Screenshots

| Home | Menu | Iwatodai Dorm |
|------|------|----------------------|
| ![Home](https://github.com/user-attachments/assets/735128dd-a5aa-403f-8d94-748d6294e27c) | ![Menu](https://github.com/user-attachments/assets/72eb8264-9e56-4666-84a9-07bfe96a0a78) | ![Dorm](https://github.com/user-attachments/assets/dad1562b-67ae-4566-9d04-f2a9966ba916) |

---

## Technical Details

| | |
|---|---|
| **Platform** | Nintendo DS |
| **CPU** | ARM9 (main) / ARM7 (sound/system) |
| **Toolchain** | devkitPro (devkitARM + libnds) |
| **Language** | C++, Python |
| **Emulator** | melonDS |

---

## Developer Setup

### 1. Install System Dependencies

**Windows:**
1. Install [devkitPro](https://devkitpro.org/wiki/Getting_Started) using the official graphical installer (ensure `nds-dev` is checked).
2. Install Python 3 from the [official website](https://www.python.org/downloads/) (Check "Add Python to PATH" during install).
3. Install FFmpeg: Open Command Prompt or PowerShell and run `winget install ffmpeg`.
4. Install [melonDS](https://melonds.kuribo64.net/downloads.php) (and optionally add melonDS to Path for debugger setup)

**macOS:**
1. Install [devkitPro](https://devkitpro.org/wiki/Getting_Started) via the official pacman pkg.
2. Install Python and FFmpeg using Homebrew:
   `brew install python ffmpeg`
3. Install [melonDS](https://melonds.kuribo64.net/downloads.php)

**Linux (Ubuntu/Debian):**
1. Install devkitPro by following the [Unix-like platforms guide](https://devkitpro.org/wiki/Getting_Started#Unix-like_platforms).
2. Install Python and FFmpeg:
   `sudo apt update && sudo apt install python3 python3-venv ffmpeg`
3. Install [melonDS](https://melonds.kuribo64.net/downloads.php)

### 2. Setup the Project
Once the system tools are installed, open your terminal, clone the repo, navigate to the project folder, and set up the Python environment:

```
python3 -m venv .venv
source .venv/bin/activate
pip install -r tools/requirements.txt
```

You can then build a .nds rom by running ```make clean && make``` in the project folder.

---

## Features

- **Animated Intro & Main Menu** — Multi-layer 2D compositing with sinusoidal sprite/background animations, fade effects, and a custom logo split across two stitched sprites. The sub-screen has a static layer plus an animated attributions layer. "Press Start" animates independently of screen brightness.
- **State Machine Architecture** — View-based state machine with clean scene separation (Disclaimers → Intro → Main Menu → Iwatodai Dorm). Each scene has its own `.cpp`/`.h` pair with dedicated init and cleanup functions.
- **2D Graphics Pipeline** — Manual VRAM bank allocation supporting 4 simultaneous background layers with extended palette support. VRAM placement is calculated via tile-count analysis to prevent memory corruption.
- **3D Environment** — Fixed-function 3D rendering with display list geometry, UV-mapped textures, and free camera controls (rotate + translate). The camera orbits and follows the player, updating movement direction relative to facing angle.
- **Collision & Interaction System** — Tile-based 2D collision map overlaid on the 3D world. Zones are typed (wall, save point, scene transition, NPC trigger) to drive interactivity like zone loading and character encounters.
- **Dialogue Controller** — Fully-featured dialogue system with character-by-character text animation, per-line character portraits, branching dialogue trees with d-pad navigation, scrollable history, and automatic word-wrapping. Dialogue is authored in plain Markdown and converted to header files via a custom generator.
- **Decoupled Controller Architecture** — Character movement, camera, dialogue, music, and video are each in separate controller files, making the codebase modular and easy to extend.
- **Audio** — PCM audio streaming via NitroFS with loop point support for seamless section looping. Also supports SFX via Maxmod.
- **Video** — Optimised video playback at 8-bit colour, 15fps, with audio and video frames interleaved in a single file and streamed via a ring buffer. Uses NitroFS.
- **Custom Tooling** — Python utilities + Blender/Blockbench plugins built alongside the engine. Asset conversion is integrated into the build system via `make`.

Key changes made (v0.2):
- **State Machine** — added Disclaimers as the first scene
- **Dialogue Controller** — added auto word-wrapping and Markdown sourcing
- **Audio** — updated from mp3/libmad to PCM streaming with loop points and Maxmod SFX
- **Video** — replaced the old frame-streaming description with the optimised interleaved ring-buffer approach
- **Custom Tooling** — added the Markdown dialogue generator, world offset calculator, and build system integration; also cleaned up the `obj2bin.py` link formatting

---

## Roadmap

See the [Project Board](https://github.com/users/TheBossT910/projects/2) for current progress and open issues.

- [x] Intro sequence with animated backgrounds and sprites
- [x] Disclaimers screen
- [x] Main menu with interactive option selector
- [x] 3D environment rendering with UV-mapped textures
- [x] Basic camera controls
- [x] Character model + movement controls
- [x] Collision + interaction detection
- [x] Dialogue system with branching, portraits, and scrollable history
- [x] Auto word-wrap + Markdown → header dialogue generator
- [x] Music/SFX playback (PCM streaming via NitroFS, loop points, Maxmod SFX)
- [x] Video playback (8-bit, 15fps, interleaved audio+video, ring buffer)
- [x] Custom intro video (Persona 3 Dual logo added)
- [x] Automated asset conversion pipeline (integrated into `make`)
- [x] Character walk animation + AnimationController
- [ ] Proper character 3D model
- [ ] Combat system (UI layout, selection wheel, battle logic)
- [ ] Iwatodai Dorm — fully detailed environment with proper textures
- [ ] Additional scenes (world map, school classroom, Tartarus)
- [ ] Transition polish (remove garbage frame between Disclaimers → Video)

---

## Inspiration

Based on the **Persona 3** series of games (OG, FES, Portable, Reload), and inspired by the **Persona 3 Dual** online joke.

*This is a fan project and is not affiliated with or endorsed by Atlus or Sega.*
