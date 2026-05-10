#!/usr/bin/env python3
"""
gen_fontsheet.py — Generate NDS font spritesheets from TTF/OTF/EOT fonts.

USAGE:
    python gen_fontsheet.py -f font.ttf -s 16 -o output.png
    python gen_fontsheet.py -f font.ttf -s 16 -o output.png --color "#F5A623"
    python gen_fontsheet.py --config assets/fonts/fonts.json --base-dir /path/to/repo

OPTIONS:
    -f, --font          Font file (.ttf/.otf/.eot)
    -s, --size          Font size in px (default: 16)
    -o, --output        Output PNG (default: fontsheet.png)
    --chars             Characters to include (default: printable ASCII 32-126)
    --color             Foreground: "#RRGGBB" or R G B (default: white)
    --color2            Gradient end color — enables vertical gradient
    --bg                Background: "#RRGGBBAA" or R G B A (default: transparent)
    --cell-size         Square cell size in px (overrides auto)
    --cell-w            Cell width in px
    --cell-h            Cell height in px
    --cols              Grid columns (default: 16)
    --padding           Cell padding in px (default: 1)
    --outline           Outline thickness in px (default: 0)
    --outline-color     Outline color: "#RRGGBB" or R G B
    --shadow            Shadow offset in px (default: 0)
    --shadow-color      Shadow color
    --shadow-opacity    Shadow opacity 0-255 (default: 180)
    --align             left | center | right (default: center)
    --no-ds-align       Disable rounding cell size to multiples of 8
    --preview           Save a labeled preview PNG
    --config            JSON batch config file
    --base-dir          Base dir for resolving relative paths in config
"""

import argparse, io, json, os, sys
from PIL import Image, ImageDraw, ImageFont


# ── Color helpers ─────────────────────────────────────────────────────────────

def parse_color(values, allow_alpha=False):
    if not values:
        return (255, 255, 255, 255) if allow_alpha else (255, 255, 255)
    if len(values) == 1:
        h = str(values[0]).lstrip('#')
        if len(h) == 6:
            r, g, b = int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16)
            return (r, g, b, 255) if allow_alpha else (r, g, b)
        elif len(h) == 8:
            r, g, b, a = (int(h[i*2:(i+1)*2], 16) for i in range(4))
            return (r, g, b, a) if allow_alpha else (r, g, b)
        raise ValueError(f'Invalid hex: #{h}')
    ints = [int(v) for v in values]
    if len(ints) == 3: return (*ints, 255) if allow_alpha else tuple(ints)
    if len(ints) == 4 and allow_alpha: return tuple(ints)
    raise ValueError(f'Expected hex or 3/4 ints, got: {values}')

def lerp_color(c1, c2, t):
    return tuple(int(c1[i] + (c2[i] - c1[i]) * t) for i in range(len(c1)))

def normalize_color_field(cfg, key):
    v = cfg.get(key)
    if v is None: return
    cfg[key] = [v] if isinstance(v, str) else [str(x) for x in v]

def normalize_cfg(cfg):
    for k in ('color', 'color2', 'bg', 'outline_color', 'shadow_color'):
        normalize_color_field(cfg, k)
    return cfg


# ── Font loading ──────────────────────────────────────────────────────────────

def load_eot_as_ttf(path):
    with open(path, 'rb') as f: data = f.read()
    for magic in (b'\x00\x01\x00\x00', b'OTTO', b'true', b'typ1'):
        idx = data.find(magic)
        if idx != -1: return data[idx:]
    raise ValueError(f'No TTF magic in EOT: {path}')

def load_font(path, size):
    if os.path.splitext(path)[1].lower() == '.eot':
        return ImageFont.truetype(io.BytesIO(load_eot_as_ttf(path)), size)
    return ImageFont.truetype(path, size)


# ── Sheet generation ──────────────────────────────────────────────────────────

