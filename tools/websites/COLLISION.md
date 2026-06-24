# Collision & Areas

Collision and scene transitions are driven by a 2D tile map overlaid on the 3D world. Each area has a `.jmap` file in `assets/maps/` that defines the map dimensions, music, and every tile's type.

---

## How Collision Works

The world is divided into a grid of equally-sized tiles. The `CharacterController` tracks the character's world-space position and converts it to tile coordinates using:

```
tileX = (worldX + worldOffsetX) / tileSize
tileZ = (worldZ + worldOffsetZ) / tileSize
```

Before each movement, the controller checks whether the destination tile is walkable. It tries three combinations to allow wall-sliding:

1. Full movement (X + Z)
2. X only (slide along a Z wall)
3. Z only (slide along an X wall)

The character has a bounding box (`characterSize`). Collision checks all four corners of the box, not just the center, so the character can't clip through narrow walls.

---

## Tile Types

Tiles are defined as integer values in the generated map header. The authoritative source is `CharacterController.h`:

| Value | Enum | jmap key | Description |
|-------|------|----------|-------------|
| 0 | `NO_COLLISION` | `w` | Walkable area |
| 1 | `COLLISION` | `c` | Solid wall, blocks movement |
| 2 | `SAVE` | `s` | Save interaction zone |
| 3 | `PREV_SCENE` | `p` | Transition to previous scene |
| 4 | `NEXT_SCENE` | `e` | Transition to next scene |
| 100 | `CHARACTER_Akihiko` | `c_a` | NPC trigger zone |

Tiles outside the map bounds default to `NO_COLLISION`.

---

## The jmap Format

Each area is authored as a `.jmap` file in `assets/maps/`. The build system converts it to a `.h` header via `build_asset.py`. **Do not edit the generated `.h` files directly.**

### Header

```
# My Area collision map  WIDTHxHEIGHT
```

- The `WIDTHxHEIGHT` comment is parsed to set map dimensions

### Tile Grid

The rest of the file is a comma-separated grid of tile keys, one row per line, left-to-right, top-to-bottom. Width × Height tiles total.

```
c, c, c, c, c
c, w, w, w, c
c, w, s, w, c
c, w, w, e, c
c, c, c, c, c
```

---

## How Scene Transitions Work

When the character steps on a `NEXT_SCENE` (`e`) or `PREV_SCENE` (`p`) tile, `isTileAt()` returns the corresponding `TileType`. The view's `Update()` checks this each frame and returns the appropriate `ViewState` to trigger a scene switch in `main.cpp`.

### Example — Dorm to Streets (IwatodaiDormView)

In `iwatodai_dorm.jmap`, the left edge has several `e` tiles:

```
e, w, w, ...
e, w, w, ...
```

In `IwatodaiDormView::Update()`:

```cpp
if (playerCtrl->isTileAt() == TileType::NEXT_SCENE) {
    musicCtrl.pause();
    return ViewState::IWATODAI_STREETS;
}
```

In `main.cpp`, returning `ViewState::IWATODAI_STREETS` causes `SwitchView(new IwatodaiStreetsView())`.

On the Streets side, `p` tiles in `iwatodai_streets.jmap` mark where the player arrives from the dorm. `IwatodaiStreetsView` handles `PREV_SCENE` to transition back.

---

## Adding a New Area

### 1. Create the jmap

Create `assets/maps/my_area.jmap`:

```
# My Area collision map  10x8
#
# Tile key:
#   c = collision
#   w = no_collision
#   e = next_scene
#   p = prev_scene
#   s = save

c, c, c, c, c, c, c, c, c, c
c, w, w, w, w, w, w, w, w, c
c, w, w, w, w, w, w, w, w, c
c, w, w, w, w, w, w, w, w, c
c, w, w, w, w, w, w, w, w, c
c, w, w, w, w, w, w, w, w, c
c, w, w, w, w, w, w, p, p, c
c, c, c, c, c, c, c, c, c, c
```

Run `make` to generate `source/maps/my_area.h`.

### 2. Add a ViewState

In `source/core/View.h`, add your new state to the `ViewState` enum:

```cpp
enum class ViewState {
    // ... existing states ...
    MY_AREA,
    KEEP_CURRENT
};
```

### 3. Create the View

Create `source/views/MyAreaView.h` and `MyAreaView.cpp`. At minimum:

```cpp
// MyAreaView.h
#pragma once
#include "core/View.h"
#include "core/globals.h"

class MyAreaView : public View {
    public:
        void Init() override;
        ViewState Update() override;
        void Cleanup() override;
};
```

```cpp
// MyAreaView.cpp
#include "MyAreaView.h"
#include "maps/my_area.h"

void MyAreaView::Init() {
    // setup 3D, music, playerCtrl etc.
    musicCtrl.init(MY_AREA_MUSIC, 0.0f, <duration>);
    playerCtrl = new CharacterController(
        MY_AREA_MAP_WIDTH, MY_AREA_MAP_HEIGHT,
        &my_area_map[0][0],
        tileSize, worldOffsetX, worldOffsetZ, ...
    );
}

ViewState MyAreaView::Update() {
    // ...
    if (playerCtrl->isTileAt() == TileType::PREV_SCENE) {
        musicCtrl.pause();
        return ViewState::IWATODAI_DORM; // or wherever
    }
    return ViewState::KEEP_CURRENT;
}

void MyAreaView::Cleanup() {
    // cleanup vram, controllers, etc.
}
```

### 4. Wire it up in main.cpp

```cpp
#include "views/MyAreaView.h"

// in the main loop:
} else if (nextState == ViewState::MY_AREA) {
    SwitchView(new MyAreaView());
}
```

### 5. Trigger the transition from an existing area

In the existing area's `.jmap`, place `e` tiles where the player should cross over. In that area's `Update()`, handle `NEXT_SCENE`:

```cpp
if (playerCtrl->isTileAt() == TileType::NEXT_SCENE) {
    musicCtrl.pause();
    return ViewState::MY_AREA;
}
```

---

## World Offset & Tile Size

`worldOffsetX` and `worldOffsetZ` shift the tile grid to align with the 3D geometry. `tileSize` sets how many world units one tile covers. These are defined per-view and must match the scale of the environment model. Use the world offset calculator tool in `/tools` if you need to recalculate them.
