# JMAP Reference

This document outlines the standardization of the tile mapping definitions used in the 3D environments.

To ensure forward compatibility and ease of expansion, map IDs are segmented into **blocks of 100**. This guarantees that any section can be cleanly appended with new tiles, interactions, or states without disrupting other categories.

## Id Block Standardizations

### 0–99: Core / System
**Purpose:** Foundational map mechanics that dictate basic collision, loading, navigation, and system-level states.
*Examples:* Walls (`w`), Collision (`c`), Save points (`s`).

### 100–199: Environment / Terrain
**Purpose:** Physical characteristics of the explorable space that may alter movement or visual properties.
*Examples:* Stairs (`stair_u`, `stair_d`), Elevators (`elev`), Doors (`door`).

### 200–299: Interactables / Objects
**Purpose:** Static objects in the environment that the player can interact with to retrieve items, progress mechanics, or view details.
*Examples:* Treasure Chests (`chest`), Item Pickups (`item`), Switches (`switch`), Shops (`shop`).

### 300–399: Encounter / Combat
**Purpose:** Triggers and zones pertaining to combat pacing, enemy encounters, and safe room states.
*Examples:* *Currently Unassigned*

### 400–499: Scene / Event
**Purpose:** Triggers for transitioning between different events, or scenes.
*Examples:* Scene_0 (`scene_0`), Scene_1 (`scene_1`), etc.

### 500–599: In-Game Cutscene / Dynamic Events
**Purpose:** Advanced, dynamic in-engine scenes and mapping sequences.
*Examples:* *Currently Unassigned*

### 600–699: Characters (Party)
**Purpose:** Placement and anchors for playable party members.
*Examples:* Main Character (`c_mc`), Yukari (`c_yu`), Junpei (`c_ju`), Akihiko (`c_ak`), etc.

### 700–799: Characters (NPCs)
**Purpose:** Placement and anchors for non-playable characters, vendors, and narrative-specific actors.
*Examples:* Social Links (`n_sl`), Merchants (`n_mer`), Generic NPCs (`n_gen`).

### 800–899: Enemies (Shadows)
**Purpose:** Overworld manifestations of enemies and their general difficulty or behavior types.
*Examples:* Weak Shadows (`shd_w`), Strong Shadows (`shd_s`).

### 900–999: Reserved / Extended
**Purpose:** Technical anchors, dev placeholders, rendering hints, or unassigned debug features.
*Examples:* Spawn Points (`spawn`), Navmesh boundaries (`nav`), Meta regions (`meta`), Null tiles (`null`).

---
*Note: Any legacy aliases are retained in the `_legacy_aliases` dictionary solely for backward compatibility with old level files.*
