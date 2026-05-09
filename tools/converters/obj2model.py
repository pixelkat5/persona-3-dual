import struct, argparse, json, os

FIFO_COLOR    = 0x20
FIFO_TEXCOORD = 0x22
FIFO_BEGIN    = 0x40
FIFO_VERTEX16 = 0x23
FIFO_NOP      = 0x00
GL_TRIANGLES = 0
GL_QUADS     = 1

def floattov16(f):
    v = int(f * (1 << 12))
    return max(-32768, min(32767, v)) & 0xFFFF

def floattot16(f):
    v = int(f * (1 << 4))
    return max(-32768, min(32767, v)) & 0xFFFF

def rgb_to_rgb15(r, g, b):
    return ((r >> 3) & 0x1F) | (((g >> 3) & 0x1F) << 5) | (((b >> 3) & 0x1F) << 10)

def pack_cmds(c1, c2=FIFO_NOP, c3=FIFO_NOP, c4=FIFO_NOP):
    return struct.pack('<I', (c4 << 24) | (c3 << 16) | (c2 << 8) | c1)

def to_s16(val):
    val = int(val) & 0xFFFF
    return val if val <= 32767 else val - 65536

def parse_obj(input_file):
    vertices = []
    texcoords = []
    faces = []
    with open(input_file, 'r') as f:
        for line in f:
            parts = line.split()
            if not parts: continue
            if parts[0] == 'v':
                vertices.append((float(parts[1]), float(parts[2]), float(parts[3])))
            elif parts[0] == 'vt':
                texcoords.append((float(parts[1]), float(parts[2])))
            elif parts[0] == 'f':
                face = []
                for p in parts[1:]:
                    components = p.split('/')
                    v_idx  = int(components[0]) - 1
                    vt_idx = int(components[1]) - 1 if len(components) > 1 and components[1] != '' else None
                    face.append((v_idx, vt_idx))
                faces.append(face)
    return vertices, texcoords, faces

def build_display_list(vertices, texcoords, faces, tex_width, tex_height, vertex_color, scale, offset, blender_source):
    has_uvs = any(vt is not None for face in faces for _, vt in face)
    words = []

    if vertex_color is not None:
        r, g, b = vertex_color
        words.append(pack_cmds(FIFO_COLOR))
        words.append(struct.pack('<I', rgb_to_rgb15(r, g, b)))

    ox, oy, oz = offset

    triangles = []
    quads = []
    for face in faces:
        if len(face) == 3:
            triangles.append(face)
        elif len(face) == 4:
            quads.append(face)
        else:
            for i in range(1, len(face) - 1):
                triangles.append([face[0], face[i], face[i + 1]])

    def emit_faces(face_list, prim_type):
        if not face_list:
            return

        words.append(pack_cmds(FIFO_BEGIN))
        words.append(struct.pack('<I', prim_type))

        for face in face_list:
            if blender_source and len(face) >= 2:
                face = [face[1], face[0]] + face[2:]

            for v_idx, vt_idx in face:
                if has_uvs and vt_idx is not None and tex_width and tex_height:
                    u, v = texcoords[vt_idx]
                    u16 = floattot16(u * tex_width)
                    v16 = floattot16((1.0 - v) * tex_height)
                    words.append(pack_cmds(FIFO_TEXCOORD))
                    words.append(struct.pack('<I', (u16 & 0xFFFF) | ((v16 & 0xFFFF) << 16)))

                vx, vy, vz = vertices[v_idx]
                sx = (vx - ox) * scale
                sy = (vy - oy) * scale
                sz = (vz - oz) * scale

                words.append(pack_cmds(FIFO_VERTEX16))
                words.append(struct.pack('<I', (floattov16(sy) << 16) | floattov16(sx)))
                words.append(struct.pack('<I', floattov16(sz)))

    emit_faces(quads, GL_QUADS)
    emit_faces(triangles, GL_TRIANGLES)

    return words

def optimize_keyframes(keyframes):
    if len(keyframes) <= 2: return keyframes
        
    optimized = [keyframes[0]]
    for i in range(1, len(keyframes) - 1):
        prev, curr, nxt = optimized[-1], keyframes[i], keyframes[i+1]
        if (curr.get('rot', [0,0,0]) == prev.get('rot', [0,0,0]) == nxt.get('rot', [0,0,0])) and \
           (curr.get('pos', [0,0,0]) == prev.get('pos', [0,0,0]) == nxt.get('pos', [0,0,0])):
            continue
        optimized.append(curr)
        
    optimized.append(keyframes[-1])
    return optimized

