# NDS Environment Exporter Toolchain

This toolchain provides a seamless pipeline for exporting 3D environments directly into optimized, hardware-ready Nintendo DS display lists. 

It handles geometry scaling, Z-up to Y-up coordinate conversion, material parsing, and automatic generation of C++ headers and GRIT sidecar files for easy integration into devkitARM Makefiles.

---

## Installation

### Command Line Tools (Standalone)
The CLI pipeline requires Python 3. No external dependencies are needed.
1. Place `build_environment.py` and `obj2environment.py` in your project's `tools/` directory (or anywhere accessible).
2. Ensure you have standard NDS development tools (devkitARM, GRIT) installed if you are building the output.

---

## How to Use Via Command Line
If you already have an `.obj` file or are running a batch build script:
```bash
python build_environment.py models/tartarus_block1.obj data/env/ --source-blender --target-size 4.0
```

---

## Output Files

For an input named `scene.obj`, the pipeline generates:
* **`scene.bin`**: The raw binary file containing the packed NDS display lists (FIFO commands, v16 vertices, t16 texcoords).
* **`scene.h`**: A C++ wrapper class (`scene_Environment`) with `load()`, `draw()`, and `cleanup()` methods, plus world bounding box definitions.
* **`[texture_name].png`**: Copied from your source materials.
* **`[texture_name].grit`**: Auto-generated sidecar files containing your GRIT flags (e.g., `-gb -gB16 -pu16`) so your devkitARM Makefile automatically converts them during the build step.

---

## 💻 Engine Integration Example

Here is how you would load and draw an exported environment inside your engine, such as the Persona 3 Dual custom engine:

```cpp
#include <nds.h>
#include "scene.h" // The auto-generated header

// Create your environment instance
scene_Environment currentLevel;

void initLevel() {
    // 1. You must provide the compiled GRIT bitmaps in the exact order 
    //    specified by the generated enum in scene.h.
    const unsigned int* textureData[SCENE_TEX_COUNT] = {
        (const unsigned int*)floor_texBitmap, 
        (const unsigned int*)wall_texBitmap
    };

    // 2. Load the binary display list from the filesystem (e.g., NitroFS)
    //    and bind the textures to VRAM.
    bool success = currentLevel.load("nitro:/env/scene.bin", textureData);
    if (!success) {
        printf("Failed to load environment!\n");
    }
}

void renderLevel() {
    // 3. Simply call draw() within your rendering loop
    currentLevel.draw();
}

void unloadLevel() {
    // 4. Free allocated memory and delete OpenGL textures
    currentLevel.cleanup();
}
```

---

## Quick Reference

### Exporter Settings

| Parameter / Flag | Description | Default |
| :--- | :--- | :--- |
| `Scale Mode` / `--scale`, `--target-size` | **Auto** scales the largest dimension of the mesh to `--target-size`. **Manual** multiplies vertices by `--scale`. | Auto, `4.0` |
| `Centre Model` / `--no-center` | Calculates the bounding box and translates the geometry so the origin is at the bottom-center of the mesh. | Enabled |
| `--source-blender` | Applies Blender's Z-up coordinate system to the NDS Y-up system and flips texture V-coordinates. | Auto via Plugin |
| `Skip GRIT` / `--skip-grit` | Prevents the generation of `.grit` sidecar files next to the output textures. | Disabled |
| `GRIT Flags` / `--grit-flags` | The pixel formatting flags written to the `.grit` sidecar files. | `-gb -gB16 -pu16` |
| `Force RGBA` / `--rgba` | Forces `GL_RGBA` for all textures in the environment, allowing the hardware to read the alpha bit for transparency. | Disabled |
| `RGBA List` / `--rgba-list` | A comma-separated list of specific texture filenames (e.g., `window.png,fence.png`) to selectively render using `GL_RGBA`. | Empty |
| `Vertex Color` / `--color` | Injects a `FIFO_COLOR` command to set the base vertex color (R G B). Acts as a multiplier for texture colors and prevents geometry from rendering black in `GL_RGBA` mode. | `[255, 255, 255]` |

### Technical Notes & Quirks
* **Texture Wrapping:** Tiled textures perfectly modulo wrap. The exporter uses 16-bit overflow (`floattot16`) to natively support infinite UV scrolling without clamping.
* **Valid Texture Sizes:** NDS hardware strictly requires power-of-two texture dimensions (8, 16, 32... 1024). The script automatically snaps your texture metadata to the nearest valid dimension.
* **VRAM Limits:** Remember that while the script exports unlimited geometry, your engine is still bound by NDS VRAM and vertex limits per frame. Group dense geometry accordingly.