def measure_cell(font, chars, padding, ds_align):
    d = ImageDraw.Draw(Image.new('RGBA', (1, 1)))
    mw = mh = 0
    for c in chars:
        b = d.textbbox((0, 0), c, font=font)
        mw = max(mw, b[2] - b[0])
        mh = max(mh, b[3] - b[1])
    cw, ch = mw + padding * 2, mh + padding * 2
    if ds_align:
        cw = ((cw + 7) // 8) * 8
        ch = ((ch + 7) // 8) * 8
    return cw, ch

def make_gradient(w, h, c1, c2):
    g = Image.new('RGBA', (w, h))
    for y in range(h): g.paste(lerp_color(c1, c2, y / max(h - 1, 1)), (0, y, w, y + 1))
    return g

def generate(cfg):
    font_path = cfg['font']
    size      = cfg.get('size', 16)
    output    = cfg.get('output', 'fontsheet.png')
    chars     = cfg.get('chars') or ''.join(chr(c) for c in range(32, 127))
    cols      = cfg.get('cols', 16)
    padding   = cfg.get('padding', 1)
    ds_align  = cfg.get('ds_align', True)
    align     = cfg.get('align', 'center')
    preview   = cfg.get('preview', False)

    color      = parse_color(cfg.get('color') or ['#FFFFFF'])
    color2_raw = cfg.get('color2')
    color2     = parse_color(color2_raw) if color2_raw else None
    bg         = parse_color(cfg.get('bg') or ['#00000000'], allow_alpha=True)
    ot         = int(cfg.get('outline', 0))
    oc         = parse_color(cfg.get('outline_color') or ['#000000'])
    sh         = int(cfg.get('shadow', 0))
    sc         = parse_color(cfg.get('shadow_color') or ['#000000'])
    sa         = int(cfg.get('shadow_opacity', 180))

    cw = cfg.get('cell_w')
    ch = cfg.get('cell_h')
    cs = cfg.get('cell_size')
    if cs: cw = ch = int(cs)

    font = load_font(font_path, size)
    if not cw or not ch:
        aw, ah = measure_cell(font, chars, padding, ds_align)
        cw = int(cw or aw)
        ch = int(ch or ah)
    cw, ch = int(cw), int(ch)

    rows = (len(chars) + cols - 1) // cols
    iw, ih = cw * cols, ch * rows
    sheet = Image.new('RGBA', (iw, ih), bg)
    draw  = ImageDraw.Draw(sheet)

    for i, char in enumerate(chars):
        ci, ri = i % cols, i // cols
        cx0, cy0 = ci * cw, ri * ch
        b = draw.textbbox((0, 0), char, font=font)
        cw2, ch2 = b[2] - b[0], b[3] - b[1]

        if align == 'left':    cx = cx0 + padding - b[0]
        elif align == 'right': cx = cx0 + cw - padding - cw2 - b[0]
        else:                  cx = cx0 + padding + (cw - padding * 2 - cw2) // 2 - b[0]
        cy = cy0 + padding + (ch - padding * 2 - ch2) // 2 - b[1]

        if sh > 0:
            draw.text((cx + sh, cy + sh), char, font=font, fill=(*sc, sa))
        if ot > 0:
            for dx in range(-ot, ot + 1):
                for dy in range(-ot, ot + 1):
                    if dx or dy: draw.text((cx + dx, cy + dy), char, font=font, fill=(*oc, 255))
        if color2:
            tmp = Image.new('RGBA', (cw, ch), (0, 0, 0, 0))
            ImageDraw.Draw(tmp).text((cx - cx0, cy - cy0), char, font=font, fill=(255, 255, 255, 255))
            grad = make_gradient(cw, ch, (*color, 255), (*color2, 255))
            grad.putalpha(tmp.split()[3])
            sheet.paste(grad, (cx0, cy0), grad)
        else:
            draw.text((cx, cy), char, font=font, fill=(*color, 255))

    sheet.save(output)
    print(f'Saved fontsheet  -> {output}')
    print(f'  {len(chars)} chars  |  cell {cw}x{ch}px  |  sheet {iw}x{ih}px')

    if preview:
        lh   = 12
        prev = Image.new('RGBA', (iw, ih + lh * rows), (30, 30, 30, 255))
        sf   = ImageFont.load_default()
        pd   = ImageDraw.Draw(prev)
        for i, char in enumerate(chars):
            ci, ri = i % cols, i // cols
            sx, sy = ci * cw, ri * (ch + lh)
            for ty in range(0, ch, 4):
                for tx in range(0, cw, 4):
                    if (tx // 4 + ty // 4) % 2 == 0:
                        pd.rectangle([sx+tx, sy+ty, sx+tx+3, sy+ty+3], fill=(55, 55, 55, 255))
            cell = sheet.crop((ci*cw, ri*ch, ci*cw+cw, ri*ch+ch))
            prev.paste(cell, (sx, sy), cell)
            pd.rectangle([sx, sy, sx+cw-1, sy+ch-1], outline=(80, 80, 80, 255))
            pd.text((sx+2, sy+ch+1), repr(char)[1:-1], font=sf, fill=(160, 160, 160, 255))
        pp = os.path.splitext(output)[0] + '_preview.png'
        prev.save(pp)
        print(f'Saved preview    -> {pp}')

    charBase = 32  # ASCII base
    base = os.path.splitext(output)[0]
    fn = os.path.basename(base).replace('-', '_').replace(' ', '_').replace('.', '_')
    tp = (cw // 8) * (ch // 8)
    with open(base + '.grit', 'w') as f:
        f.write(f'# grit config — gen_fontsheet.py\n')
        f.write(f'# Font: {font_path}  Size: {size}px  Cell: {cw}x{ch}\n\n')
        f.write(f'-gB4\n-gt\n-m!\n-Mw{iw//8}\n-Mh{ih//8}\n')
    with open(base + '_map.h', 'w') as f:
        f.write(f'#pragma once\n')
        f.write(f'// Font char->tile map — gen_fontsheet.py\n')
        f.write(f'// Font: {font_path}  Size: {size}px  Cell: {cw}x{ch}px\n\n')
        f.write(f'#define {fn.upper()}_CELL_W     {cw}\n')
        f.write(f'#define {fn.upper()}_CELL_H     {ch}\n')
        f.write(f'#define {fn.upper()}_COLS       {cols}\n')
        f.write(f'#define {fn.upper()}_CHAR_COUNT {len(chars)}\n')
        f.write(f'#define {fn.upper()}_TILES_PER  {tp}\n\n')
        f.write(f'// int tile = {fn}_CHAR_MAP[c - {charBase}] * {fn.upper()}_TILES_PER;\n\n')
        f.write(f'static const unsigned char {fn}_CHAR_MAP[{len(chars)}] = {{\n    ')
        f.write(', '.join(str(ord(c) - charBase) for c in chars))
        f.write('\n};\n')
    print(f'Saved NDS grit   -> {base}.grit')
    print(f'Saved NDS header -> {base}_map.h')
    print(f'  Run: grit {os.path.basename(output)} @{os.path.basename(base)}.grit')


# ── CLI ───────────────────────────────────────────────────────────────────────

def main():
    p = argparse.ArgumentParser(description='Generate NDS font spritesheets.')
    p.add_argument('-f', '--font')
    p.add_argument('-s', '--size',         type=int, default=16)
    p.add_argument('-o', '--output',       default='fontsheet.png')
    p.add_argument('--chars',              default=None)
    p.add_argument('--cell-size',          type=int, default=None)
    p.add_argument('--cell-w',             type=int, default=None)
    p.add_argument('--cell-h',             type=int, default=None)
    p.add_argument('--cols',               type=int, default=16)
    p.add_argument('--color',   nargs='+', default=['#FFFFFF'])
    p.add_argument('--color2',  nargs='+', default=None)
    p.add_argument('--bg',      nargs='+', default=['#00000000'])
    p.add_argument('--padding',            type=int, default=1)
    p.add_argument('--outline',            type=int, default=0)
    p.add_argument('--outline-color', nargs='+', default=['#000000'])
    p.add_argument('--shadow',             type=int, default=0)
    p.add_argument('--shadow-color', nargs='+', default=['#000000'])
    p.add_argument('--shadow-opacity',     type=int, default=180)
    p.add_argument('--align',  default='center', choices=['left', 'center', 'right'])
    p.add_argument('--no-ds-align', action='store_true')
    p.add_argument('--preview', action='store_true')
    p.add_argument('--config')
    p.add_argument('--base-dir', default=None)
    args = p.parse_args()

    base_dir = args.base_dir or os.getcwd()

    if args.config:
        if not os.path.exists(args.config):
            print(f'Error: config not found: {args.config}', file=sys.stderr); sys.exit(1)
        with open(args.config) as f:
            batch = json.load(f)
        for i, entry in enumerate(batch):
            print(f'\n[{i+1}/{len(batch)}] {entry.get("output", "fontsheet.png")}')
            cfg = normalize_cfg(entry)
            cfg.setdefault('ds_align', True)
            for key in ('font', 'output'):
                if cfg.get(key) and not os.path.isabs(cfg[key]):
                    cfg[key] = os.path.join(base_dir, cfg[key])
            generate(cfg)
        return

    if not args.font:
        p.error('--font is required')
    if not os.path.exists(args.font):
        print(f'Error: font not found: {args.font}', file=sys.stderr); sys.exit(1)

    cfg = normalize_cfg({
        'font':          args.font,
        'size':          args.size,
        'output':        args.output,
        'chars':         args.chars,
        'cell_size':     args.cell_size,
        'cell_w':        args.cell_w,
        'cell_h':        args.cell_h,
        'cols':          args.cols,
        'color':         args.color,
        'color2':        args.color2,
        'bg':            args.bg,
        'padding':       args.padding,
        'outline':       args.outline,
        'outline_color': args.outline_color,
        'shadow':        args.shadow,
        'shadow_color':  args.shadow_color,
        'shadow_opacity':args.shadow_opacity,
        'align':         args.align,
        'ds_align':      not args.no_ds_align,
        'preview':       args.preview,
    })
    generate(cfg)

if __name__ == '__main__':
    main()