def convert_model_json(input_file, output_file, tex_width=None, tex_height=None, vertex_color=None, scale=None, target_size=4.0, center=True, blender_source=False):
    base_dir = os.path.dirname(input_file)
    model_name = os.path.splitext(os.path.basename(input_file))[0].replace('-', '_')
    
    with open(input_file, 'r') as f:
        data = json.load(f)
    
    bin_out = output_file if output_file.endswith('.bin') else os.path.splitext(output_file)[0] + '.bin'
    
    all_verts = []
    obj_data = {}
    for node in data['nodes']:
        obj_path = os.path.join(base_dir, node['obj'])
        v, vt, f = parse_obj(obj_path)
        if blender_source: v = [(x, z, y) for x, y, z in v]
        obj_data[node['id']] = (v, vt, f)
        all_verts.extend(v)

    if not all_verts: all_verts = [(0,0,0)]
        
    xs, ys, zs = [pt[0] for pt in all_verts], [pt[1] for pt in all_verts], [pt[2] for pt in all_verts]
    max_dim = max(max(xs)-min(xs), max(ys)-min(ys), max(zs)-min(zs))
    if scale is None: scale = (target_size / max_dim) if max_dim > 0 else 1.0
    offset = ((min(xs)+max(xs))/2.0, min(ys), (min(zs)+max(zs))/2.0) if center else (0.0, 0.0, 0.0)

    node_count = len(data['nodes'])
    
    with open(bin_out, 'wb') as out:
        out.write(b'MDL1')
        out.write(struct.pack('<I I', node_count, len(data['animations'])))

        for node in data['nodes']:
            nid = node['id']
            pid = node['parent']
            origin = node.get('origin', [0, 0, 0])
            if blender_source: origin = [origin[0], origin[2], origin[1]]
                
            bb_scale = 1.0 if blender_source else 16.0
            
            # Global Absolute Pivot
            ox = to_s16(floattov16(((origin[0] / bb_scale) - offset[0]) * scale))
            oy = to_s16(floattov16(((origin[1] / bb_scale) - offset[1]) * scale))
            oz = to_s16(floattov16(((origin[2] / bb_scale) - offset[2]) * scale))

            v, vt, f = obj_data[nid]
            
            # Reverted to Absolute Geometry Generation (Passing standard offset)
            words = build_display_list(v, vt, f, tex_width, tex_height, vertex_color, scale, offset, blender_source)
            
            out.write(struct.pack('<i i i i I', pid, ox, oy, oz, len(words)))
            for w in words: out.write(w)

        for anim_name, anim_data in data['animations'].items():
            name_bytes = anim_name.encode('utf-8')[:31].ljust(32, b'\0')
            
            # Reverted the * 60.0 multiplier
            anim_duration = int(float(anim_data['duration']))
            out.write(struct.pack('<32s I', name_bytes, anim_duration))

            for node_id in range(node_count):
                str_id = str(node_id)
                optimized_kf = []
                if str_id in anim_data['tracks']:
                    optimized_kf = optimize_keyframes(anim_data['tracks'][str_id])
                
                out.write(struct.pack('<I', len(optimized_kf)))
                
                for kf in optimized_kf:
                    rot = kf.get('rot', [0,0,0])
                    pos = kf.get('pos', [0,0,0])
                    if blender_source:
                        rot, pos = [rot[0], rot[2], rot[1]], [pos[0], pos[2], pos[1]]

                    rx = to_s16((rot[0] / 360.0) * 32768)
                    ry = to_s16((rot[1] / 360.0) * 32768)
                    rz = to_s16((rot[2] / 360.0) * 32768)
                    
                    bb_scale = 1.0 if blender_source else 16.0
                    px = to_s16(floattov16((pos[0] / bb_scale) * scale))
                    py = to_s16(floattov16((pos[1] / bb_scale) * scale))
                    pz = to_s16(floattov16((pos[2] / bb_scale) * scale))
                    
                    # Reverted the * 60.0 multiplier, kept the strict 16-byte memory alignment
                    time_frames = int(float(kf['time']))
                    out.write(struct.pack('<i h h h h h h', time_frames, rx, ry, rz, px, py, pz))

    header_out = os.path.splitext(bin_out)[0] + '.h'
    with open(header_out, 'w') as hout:
        hout.write(f"#pragma once\n\n")
        hout.write(f"enum Model_{model_name} {{\n")
        for i, anim_name in enumerate(data['animations'].keys()):
            safe_name = anim_name.upper().replace('.', '_').replace('-', '_')
            hout.write(f"    MODEL_{model_name.upper()}_{safe_name} = {i},\n")
        hout.write("};\n")

    print(f"Written: {bin_out} / {header_out}")

def convert(input_file, output_file, config):
    tex_w, tex_h = None, None
    texsize = config.get("texsize")
    if isinstance(texsize, list) and len(texsize) >= 2: tex_w, tex_h = texsize[0], texsize[1]
    elif isinstance(texsize, str):
        parts = texsize.split()
        if len(parts) >= 2: tex_w, tex_h = int(parts[0]), int(parts[1])

    vertex_color = config.get("color")
    if isinstance(vertex_color, list) and len(vertex_color) == 3: vertex_color = tuple(vertex_color)

    scale = config.get("scale")
    target_size = config.get("target_size", 4.0)
    center = config.get("center", True)
    if config.get("no_center"): center = False
    blender_source = config.get("source_blender", False)

    if input_file.endswith('.json'):
        convert_model_json(input_file, output_file, tex_width=tex_w, tex_height=tex_h, vertex_color=vertex_color, scale=scale, target_size=target_size, center=center, blender_source=blender_source)
    else:
        convert_obj(input_file, output_file, tex_width=tex_w, tex_height=tex_h, vertex_color=vertex_color, scale=scale, target_size=target_size, center=center, blender_source=blender_source)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Convert OBJ/JSON to NDS display list')
    parser.add_argument('input', help='Input .obj or .json file')
    parser.add_argument('output', help='Output .bin file')
    parser.add_argument('--texsize', nargs=2, type=int, metavar=('W', 'H'))
    parser.add_argument('--color', nargs=3, type=int, metavar=('R', 'G', 'B'))
    parser.add_argument('--scale', type=float, default=None)
    parser.add_argument('--target-size', type=float, default=4.0)
    parser.add_argument('--no-center', action='store_true')
    parser.add_argument('--source-blender', action='store_true')
    args = parser.parse_args()

    cli_config = {
        "texsize": args.texsize, "color": args.color, "scale": args.scale,
        "target_size": args.target_size, "no_center": args.no_center, "source_blender": args.source_blender
    }
    convert(args.input, args.output, cli_config)