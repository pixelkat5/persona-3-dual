# Persona 3 Dual — Asset Pipeline

> Everything in `assets/` is source. Everything in `source/` or `nitrofiles/` is generated.  
> **Never hand-edit generated files** — run `make assets` instead.

---

## Quick Start

```bash
# First time only — set up venv (No external dependencies like Pillow required!)
python3 -m venv ~/.venv
source ~/.venv/bin/activate

# Full build (assets + ROM)
make

# Asset conversion only
make assets

# One category at a time
make dialogue
make music
make video
make environments
make models
make maps

# See all targets and flags
make help
```

---

## Directory Layout

```text
assets/
  dialogue/       .dlg scripts          → source/dialogue/
  music/          .mp3 music            → nitrofiles/music/
  sfx/            .wav SFX              → soundbank.bin (via mmutil)
  video/          .mp4 cutscenes        → nitrofiles/video/
  environments/   .obj static rooms     → source/environments/
  models/         .json animated models → source/models/
  maps/           .png collision maps   → source/maps/

source/
  dialogue/       *_dialogue.h + .cpp   (auto-generated)
  environments/   *_env.h + .s + tex    (auto-generated)
  models/         *.h hierarchy headers (auto-generated)
  maps/           *.h collision arrays  (auto-generated)

nitrofiles/
  music/          *.pcm                 (auto-generated)
  video/          *.vid                 (auto-generated)

tools/
  dlg2dialogue.py
  build_environment.py
  obj2environment.py
  obj2model.py
  video2vid.py
```

---

## Tools Reference

---

### `build_environment.py` — 3D Static Environment Pipeline

Converts an `.obj` + `.mtl` + `.png` collection into a zero-boilerplate C++ header, raw `.s` assembly display lists, and GRIT-compiled textures. 

*(See `ENVIRONMENT_README.md` for full details and Blender plugin instructions).*

| | |
|---|---|
| **Input** | `assets/environments/<name>/<name>.obj` (plus textures) |
| **Output** | `source/environments/<name>/<name>_env.h`, `.s`, and texture headers |

**Make flags**

```bash
make environments ENV_FLAGS="--no-center" # Override default Blender Z-up fix
```

---

### `obj2model.py` — 3D Animated Model Converter

Compiles a hierarchical `.json` skeleton and its associated `.obj` body parts into a C++ header containing pre-compiled display lists and keyframes for the `AnimationController`.

*(See `MODEL_README.md` for full details on Blockbench/Blender plugin usage).*

| | |
|---|---|
| **Input** | `assets/models/<name>/<name>[_WxH].json` (plus `.obj` parts) |
| **Output** | `source/models/<name>.h` |

**Filename encoding — texture size**

Encode the texture dimensions directly in the JSON filename using `_WxH`. If omitted, it falls back to `MODEL_TEXSIZE`.

```text
player_64x64.json   →  --texsize 64 64
enemy.json          →  uses MODEL_TEXSIZE default (32x32)
```

**Make flag — global fallback**

```bash
make models MODEL_TEXSIZE='128 128'
```

### `dlg2dialogue.py` — Dialogue compiler

Compiles a `.dlg` scene script into a C++ header + source pair for `DialogueController`.

| | |
|---|---|
| **Input** | `assets/dialogue/<name>.dlg` |
| **Output** | `source/dialogue/<name>_dialogue.h` + `.cpp` |

**Make flag**

```bash
make dialogue DLG_FLAGS='--stdout'   # dump to stdout instead of writing files (debug)
```

---

### `ffmpeg` — Music converter (PCM)

Converts MP3s to raw 16-bit stereo PCM at 32 kHz for `maxmod`.

| | |
|---|---|
| **Input** | `assets/music/<name>.mp3` |
| **Output** | `nitrofiles/music/<name>.pcm` |

---

### `video2vid.py` — Video encoder

Encodes MP4 video to an interleaved `.vid` file.

| | |
|---|---|
| **Input** | `assets/video/<name>.mp4` |
| **Output** | `nitrofiles/video/<name>.vid` |

**Make flags**

| Flag | Default | Values |
|---|---|---|
| `VIDEO_BITS` | `8` | `8` (8-bpp palette) or `16` (BGR555) |
| `VIDEO_FPS`  | `15` | any integer |
| `VIDEO_SIZE` | `256x192` | `WxH` string |

```bash
make video VIDEO_FPS=24 VIDEO_BITS=16 VIDEO_SIZE=256x192
```

---

## Naming Convention Summary

| Asset | Filename pattern | Encoded info |
|---|---|---|
| Dialogue | `<scene>.dlg` | — |
| Music | `<name>.mp3` | — |
| SFX | `<name>.mp3` | — |
| Video | `<name>.mp4` | — |
| Environment | `<name>.obj` (in `<name>/` folder) | Auto-processed alongside local `.mtl` / `.png` |
| Model | `<name>_[W]x[H].json` | Tex size (optional, default = `MODEL_TEXSIZE`) |
| Collision map | `<name>_[x]_[y]_[w]_[h].png` | Crop rect (optional, default = full image) |

---

## Incremental Builds

Make tracks input → output dependencies. Re-running `make assets` only reconverts files whose source is **newer** than the output. To force a full reconvert:

```bash
make clean && make assets
```

To force a single category:

```bash
touch assets/environments/dorm/dorm.obj && make environments
```
```