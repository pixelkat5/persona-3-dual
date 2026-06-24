let cameraZones = [];          // Array of zone objects
let camZoneMode  = false;      // "zone" draw mode active
let camGizmoMode = false;      // "place camera" gizmo mode active
let camZoneDraw  = null;       // { id, startTX, startTZ, curTX, curTZ } during rect drag
let camZoneSelected = null;    // index into cameraZones, or null
let camZoneOverlay  = null;    // <canvas> overlay element
let camGizmoDrag = null;       // { zoneIdx } while dragging a gizmo
let camCornerDrag = null;      // { zoneIdx, cornerIdx } while dragging a resize handle
let camCutoutDraw = null;      // { zoneIdx, startTX, startTZ, curTX, curTZ } during shift+drag cutout

// Corner order: 0=TL 1=TR 2=BR 3=BL, clockwise from top-left.
const CORNER_NEIGHBORS = [
  { sameTZ: 1, sameTX: 3 }, // TL
  { sameTZ: 0, sameTX: 2 }, // TR
  { sameTZ: 3, sameTX: 1 }, // BR
  { sameTZ: 2, sameTX: 0 }, // BL
];

const CAM_ZONE_COLORS = [
  "#e74c3c", "#3498db", "#2ecc71", "#f39c12", "#9b59b6",
  "#1abc9c", "#e67e22", "#e91e8c", "#00bcd4", "#cddc39"
];

function camZoneColor(id) {
  return CAM_ZONE_COLORS[id % CAM_ZONE_COLORS.length];
}

let TILE_DEFS = {};
let TILE_CATEGORIES = []; // Stores { id, name, tiles: [] }
let activeCategory = localStorage.getItem("me_active_category") || "all";
let currentJmapStem = "map"; // filename stem from the last loaded .jmap

function hslToHex(h, s, l) {
  l /= 100;
  const a = (s * Math.min(l, 1 - l)) / 100;
  const f = (n) => {
    const k = (n + h / 30) % 12;
    const color = l - a * Math.max(Math.min(k - 3, 9 - k, 1), -1);
    return Math.round(255 * color)
      .toString(16)
      .padStart(2, "0");
  };
  return `#${f(0)}${f(8)}${f(4)}`;
}

function generateDeterministicColor(seedString) {
  let hash = 0;
  for (let i = 0; i < seedString.length; i++) {
    hash = seedString.charCodeAt(i) + ((hash << 5) - hash);
  }
  const h = Math.abs(hash) % 360;
  return hslToHex(h, 70, 45);
}

async function loadTileDefinitions() {
  try {
    // Root-relative first (Vercel), then relative fallback (local dev server).
    let response = await fetch("/tile_map.json");
    if (!response.ok) response = await fetch("../../tile_map.json");
    if (!response.ok) throw new Error("tile_map.json not found at /tile_map.json or ../../tile_map.json");
    const data = await response.json();

    const shortcuts = ["1", "2", "3", "4", "5", "6", "7", "8", "9", "0"];
    let shortcutIdx = 0;


    TILE_CATEGORIES = [{ id: "all", name: "All Tiles", tiles: [] }];
    let currentGroup = TILE_CATEGORIES[0];

    for (const key of Object.keys(data.TILE_MAP_META)) {
      if (key === "_legacy_aliases") continue;

      if (key.startsWith("_group_")) {
        const groupName =
          data.TILE_MAP_META[key].split(":")[1]?.trim() ||
          data.TILE_MAP_META[key];
        currentGroup = { id: key, name: groupName, tiles: [] };
        TILE_CATEGORIES.push(currentGroup);
        continue;
      }

      const metaLabel = data.TILE_MAP_META[key];
      const baseColor = generateDeterministicColor(key + metaLabel);

      TILE_DEFS[key] = {
        label: metaLabel
          .replace(/_/g, " ")
          .replace(/\b\w/g, (l) => l.toUpperCase()),
        shortLabel: metaLabel.split("—")[0].trim().toLowerCase(),
        mapValue: data.TILE_MAP[key],
        key: shortcutIdx < shortcuts.length ? shortcuts[shortcutIdx] : "",
        bg: baseColor + "40",
        border: baseColor,
        textColor: "#ffffff",
      };

      TILE_CATEGORIES[0].tiles.push(key);
      if (currentGroup !== TILE_CATEGORIES[0]) {
        currentGroup.tiles.push(key);
      }

      shortcutIdx++;
    }
  } catch (error) {
    console.error(
      "Failed to load tile_map.json. Did you run a local server?",
      error,
    );
    alert(
      "Failed to load tile definitions from JSON. Check console for details.",
    );
  }
}

// UI GENERATORS
function buildCategorySelect() {
  const select = document.getElementById("category-select");
  if (!select) return;
  select.innerHTML = "";

  TILE_CATEGORIES.forEach((cat) => {
    // Hide empty groups (like Future Cutscenes) from the UI
    if (cat.tiles.length === 0) return;

    const opt = document.createElement("option");
    opt.value = cat.id;
    opt.textContent = cat.name;
    select.appendChild(opt);
  });

  select.value = activeCategory;
  if (!select.value) select.value = select.options[0]?.value || "all";
  activeCategory = select.value;

  // onchange avoids stacking duplicate listeners on repeated calls
  select.onchange = (e) => {
    activeCategory = e.target.value;
    try { localStorage.setItem("me_active_category", activeCategory); } catch (_) {}
    buildPalette();
  };
}

function buildPalette() {
  const el = document.getElementById("palette");
  el.innerHTML = "";

  const cat =
    TILE_CATEGORIES.find((c) => c.id === activeCategory) || TILE_CATEGORIES[0];

  cat.tiles.forEach((k) => {
    const def = TILE_DEFS[k];
    if (!def) return;
    const btn = document.createElement("div");
    btn.className = "tile-btn" + (k === currentTool ? " active" : "");
    btn.id = `tb-${k}`;
    btn.style.cssText = `background:${def.bg}; border-color:${def.border}; color:${def.textColor}`;
    btn.innerHTML = `
        <div class="tile-key">[${def.key || "-"}]</div>
        <div class="tile-code">${k}</div>
        <div class="tile-name">${def.shortLabel}</div>
    `;
    btn.dataset.tip = `Paint: ${def.label}${def.key ? ` · shortcut [${def.key}]` : ""}`;
    btn.onclick = () => selectTool(k);
    el.appendChild(btn);
  });
}

// STATE
let params = {
  tileSize: 0.0625,
  offsetX: 0,
  offsetZ: 0,
  width: 0,
  height: 0,
  name: "map",
  scale: 1.0,
  centered: true,
  source_blender: false,
};
let tileData = new Array(0 * 0).fill("w");
let currentTool = "w";
let isPainting = false;
let paintErase = false;
let currentView = "top";

// Undo / Redo (tiles)
let undoStack = [];
let redoStack = [];
let strokeBefore = null;

// Separate zone undo stack so Ctrl+Z applies to zones too
let zoneUndoStack = [];
let zoneRedoStack = [];

// Minimap
let minimapCanvas = null;
let minimapCtx = null;

// Free-look: camera orbits freeTarget in spherical coordinates
let isFreeLook = false;
let freeTheta = -Math.PI / 5; // azimuth
let freePhi = Math.PI / 3.5;   // polar
const FREE_RADIUS = 15;
let freeTarget = null; // THREE.Vector3, set in initThree
let freeDragStartX = 0;
let freeDragStartY = 0;
let freeDragMoved = false;
let freeDragButton = 0; // which button started the current free-look drag

// Three.js
let scene, camera, renderer;
let rawModelGroup = null;
let modelGroup, gridGroup, tileGroup;
let groundRaycaster;
let groundMesh;
let tileCanvasEl, tileCanvasCtx, tileTex, tilePlane;
let wireMaterial, solidMaterial;
let showWire = true,
  showSolid = false;
let showGrid = true,
  showModel = true;
let viewSize = 2.5;
let panX = 0,
  panZ = 0;
let isSpaceDown = false,
  isPanning = false;
let lastMX = 0,
  lastMY = 0;

// THREE.JS INIT
function initThree() {
  const canvas = document.getElementById("three-canvas");
  renderer = new THREE.WebGLRenderer({ canvas, antialias: true });
  renderer.setPixelRatio(window.devicePixelRatio);
  renderer.setClearColor(0x0d0d0f, 1);
  renderer.sortObjects = true;

  scene = new THREE.Scene();

  const vp = document.getElementById("viewport");
  const asp = vp.clientWidth / vp.clientHeight;
  camera = new THREE.OrthographicCamera(
    -viewSize * asp,
    viewSize * asp,
    viewSize,
    -viewSize,
    -1000,
    1000,
  );

  freeTarget = new THREE.Vector3(
    (params.width * params.tileSize) / 2 - params.offsetX,
    0,
    (params.height * params.tileSize) / 2 - params.offsetZ,
  );

  setView("top");

  scene.add(new THREE.AmbientLight(0xffffff, 0.7));
  const sun = new THREE.DirectionalLight(0xffffee, 0.6);
  sun.position.set(2, 5, 3);
  scene.add(sun);

  modelGroup = new THREE.Group();
  scene.add(modelGroup);
  gridGroup = new THREE.Group();
  scene.add(gridGroup);
  tileGroup = new THREE.Group();
  scene.add(tileGroup);

  wireMaterial = new THREE.MeshBasicMaterial({
    color: 0x6688aa,
    wireframe: true,
    transparent: true,
    opacity: 0.55,
  });
  solidMaterial = new THREE.MeshLambertMaterial({
    color: 0x7799bb,
    transparent: true,
    opacity: 0.45,
    side: THREE.DoubleSide,
  });

  groundMesh = new THREE.Mesh(
    new THREE.PlaneGeometry(10000, 10000),
    new THREE.MeshBasicMaterial({ visible: false, side: THREE.DoubleSide }),
  );
  groundMesh.rotation.x = -Math.PI / 2;
  scene.add(groundMesh);
  groundRaycaster = new THREE.Raycaster();

  setupEvents(canvas);
  rebuildGrid();
  rebuildTileLayer();
  resetCamera();

  window.addEventListener("resize", onResize);
  onResize();
  animate();
}

function animate() {
  requestAnimationFrame(animate);
  renderer.render(scene, camera);
}

function onResize() {
  const vp = document.getElementById("viewport");
  renderer.setSize(vp.clientWidth, vp.clientHeight);
  updateCameraFrustum();
}

// CAMERA
function updateCameraFrustum() {
  const vp = document.getElementById("viewport");
  const asp = vp.clientWidth / vp.clientHeight;

  if (isFreeLook) {
    camera.left = -viewSize * asp;
    camera.right = viewSize * asp;
    camera.top = viewSize;
    camera.bottom = -viewSize;
  } else {
    camera.left = -viewSize * asp + panX;
    camera.right = viewSize * asp + panX;
    camera.top = viewSize - panZ;
    camera.bottom = -viewSize - panZ;
  }
  camera.updateProjectionMatrix();
  redrawMinimap(); // keep minimap frustum rect in sync
  redrawCamZoneOverlay(); // keep zone overlay aligned with camera
}

function resetCamera() {
  const { tileSize, offsetX, offsetZ, width, height } = params;
  if (isFreeLook) {
    freeTarget.set(
      (width * tileSize) / 2 - offsetX,
      0,
      (height * tileSize) / 2 - offsetZ,
    );
    viewSize = Math.max(width * tileSize, height * tileSize) * 0.7;
    updateFreeLookCamera();
  } else {
    panX = (width * tileSize) / 2 - offsetX;
    panZ = (height * tileSize) / 2 - offsetZ;
    viewSize = Math.max(width * tileSize, height * tileSize) * 0.7;
    updateCameraFrustum();
  }
}

function setView(view) {
  // Switching to a fixed view always exits free-look
  if (isFreeLook) {
    isFreeLook = false;
    const flBtn = document.getElementById("btn-freelook");
    if (flBtn) {
      flBtn.classList.remove("active-btn");
      flBtn.textContent = "Free Look";
    }
  }

  currentView = view;
  document
    .querySelectorAll(".view-btn")
    .forEach((b) => b.classList.remove("active-btn"));
  document.getElementById("btn-view-" + view).classList.add("active-btn");
  document.getElementById("paint-warning").style.display =
    view === "top" ? "none" : "block";

  if (view === "top") {
    camera.position.set(0, 50, 0);
    camera.up.set(0, 0, -1);
  } else if (view === "front") {
    camera.position.set(0, 0, 50);
    camera.up.set(0, 1, 0);
  } else if (view === "back") {
    camera.position.set(0, 0, -50);
    camera.up.set(0, 1, 0);
  } else if (view === "left") {
    camera.position.set(-50, 0, 0);
    camera.up.set(0, 1, 0);
  } else if (view === "right") {
    camera.position.set(50, 0, 0);
    camera.up.set(0, 1, 0);
  }
  camera.lookAt(0, 0, 0);
  updateCameraFrustum();
}

/**
 * Free-look: orbit the camera around freeTarget using spherical coordinates
 */
function updateFreeLookCamera() {
  const sinPhi = Math.sin(freePhi);
  const cosPhi = Math.cos(freePhi);
  const sinTheta = Math.sin(freeTheta);
  const cosTheta = Math.cos(freeTheta);

  camera.position.set(
    freeTarget.x + FREE_RADIUS * sinPhi * sinTheta,
    freeTarget.y + FREE_RADIUS * cosPhi,
    freeTarget.z + FREE_RADIUS * sinPhi * cosTheta,
  );
  camera.up.set(0, 1, 0);
  camera.lookAt(freeTarget);
  updateCameraFrustum();
}

function toggleFreeLook() {
  isFreeLook = !isFreeLook;
  const btn = document.getElementById("btn-freelook");
  btn.classList.toggle("active-btn", isFreeLook);
  btn.textContent = isFreeLook ? "Free Look ✓" : "Free Look";

  if (isFreeLook) {
    // Deactivate all fixed-view buttons
    document
      .querySelectorAll(".view-btn")
      .forEach((b) => b.classList.remove("active-btn"));
    // Painting is allowed in free-look (via click), so hide the warning
    document.getElementById("paint-warning").style.display = "none";
    // Orbit around the current world centre
    const { tileSize, offsetX, offsetZ, width, height } = params;
    freeTarget.set(
      (width * tileSize) / 2 - offsetX,
      0,
      (height * tileSize) / 2 - offsetZ,
    );
    freeTheta = -Math.PI / 5;
    freePhi = Math.PI / 3.5;
    updateFreeLookCamera();
  } else {
    setView("top");
  }
}

// GRID
function rebuildGrid() {
  gridGroup.clear();
  const {
    tileSize: ts,
    offsetX: ox,
    offsetZ: oz,
    width: W,
    height: H,
  } = params;
  const x0 = -ox,
    x1 = W * ts - ox;
  const z0 = -oz,
    z1 = H * ts - oz;
  const Y = 0.03;

  const verts = [];
  for (let x = 0; x <= W; x++) {
    const wx = x * ts - ox;
    verts.push(wx, Y, z0, wx, Y, z1);
  }
  for (let z = 0; z <= H; z++) {
    const wz = z * ts - oz;
    verts.push(x0, Y, wz, x1, Y, wz);
  }
  const geo = new THREE.BufferGeometry();
  geo.setAttribute("position", new THREE.Float32BufferAttribute(verts, 3));
  gridGroup.add(
    new THREE.LineSegments(
      geo,
      new THREE.LineBasicMaterial({ color: 0x2a2a3a }),
    ),
  );

  const bv = [
    x0,
    Y + 0.01,
    z0,
    x1,
    Y + 0.01,
    z0,
    x1,
    Y + 0.01,
    z0,
    x1,
    Y + 0.01,
    z1,
    x1,
    Y + 0.01,
    z1,
    x0,
    Y + 0.01,
    z1,
    x0,
    Y + 0.01,
    z1,
    x0,
    Y + 0.01,
    z0,
  ];
  const bgeo = new THREE.BufferGeometry();
  bgeo.setAttribute("position", new THREE.Float32BufferAttribute(bv, 3));
  gridGroup.add(
    new THREE.LineSegments(
      bgeo,
      new THREE.LineBasicMaterial({ color: 0xf0a030 }),
    ),
  );

  gridGroup.visible = showGrid;
}

// TILE LAYER
function rebuildTileLayer() {
  tileGroup.clear();
  const {
    tileSize: ts,
    offsetX: ox,
    offsetZ: oz,
    width: W,
    height: H,
  } = params;

  tileCanvasEl = document.createElement("canvas");
  tileCanvasEl.width = W;
  tileCanvasEl.height = H;
  tileCanvasCtx = tileCanvasEl.getContext("2d");

  tileTex = new THREE.CanvasTexture(tileCanvasEl);
  tileTex.magFilter = THREE.NearestFilter;
  tileTex.minFilter = THREE.NearestFilter;
  tileTex.flipY = true; // default=true; keeps canvas Z→pixel-Y in sync with world Z

  const ww = W * ts,
    wh = H * ts;
  const geo = new THREE.PlaneGeometry(ww, wh);
  const mat = new THREE.MeshBasicMaterial({
    map: tileTex,
    transparent: true,
    opacity: 0.72,
    depthTest: false,
    depthWrite: false,
  });
  tilePlane = new THREE.Mesh(geo, mat);
  tilePlane.rotation.x = -Math.PI / 2;
  tilePlane.position.set(ww / 2 - ox, 0.04, wh / 2 - oz);
  tilePlane.renderOrder = 1;
  tileGroup.add(tilePlane);

  redrawTiles();
}

function redrawTiles() {
  if (!tileCanvasCtx) return;
  const { width: W, height: H } = params;
  const ctx = tileCanvasCtx;
  ctx.clearRect(0, 0, W, H);
  for (let z = 0; z < H; z++) {
    for (let x = 0; x < W; x++) {
      const type = tileData[z * W + x];
      if (type === "w") continue;
      const def = TILE_DEFS[type];
      if (!def) continue;
      ctx.fillStyle = def.border;
      ctx.fillRect(x, z, 1, 1);
    }
  }
  if (tileTex) tileTex.needsUpdate = true;
  redrawMinimap(); // keep minimap in sync whenever tiles change
}

// ─────────────────────────────────────────────────────────────────────────────
// CAMERA ZONE OVERLAY
// ─────────────────────────────────────────────────────────────────────────────

function initCamZoneOverlay() {
  if (camZoneOverlay) return;
  camZoneOverlay = document.createElement("canvas");
  camZoneOverlay.style.cssText =
    "position:absolute;inset:0;width:100%;height:100%;pointer-events:none;z-index:4";
  document.getElementById("viewport").appendChild(camZoneOverlay);
}

// World XZ → overlay canvas pixel (top-down only).
// In top view, screen-up = world -Z, so world Z range = -camera.top / -camera.bottom.
function worldToOverlay(wx, wz) {
  const minZ = -camera.top;
  const maxZ = -camera.bottom;
  const px = (wx - camera.left) / (camera.right - camera.left) * camZoneOverlay.width;
  const py = (wz - minZ) / (maxZ - minZ) * camZoneOverlay.height;
  return { x: px, y: py };
}

function tileToWorld(tx, tz) {
  const { tileSize: ts, offsetX: ox, offsetZ: oz } = params;
  return { wx: tx * ts - ox, wz: tz * ts - oz };
}

// Snaps to grid lines (not tile centers); used for corner-handle dragging.
function worldToGridLine(wx, wz) {
  const { tileSize: ts, offsetX: ox, offsetZ: oz } = params;
  return {
    tx: Math.round((wx + ox) / ts),
    tz: Math.round((wz + oz) / ts),
  };
}

// x2/z2 are inclusive tile indices; right/bottom grid lines sit one past them.
function defaultCornersFromRect(x1, z1, x2, z2) {
  return [
    { tx: x1,     tz: z1 },
    { tx: x2 + 1, tz: z1 },
    { tx: x2 + 1, tz: z2 + 1 },
    { tx: x1,     tz: z2 + 1 },
  ];
}

// Falls back to default rect corners for zones loaded from older .jmap files.
function zoneCorners(zone) {
  return zone.corners || defaultCornersFromRect(zone.x1, zone.z1, zone.x2, zone.z2);
}

// Keep bounding rect in sync after a corner drag.
function syncZoneBoundsFromCorners(zone) {
  const txs = zone.corners.map((c) => c.tx);
  const tzs = zone.corners.map((c) => c.tz);
  zone.x1 = Math.min(...txs);
  zone.x2 = Math.max(...txs) - 1;
  zone.z1 = Math.min(...tzs);
  zone.z2 = Math.max(...tzs) - 1;
}

function hasZoneCutouts(zone) {
  return zone.cutouts && zone.cutouts.length > 0;
}

// Returns the cutout index under the mouse, or -1.
function hitTestCutout(canvas, e, zone) {
  if (!zone.cutouts || !camZoneOverlay || !camera) return -1;
  const rect = canvas.getBoundingClientRect();
  const px = e.clientX - rect.left;
  const py = e.clientY - rect.top;
  for (let i = 0; i < zone.cutouts.length; i++) {
    const c = zone.cutouts[i];
    const tl = worldToOverlay(tileToWorld(c.x1, c.z1).wx, tileToWorld(c.x1, c.z1).wz);
    const br = worldToOverlay(tileToWorld(c.x2 + 1, c.z2 + 1).wx, tileToWorld(c.x2 + 1, c.z2 + 1).wz);
    const minX = Math.min(tl.x, br.x), maxX = Math.max(tl.x, br.x);
    const minY = Math.min(tl.y, br.y), maxY = Math.max(tl.y, br.y);
    if (px >= minX && px <= maxX && py >= minY && py <= maxY) return i;
  }
  return -1;
}

// Returns the corner handle index (0-3) under the mouse, or -1.
function hitTestZoneCorner(canvas, e, zone, radiusPx = 10) {
  if (!zone || !camZoneOverlay || !camera) return -1;
  const rect = canvas.getBoundingClientRect();
  const px = e.clientX - rect.left;
  const py = e.clientY - rect.top;
  const corners = zoneCorners(zone);
  for (let i = 0; i < corners.length; i++) {
    const w = tileToWorld(corners[i].tx, corners[i].tz);
    const sc = worldToOverlay(w.wx, w.wz);
    if (Math.hypot(px - sc.x, py - sc.y) < radiusPx) return i;
  }
  return -1;
}

function redrawCamZoneOverlay(previewZone) {
  if (!camZoneOverlay || !camera) return;
  const vp = document.getElementById("viewport");
  camZoneOverlay.width  = vp.clientWidth;
  camZoneOverlay.height = vp.clientHeight;
  const ctx = camZoneOverlay.getContext("2d");
  ctx.clearRect(0, 0, camZoneOverlay.width, camZoneOverlay.height);

  const { tileSize: ts, offsetX: ox, offsetZ: oz } = params;

  // Build a screen-space cutout rect path (grid-line snapped tile coords).
  const cutoutScreenRect = (c) => {
    const tl = worldToOverlay(tileToWorld(c.x1,     c.z1    ).wx, tileToWorld(c.x1,     c.z1    ).wz);
    const br = worldToOverlay(tileToWorld(c.x2 + 1, c.z2 + 1).wx, tileToWorld(c.x2 + 1, c.z2 + 1).wz);
    return { x: Math.min(tl.x, br.x), y: Math.min(tl.y, br.y),
             w: Math.abs(br.x - tl.x), h: Math.abs(br.y - tl.y) };
  };

  const drawZoneRect = (zone, isPreview, isSelected) => {
    const color = camZoneColor(zone.id);
    const corners = zoneCorners(zone);
    const screenPts = corners.map((c) => {
      const w = tileToWorld(c.tx, c.tz);
      return worldToOverlay(w.wx, w.wz);
    });
    const xs = screenPts.map((p) => p.x);
    const ys = screenPts.map((p) => p.y);
    const w = Math.max(...xs) - Math.min(...xs);
    const h = Math.max(...ys) - Math.min(...ys);
    if (w <= 0 || h <= 0) return;

    // evenodd fill: cutout rects wound opposite to outer path punch holes
    const buildPath = () => {
      ctx.beginPath();
      ctx.moveTo(screenPts[0].x, screenPts[0].y);
      for (let i = 1; i < screenPts.length; i++) ctx.lineTo(screenPts[i].x, screenPts[i].y);
      ctx.closePath();
      if (!isPreview && zone.cutouts) {
        for (const c of zone.cutouts) {
          const r = cutoutScreenRect(c);
          if (r.w > 0 && r.h > 0) {
              ctx.moveTo(r.x, r.y);
            ctx.lineTo(r.x, r.y + r.h);
            ctx.lineTo(r.x + r.w, r.y + r.h);
            ctx.lineTo(r.x + r.w, r.y);
            ctx.closePath();
          }
        }
      }
    };

    ctx.save();
    buildPath();
    ctx.globalAlpha = isPreview ? 0.25 : isSelected ? 0.30 : 0.18;
    ctx.fillStyle = color;
    ctx.fill("evenodd");

    buildPath();
    ctx.globalAlpha = isPreview ? 0.8 : isSelected ? 1.0 : 0.7;
    ctx.strokeStyle = color;
    ctx.lineWidth = isSelected ? 2.5 : 1.5;
    if (isPreview) ctx.setLineDash([4, 3]);
    ctx.stroke();
    ctx.setLineDash([]);

    if (!isPreview && isSelected && zone.cutouts) {
      for (let ci = 0; ci < zone.cutouts.length; ci++) {
        const r = cutoutScreenRect(zone.cutouts[ci]);
        if (r.w <= 0 || r.h <= 0) continue;
        ctx.globalAlpha = 0.7;
        ctx.strokeStyle = color;
        ctx.lineWidth = 1;
        ctx.setLineDash([3, 3]);
        ctx.strokeRect(r.x, r.y, r.w, r.h);
        ctx.setLineDash([]);
        const bx = r.x + r.w - 8, by = r.y + 2;
        ctx.globalAlpha = 0.85;
        ctx.fillStyle = color;
        ctx.font = "bold 10px monospace";
        ctx.textBaseline = "top";
        ctx.fillText("✕", bx, by);
      }
    }

    if (!isPreview && w > 20 && h > 14) {
      ctx.globalAlpha = 0.9;
      ctx.fillStyle = color;
      ctx.font = `bold 11px "BM Space", monospace`;
      ctx.textBaseline = "top";
      ctx.fillText(`cam_${zone.id}`, Math.min(...xs) + 4, Math.min(...ys) + 3);
    }
    ctx.restore();
  };

  const drawCornerHandles = (zone) => {
    const color = camZoneColor(zone.id);
    const corners = zoneCorners(zone);
    corners.forEach((c, i) => {
      const w = tileToWorld(c.tx, c.tz);
      const sc = worldToOverlay(w.wx, w.wz);
      const isDragging =
        camCornerDrag &&
        cameraZones[camCornerDrag.zoneIdx] === zone &&
        camCornerDrag.cornerIdx === i;
      const size = isDragging ? 7 : 5;
      ctx.save();
      ctx.fillStyle = isDragging ? "#ffffff" : color;
      ctx.strokeStyle = color;
      ctx.lineWidth = 1.5;
      ctx.fillRect(sc.x - size, sc.y - size, size * 2, size * 2);
      ctx.strokeRect(sc.x - size, sc.y - size, size * 2, size * 2);
      ctx.restore();
    });
  };

  const drawCamGizmo = (zone, isSelected, isDragging) => {
    if (zone.camWX == null) return;
    const color = camZoneColor(zone.id);
    const sc = worldToOverlay(zone.camWX, zone.camWZ);
    const R = isSelected ? 10 : 7;

    const tx1 = Math.min(zone.x1, zone.x2);
    const tz1 = Math.min(zone.z1, zone.z2);
    const tx2 = Math.max(zone.x1, zone.x2);
    const tz2 = Math.max(zone.z1, zone.z2);
    const centerWX = ((tx1 + tx2 + 1) / 2) * ts - ox;
    const centerWZ = ((tz1 + tz2 + 1) / 2) * ts - oz;
    const sc2 = worldToOverlay(centerWX, centerWZ);

    ctx.save();
    ctx.globalAlpha = isDragging ? 1.0 : isSelected ? 0.95 : 0.75;
    ctx.strokeStyle = color;
    ctx.lineWidth = 1.5;
    ctx.setLineDash([3, 3]);
    ctx.beginPath();
    ctx.moveTo(sc.x, sc.y);
    ctx.lineTo(sc2.x, sc2.y);
    ctx.stroke();
    ctx.setLineDash([]);

    ctx.lineWidth = isSelected ? 2 : 1.5;
    ctx.strokeStyle = color;
    ctx.fillStyle = color;
    ctx.globalAlpha = isDragging ? 0.5 : isSelected ? 0.35 : 0.2;
    ctx.beginPath();
    ctx.arc(sc.x, sc.y, R, 0, Math.PI * 2);
    ctx.fill();
    ctx.globalAlpha = isDragging ? 1.0 : isSelected ? 0.95 : 0.75;
    ctx.stroke();

    ctx.globalAlpha = isDragging ? 1.0 : isSelected ? 0.9 : 0.7;
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.moveTo(sc.x - R * 0.55, sc.y);
    ctx.lineTo(sc.x + R * 0.55, sc.y);
    ctx.moveTo(sc.x, sc.y - R * 0.55);
    ctx.lineTo(sc.x, sc.y + R * 0.55);
    ctx.stroke();

    if (isSelected || R > 6) {
      ctx.globalAlpha = 0.9;
      ctx.fillStyle = color;
      ctx.font = `bold 9px "BM Space", monospace`;
      ctx.textAlign = "center";
      ctx.textBaseline = "top";
      ctx.fillText(`cam_${zone.id}`, sc.x, sc.y + R + 2);
      ctx.textAlign = "left";
    }
    ctx.restore();
  };

  cameraZones.forEach((z, i) => {
    const isSel = i === camZoneSelected;
    drawZoneRect(z, false, isSel);
    drawCamGizmo(z, isSel, camGizmoDrag?.zoneIdx === i);
    if (isSel) drawCornerHandles(z);
  });
  if (previewZone) drawZoneRect(previewZone, true, false);

  if (camCutoutDraw) {
    const { startTX, startTZ, curTX, curTZ } = camCutoutDraw;
    const cx1 = Math.min(startTX, curTX), cx2 = Math.max(startTX, curTX);
    const cz1 = Math.min(startTZ, curTZ), cz2 = Math.max(startTZ, curTZ);
    const tl = worldToOverlay(tileToWorld(cx1, cz1).wx, tileToWorld(cx1, cz1).wz);
    const br = worldToOverlay(tileToWorld(cx2 + 1, cz2 + 1).wx, tileToWorld(cx2 + 1, cz2 + 1).wz);
    const rx = Math.min(tl.x, br.x), ry = Math.min(tl.y, br.y);
    const rw = Math.abs(br.x - tl.x), rh = Math.abs(br.y - tl.y);
    if (rw > 0 && rh > 0) {
      ctx.save();
      ctx.globalAlpha = 0.18;
      ctx.fillStyle = "#ff3333";
      ctx.fillRect(rx, ry, rw, rh);
      ctx.globalAlpha = 0.85;
      ctx.strokeStyle = "#ff3333";
      ctx.lineWidth = 1.5;
      ctx.setLineDash([4, 3]);
      ctx.strokeRect(rx, ry, rw, rh);
      ctx.setLineDash([]);
      ctx.restore();
    }
  }
}

function buildCamZoneList() {
  const el = document.getElementById("cam-zone-list");
  if (!el) return;
  el.innerHTML = "";

  if (cameraZones.length === 0) {
    el.innerHTML = '<div style="color:var(--dim);font-size:10px;padding:4px 0">No zones yet. Enable zone mode and drag to draw.</div>';
    return;
  }

  cameraZones.forEach((z, i) => {
    const color = camZoneColor(z.id);
    const isSel = i === camZoneSelected;

    // ── header row ──
    const card = document.createElement("div");
    card.style.cssText = `border:1px solid ${isSel ? color : "var(--border)"};
      border-radius:3px;margin-bottom:5px;overflow:hidden;`;

    const header = document.createElement("div");
    header.style.cssText = `display:flex;align-items:center;gap:6px;padding:4px 6px;
      background:${isSel ? color + "22" : "var(--panel2)"};cursor:pointer;`;

    const swatch = document.createElement("span");
    swatch.style.cssText = `width:10px;height:10px;border-radius:1px;flex-shrink:0;background:${color}`;

    const label = document.createElement("span");
    label.style.cssText = "flex:1;font-size:10px;color:var(--text);";
    const cutCount = z.cutouts?.length ?? 0;
    const shapeTag = cutCount > 0 ? `  [${cutCount} cutout${cutCount > 1 ? "s" : ""}]` : "";
    label.textContent = `cam_${z.id}  (${z.x1},${z.z1})→(${z.x2},${z.z2})${shapeTag}`;

    const del = document.createElement("button");
    del.textContent = "✕";
    del.style.cssText = "padding:1px 5px;font-size:9px;";
    del.title = "Delete zone";
    del.onclick = (e) => {
      e.stopPropagation();
      cameraZones.splice(i, 1);
      if (camZoneSelected === i) camZoneSelected = null;
      else if (camZoneSelected > i) camZoneSelected--;
      buildCamZoneList();
      redrawCamZoneOverlay();
    };

    header.appendChild(swatch);
    header.appendChild(label);
    header.appendChild(del);
    header.onclick = () => {
      camZoneSelected = isSel ? null : i;
      buildCamZoneList();
      redrawCamZoneOverlay();
    };

    card.appendChild(header);

    if (isSel) {
      const detail = document.createElement("div");
      detail.style.cssText = "padding:6px 8px;display:flex;flex-direction:column;gap:5px;background:var(--bg);";

      const resizeHint = document.createElement("div");
      resizeHint.style.cssText = "font-size:9px;color:var(--dim);line-height:1.4;";
      resizeHint.textContent = "Drag corner to resize · Shift+drag to carve cutout · Right-click ✕ to remove cutout";
      detail.appendChild(resizeHint);

      const camPos = document.createElement("div");
      camPos.style.cssText = "font-size:10px;color:var(--dim);";
      const posStr = z.camWX != null
        ? `${z.camWX.toFixed(3)}, ${z.camWZ.toFixed(3)}`
        : "not placed — use Place Camera";
      camPos.innerHTML = `<span style="color:${color}">&#9654; origin</span> ${posStr}`;
      detail.appendChild(camPos);

      const hRow = document.createElement("div");
      hRow.style.cssText = "display:flex;align-items:center;gap:6px;";
      const hLbl = document.createElement("label");
      hLbl.textContent = "height";
      hLbl.style.cssText = "font-size:10px;color:var(--dim);min-width:58px;";
      const hInp = document.createElement("input");
      hInp.type = "number"; hInp.step = "0.25"; hInp.value = z.height ?? 3.0;
      hInp.style.cssText = "width:70px;font-size:10px;";
      hInp.oninput = () => { cameraZones[i].height = parseFloat(hInp.value) || 3.0; };
      hRow.appendChild(hLbl); hRow.appendChild(hInp);
      detail.appendChild(hRow);

      const sRow = document.createElement("div");
      sRow.style.cssText = "display:flex;align-items:center;gap:6px;";
      const sLbl = document.createElement("label");
      sLbl.textContent = "smoothing";
      sLbl.style.cssText = "font-size:10px;color:var(--dim);min-width:58px;";
      const sInp = document.createElement("input");
      sInp.type = "number"; sInp.step = "0.01"; sInp.min = "0"; sInp.max = "1";
      sInp.value = z.smoothing ?? 0.1;
      sInp.style.cssText = "width:70px;font-size:10px;";
      sInp.oninput = () => { cameraZones[i].smoothing = parseFloat(sInp.value) ?? 0.1; };
      sRow.appendChild(sLbl); sRow.appendChild(sInp);
      detail.appendChild(sRow);

      const btnRow = document.createElement("div");
      btnRow.style.cssText = "display:flex;gap:5px;margin-top:2px;";

      const placeBtn = document.createElement("button");
      placeBtn.textContent = camGizmoMode ? "Place Camera ✓" : "Place Camera";
      placeBtn.style.cssText = "flex:1;font-size:9px;" + (camGizmoMode ? `border-color:${color};color:${color};` : "");
      placeBtn.onclick = () => {
      camGizmoMode = !camGizmoMode;
      if (camGizmoMode && camZoneMode) toggleCamZoneMode();
        buildCamZoneList();
        const canvas = document.getElementById("three-canvas");
        canvas.style.cursor = camGizmoMode ? "cell" : "crosshair";
      };
      btnRow.appendChild(placeBtn);

      const centerBtn = document.createElement("button");
      centerBtn.textContent = "Drop at Center";
      centerBtn.style.cssText = "flex:1;font-size:9px;";
      centerBtn.title = "Drop camera at zone center";
      centerBtn.onclick = () => {
        const tx1 = Math.min(z.x1, z.x2);
        const tz1 = Math.min(z.z1, z.z2);
        const tx2 = Math.max(z.x1, z.x2);
        const tz2 = Math.max(z.z1, z.z2);
        const { tileSize: ts, offsetX: ox, offsetZ: oz } = params;
        cameraZones[i].camWX = ((tx1 + tx2 + 1) / 2) * ts - ox;
        cameraZones[i].camWZ = ((tz1 + tz2 + 1) / 2) * ts - oz;
        camGizmoMode = false;
        document.getElementById("three-canvas").style.cursor = "crosshair";
        buildCamZoneList();
        redrawCamZoneOverlay();
      };
      btnRow.appendChild(centerBtn);

      if (z.camWX != null) {
        const clearBtn = document.createElement("button");
        clearBtn.textContent = "Clear Camera";
        clearBtn.style.cssText = "flex:1;font-size:9px;";
        clearBtn.onclick = () => {
          cameraZones[i].camWX = null;
          cameraZones[i].camWZ = null;
          buildCamZoneList();
          redrawCamZoneOverlay();
        };
        btnRow.appendChild(clearBtn);
      }
      detail.appendChild(btnRow);

      card.appendChild(detail);
    }

    el.appendChild(card);
  });
}

function toggleCamZoneMode() {
  camZoneMode = !camZoneMode;
  if (camZoneMode && camGizmoMode) {
  camGizmoMode = false;
  buildCamZoneList();
  }
  const btn = document.getElementById("btn-cam-zone-mode");
  btn.classList.toggle("active-btn", camZoneMode);
  btn.textContent = camZoneMode ? "Draw Zones ✓" : "Draw Zones";
  document.getElementById("three-canvas").style.cursor = camZoneMode ? "crosshair" : (isSpaceDown ? "grab" : "crosshair");
  if (!camZoneMode) camZoneDraw = null;
  redrawCamZoneOverlay();
}

// INPUT EVENTS
function setupEvents(canvas) {
  canvas.addEventListener(
    "wheel",
    (e) => {
      e.preventDefault();
      viewSize *= e.deltaY > 0 ? 1.08 : 0.92;
      viewSize = Math.max(0.03, Math.min(30, viewSize));
      // updateFreeLookCamera calls updateCameraFrustum internally
      if (isFreeLook) {
        updateFreeLookCamera();
      } else {
        updateCameraFrustum();
      }
    },
    { passive: false },
  );

  canvas.addEventListener("mousedown", (e) => {
    lastMX = e.clientX;
    lastMY = e.clientY;

    if (isFreeLook) {
      // Defer paint/orbit decision to mousemove & mouseup
      freeDragStartX = e.clientX;
      freeDragStartY = e.clientY;
      freeDragMoved = false;
      freeDragButton = e.button;
      paintErase = e.button === 2;
      return;
    }

    if (e.button === 1 || (e.button === 0 && isSpaceDown)) {
      isPanning = true;
      canvas.style.cursor = "grabbing";
      return;
    }

    if (currentView === "top") {
      const pt = getWorldPos(e);

      if (e.button === 0) {
        if (pt && camZoneOverlay) {
          const HIT_RADIUS_PX = 12;
          const rect = canvas.getBoundingClientRect();
          const px = e.clientX - rect.left;
          const py = e.clientY - rect.top;
          for (let i = 0; i < cameraZones.length; i++) {
            const z = cameraZones[i];
            if (z.camWX == null) continue;
            const sc = worldToOverlay(z.camWX, z.camWZ);
            if (Math.hypot(px - sc.x, py - sc.y) < HIT_RADIUS_PX) {
              pushZoneUndo();
              camGizmoDrag = { zoneIdx: i };
              camZoneSelected = i;
              buildCamZoneList();
              return;
            }
          }
        }

        if (camZoneSelected !== null) {
          const hitIdx = hitTestZoneCorner(canvas, e, cameraZones[camZoneSelected]);
          if (hitIdx !== -1) {
            pushZoneUndo();
            camCornerDrag = { zoneIdx: camZoneSelected, cornerIdx: hitIdx };
            return;
          }
        }

        // Must be checked BEFORE camZoneMode so shift never draws a new zone by accident.
        if (e.shiftKey && camZoneSelected !== null && pt) {
          const { tx, tz } = worldToTile(pt.x, pt.z);
          camCutoutDraw = { zoneIdx: camZoneSelected, startTX: tx, startTZ: tz, curTX: tx, curTZ: tz };
          return;
        }

        if (camGizmoMode && camZoneSelected !== null && pt) {
          cameraZones[camZoneSelected].camWX = pt.x;
          cameraZones[camZoneSelected].camWZ = pt.z;
          camGizmoMode = false;
          canvas.style.cursor = "crosshair";
          buildCamZoneList();
          redrawCamZoneOverlay();
          return;
        }

        if (camZoneMode) {
          const id = parseInt(document.getElementById("cam-zone-id").value) || 0;
          if (pt) {
            const { tx, tz } = worldToTile(pt.x, pt.z);
            camZoneDraw = { id, startTX: tx, startTZ: tz, curTX: tx, curTZ: tz };
          } else {
            // Fallback when raycaster misses
            const rect = canvas.getBoundingClientRect();
            const fx = (e.clientX - rect.left) / rect.width;
            const fy = (e.clientY - rect.top) / rect.height;
            const wx = camera.left + fx * (camera.right - camera.left);
            const wz = camera.top  + fy * (camera.bottom - camera.top);
            const { tx, tz } = worldToTile(wx, wz);
            camZoneDraw = { id, startTX: tx, startTZ: tz, curTX: tx, curTZ: tz };
          }
          return;
        }
      }

      if (e.button === 2 && camZoneSelected !== null && !camZoneMode) {
        const zone = cameraZones[camZoneSelected];
        const ci = hitTestCutout(canvas, e, zone);
        if (ci !== -1) {
          pushZoneUndo();
          zone.cutouts.splice(ci, 1);
          buildCamZoneList();
          redrawCamZoneOverlay();
          return;
        }
      }
    }

    if ((e.button === 0 || e.button === 2) && currentView === "top") {
      isPainting = true;
      paintErase = e.button === 2;
      beginStroke();
      paintAt(e);
    }
  });

  canvas.addEventListener("mousemove", (e) => {
    if (isFreeLook) {
      const dx = e.clientX - lastMX;
      const dy = e.clientY - lastMY;
      lastMX = e.clientX;
      lastMY = e.clientY;

      const totalDist = Math.hypot(
        e.clientX - freeDragStartX,
        e.clientY - freeDragStartY,
      );
      if (totalDist > 5) freeDragMoved = true;

      if (e.buttons > 0 && freeDragMoved) {
        if (isSpaceDown) {
          // Derive screen-right/up from spherical angles (camera matrix may not be current)
          const vp = document.getElementById("viewport");
          const panScale = (viewSize * 2) / vp.clientHeight;
          const lookDir = new THREE.Vector3(
            -Math.sin(freePhi) * Math.sin(freeTheta),
            -Math.cos(freePhi),
            -Math.sin(freePhi) * Math.cos(freeTheta),
          );
          const worldUp = new THREE.Vector3(0, 1, 0);
          const right = new THREE.Vector3()
            .crossVectors(lookDir, worldUp)
            .normalize();
          const up = new THREE.Vector3()
            .crossVectors(right, lookDir)
            .normalize();
          freeTarget.addScaledVector(right, -dx * panScale);
          freeTarget.addScaledVector(up, dy * panScale);
          updateFreeLookCamera();
        } else if (e.buttons === 1) {
          freeTheta -= dx * 0.007;
          freePhi -= dy * 0.007;
          freePhi = Math.max(0.05, Math.min(Math.PI * 0.95, freePhi));
          updateFreeLookCamera();
        }
      }
      hoverAt(e);
      return;
    }

    if (camGizmoDrag && currentView === "top") {
      const pt = getWorldPos(e);
      if (pt) {
        cameraZones[camGizmoDrag.zoneIdx].camWX = pt.x;
        cameraZones[camGizmoDrag.zoneIdx].camWZ = pt.z;
        redrawCamZoneOverlay();
        buildCamZoneList();
      }
      return;
    }

    if (camCornerDrag && currentView === "top") {
      const pt = getWorldPos(e);
      if (pt) {
        const zone = cameraZones[camCornerDrag.zoneIdx];
        if (zone) {
          if (!zone.corners) zone.corners = defaultCornersFromRect(zone.x1, zone.z1, zone.x2, zone.z2);
          const { tx, tz } = worldToGridLine(pt.x, pt.z);
          const i = camCornerDrag.cornerIdx;
          zone.corners[i].tx = tx;
          zone.corners[i].tz = tz;
          // Slide the two adjacent corners along the shared edge to keep rect.
          const { sameTZ, sameTX } = CORNER_NEIGHBORS[i];
          zone.corners[sameTZ].tz = tz;
          zone.corners[sameTX].tx = tx;
          syncZoneBoundsFromCorners(zone);
          redrawCamZoneOverlay();
          buildCamZoneList();
        }
      }
      hoverAt(e);
      return;
    }

    if (camCutoutDraw && currentView === "top") {
      const pt = getWorldPos(e);
      if (pt) {
        const { tx, tz } = worldToTile(pt.x, pt.z);
        camCutoutDraw.curTX = tx;
        camCutoutDraw.curTZ = tz;
        redrawCamZoneOverlay();
      }
      hoverAt(e);
      return;
    }

    if (camZoneDraw && currentView === "top") {
      const pt = getWorldPos(e);
      if (pt) {
        const { tx, tz } = worldToTile(pt.x, pt.z);
        camZoneDraw.curTX = tx;
        camZoneDraw.curTZ = tz;
        redrawCamZoneOverlay({
          id: camZoneDraw.id,
          x1: Math.min(camZoneDraw.startTX, tx), z1: Math.min(camZoneDraw.startTZ, tz),
          x2: Math.max(camZoneDraw.startTX, tx), z2: Math.max(camZoneDraw.startTZ, tz),
        });
      }
      hoverAt(e);
      return;
    }

    if (
      currentView === "top" &&
      !isPanning &&
      !isPainting &&
      camZoneSelected !== null &&
      !camZoneMode &&
      !camGizmoMode
    ) {
      const hitIdx = hitTestZoneCorner(canvas, e, cameraZones[camZoneSelected]);
      canvas.style.cursor = hitIdx !== -1 ? "move" : isSpaceDown ? "grab" : "crosshair";
    }

    if (isPanning) {
      const dx = e.clientX - lastMX;
      const dy = e.clientY - lastMY;
      lastMX = e.clientX;
      lastMY = e.clientY;
      const vp = document.getElementById("viewport");
      const asp = vp.clientWidth / vp.clientHeight;
      panX -= (dx / vp.clientWidth) * viewSize * asp * 1.5;
      panZ -= (dy / vp.clientHeight) * viewSize * 1.5;
      updateCameraFrustum();
    } else if (isPainting && currentView === "top") {
      paintAt(e);
    }
    hoverAt(e);
  });

  // Registered on window so it fires even if pointer leaves canvas
  window.addEventListener("mouseup", (e) => {
    if (isFreeLook) {
      const totalDist = Math.hypot(
        e.clientX - freeDragStartX,
        e.clientY - freeDragStartY,
      );
      if (
        !freeDragMoved &&
        totalDist <= 5 &&
        (freeDragButton === 0 || freeDragButton === 2)
      ) {
        beginStroke();
        paintAt({ clientX: freeDragStartX, clientY: freeDragStartY });
        endStroke();
      }
      freeDragMoved = false;
      return;
    }

    if (camGizmoDrag) {
      camGizmoDrag = null;
      buildCamZoneList();
      redrawCamZoneOverlay();
      return;
    }

    if (camCornerDrag) {
      camCornerDrag = null;
      buildCamZoneList();
      redrawCamZoneOverlay();
      return;
    }

    if (camCutoutDraw) {
      const { zoneIdx, startTX, startTZ, curTX, curTZ } = camCutoutDraw;
      camCutoutDraw = null;
      const zone = cameraZones[zoneIdx];
      if (zone) {
        const cx1 = Math.min(startTX, curTX), cx2 = Math.max(startTX, curTX);
        const cz1 = Math.min(startTZ, curTZ), cz2 = Math.max(startTZ, curTZ);
        if (cx2 >= cx1 && cz2 >= cz1) {
          pushZoneUndo();
          if (!zone.cutouts) zone.cutouts = [];
          zone.cutouts.push({ x1: cx1, z1: cz1, x2: cx2, z2: cz2 });
        }
      }
      buildCamZoneList();
      redrawCamZoneOverlay();
      return;
    }

    if (camZoneDraw) {
      const { id, startTX, startTZ, curTX, curTZ } = camZoneDraw;
      const x1 = Math.min(startTX, curTX);
      const z1 = Math.min(startTZ, curTZ);
      const x2 = Math.max(startTX, curTX);
      const z2 = Math.max(startTZ, curTZ);
      if (x2 >= x1 && z2 >= z1) {
        pushZoneUndo();
        cameraZones.push({
          id, x1, z1, x2, z2,
          camWX: null, camWZ: null,
          height: 3.0,
          smoothing: 0.1,
          corners: defaultCornersFromRect(x1, z1, x2, z2),
          cutouts: [],
        });
        buildCamZoneList();
      }
      camZoneDraw = null;
      redrawCamZoneOverlay();
      return;
    }

    endStroke();
    isPainting = false;
    isPanning = false;
    canvas.style.cursor = isSpaceDown ? "grab" : "crosshair";
  });

  canvas.addEventListener("contextmenu", (e) => e.preventDefault());

  // Keyboard
  window.addEventListener("keydown", (e) => {
    if (e.code === "Space" && !e.repeat) {
      isSpaceDown = true;
      canvas.style.cursor = "grab";
    }

    if (e.ctrlKey && e.key === "z") {
      e.preventDefault();
      undo();
      return;
    }
    if (e.ctrlKey && (e.key === "y" || (e.shiftKey && e.key === "Z"))) {
      e.preventDefault();
      redo();
      return;
    }

    for (const [k, def] of Object.entries(TILE_DEFS)) {
      if (e.key === def.key) {
        selectTool(k);
        return;
      }
    }
    if (e.key === "g" || e.key === "G") toggleGrid();
  });
  window.addEventListener("keyup", (e) => {
    if (e.code === "Space") {
      isSpaceDown = false;
      canvas.style.cursor = "crosshair";
    }
  });

  canvas.style.cursor = "crosshair";
}

// WORLD POSITION / TILE MAPPING
function getWorldPos(e) {
  const canvas = document.getElementById("three-canvas");
  const rect = canvas.getBoundingClientRect();
  const ndcX = ((e.clientX - rect.left) / rect.width) * 2 - 1;
  const ndcY = -((e.clientY - rect.top) / rect.height) * 2 + 1;
  groundRaycaster.setFromCamera(new THREE.Vector2(ndcX, ndcY), camera);
  const hits = groundRaycaster.intersectObject(groundMesh);
  return hits.length ? hits[0].point : null;
}

function worldToTile(wx, wz) {
  const {
    tileSize: ts,
    offsetX: ox,
    offsetZ: oz,
    width: W,
    height: H,
  } = params;
  const tx = Math.floor((wx + ox) / ts);
  const tz = Math.floor((wz + oz) / ts);
  return { tx, tz, valid: tx >= 0 && tx < W && tz >= 0 && tz < H };
}

function paintAt(e) {
  const pt = getWorldPos(e);
  if (!pt) return;
  const { tx, tz, valid } = worldToTile(pt.x, pt.z);
  if (!valid) return;
  const type = paintErase ? "w" : currentTool;
  const idx = tz * params.width + tx;
  if (tileData[idx] !== type) {
    tileData[idx] = type;
    redrawTiles();
  }
}

function hoverAt(e) {
  const pt = getWorldPos(e);
  if (!pt) {
    document.getElementById("s-tile").textContent = "—";
    document.getElementById("s-world").textContent = "—";
    document.getElementById("s-type").textContent = "—";
    return;
  }
  document.getElementById("s-world").textContent =
    `(${pt.x.toFixed(4)}, ${pt.z.toFixed(4)})`;
  const { tx, tz, valid } = worldToTile(pt.x, pt.z);
  if (!valid) {
    document.getElementById("s-tile").textContent = "out of bounds";
    document.getElementById("s-type").textContent = "—";
    return;
  }
  const type = tileData[tz * params.width + tx];
  document.getElementById("s-tile").textContent = `(${tx}, ${tz})`;
  document.getElementById("s-type").textContent =
    `${type} — ${TILE_DEFS[type]?.label || "?"}`;
}

// PALETTE UI

function selectTool(k) {
  if (!TILE_DEFS[k]) return;
  currentTool = k;
  document
    .querySelectorAll(".tile-btn")
    .forEach((b) => b.classList.remove("active"));
  const btn = document.getElementById(`tb-${k}`);
  if (btn) btn.classList.add("active");
  const def = TILE_DEFS[k];
  const indicator = document.getElementById("s-tool");
  indicator.textContent = k;
  indicator.style.background = def.border;
  indicator.style.color = def.bg;
}

// PARAMETERS
const paramInputs = document.querySelectorAll(
  "#p-offsetx, #p-offsetz, #p-width, #p-height, #p-scale, #p-center, #p-source-blender",
);
paramInputs.forEach((input) => {
  input.addEventListener("input", () =>
    document.getElementById("btn-apply").classList.add("needs-apply"),
  );
  input.addEventListener("change", () =>
    document.getElementById("btn-apply").classList.add("needs-apply"),
  );
});

function readParamInputs() {
  return {
    tileSize: parseFloat(document.getElementById("p-tilesize").value) || 0.0625,
    offsetX: parseFloat(document.getElementById("p-offsetx").value) || 0,
    offsetZ: parseFloat(document.getElementById("p-offsetz").value) || 0,
    width: Math.max(
      1,
      parseInt(document.getElementById("p-width").value) || 56,
    ),
    height: Math.max(
      1,
      parseInt(document.getElementById("p-height").value) || 12,
    ),
    name: currentJmapStem,
    scale: parseFloat(document.getElementById("p-scale").value) || 1.0,
    centered: document.getElementById("p-center").checked,
    source_blender: document.getElementById("p-source-blender").checked,
  };
}

function applyParams() {
  const p = readParamInputs();
  const newTiles = new Array(p.width * p.height).fill("w");
  const copyW = Math.min(p.width, params.width);
  const copyH = Math.min(p.height, params.height);
  for (let z = 0; z < copyH; z++) {
    for (let x = 0; x < copyW; x++) {
      newTiles[z * p.width + x] = tileData[z * params.width + x] || "w";
    }
  }

  const requiresTransformUpdate =
    p.scale !== params.scale ||
    p.centered !== params.centered ||
    p.source_blender !== params.source_blender;
  params = p;
  tileData = newTiles;

  undoStack = [];
  redoStack = [];
  strokeBefore = null;
  zoneUndoStack = [];
  zoneRedoStack = [];
  updateUndoRedoUI();

  rebuildGrid();
  rebuildTileLayer();
  updateMinimapSize(); // resize minimap canvas to new aspect ratio

  if (requiresTransformUpdate && rawModelGroup) {
    applyModelTransforms();
  }

  if (freeTarget) {
    freeTarget.set(
      (p.width * p.tileSize) / 2 - p.offsetX,
      0,
      (p.height * p.tileSize) / 2 - p.offsetZ,
    );
  }

  updateMapInfo();
  updateComputed();
  document.getElementById("btn-apply").classList.remove("needs-apply");
}

function autoSize() {
  const p = readParamInputs();
  const w = Math.round((p.offsetX * 2) / p.tileSize);
  const h = Math.round((p.offsetZ * 2) / p.tileSize);
  document.getElementById("p-width").value = w;
  document.getElementById("p-height").value = h;
}

function updateComputed() {
  const { tileSize: ts, width: W, height: H } = params;
  document.getElementById("world-computed").textContent =
    `${W}×${H} tiles  ·  ${(W * ts).toFixed(4)}×${(H * ts).toFixed(4)} world units`;
}

function updateMapInfo() {
  document.getElementById("map-info").innerHTML =
    `<strong>${params.name}</strong> · ${params.width}×${params.height}`;
}

// TOGGLES
function toggleGrid() {
  showGrid = !showGrid;
  gridGroup.visible = showGrid;
  const btn = document.getElementById("grid-btn");
  btn.textContent = `Grid: ${showGrid ? "ON" : "OFF"}`;
  btn.classList.toggle("active-btn", showGrid);
}
function toggleModel() {
  showModel = !showModel;
  modelGroup.visible = showModel;
  document.getElementById("model-btn").textContent =
    `Model: ${showModel ? "ON" : "OFF"}`;
}
function toggleWire() {
  showWire = !showWire;
  modelGroup.traverse((c) => {
    if (c.isMesh && c._isWire) c.visible = showWire;
  });
  document.getElementById("wire-btn").textContent =
    `Wire: ${showWire ? "ON" : "OFF"}`;
}
function toggleSolid() {
  showSolid = !showSolid;
  modelGroup.traverse((c) => {
    if (c.isMesh && c._isSolid) c.visible = showSolid;
  });
  document.getElementById("solid-btn").textContent =
    `Solid: ${showSolid ? "ON" : "OFF"}`;
}

function fillAll(type) {
  beginStroke(); // snapshot before fill
  tileData.fill(type);
  endStroke(); // commit to history if anything changed
  redrawTiles();
}

// MODEL LOADING
document.getElementById("obj-input").addEventListener("change", (e) => {
  const file = e.target.files[0];
  if (!file) return;
  const url = URL.createObjectURL(file);
  document.getElementById("no-model-hint").style.display = "none";

  const cacheReader = new FileReader();
  cacheReader.onload = (ev) => {
    try {
      localStorage.setItem("me_last_obj_text", ev.target.result);
      localStorage.setItem("me_last_obj_name", file.name);
    } catch (_) {}
  };
  cacheReader.readAsText(file);

  new THREE.OBJLoader().load(
    url,
    (obj) => {
      rawModelGroup = obj;
      applyModelTransforms();
      URL.revokeObjectURL(url);
      e.target.value = "";
    },
    undefined,
    (err) => {
      alert("Failed to load .obj: " + err.message);
      URL.revokeObjectURL(url);
    },
  );
});

function applyModelTransforms() {
  modelGroup.clear();

  // Blender Z-up → NDS Y-up rotation (det=+1, winding preserved):
  //   x'=x, y'=z, z'=-y
  const blender = params.source_blender;
  const swapMat = blender
    ? new THREE.Matrix4().set(1, 0, 0, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1)
    : null;

  rawModelGroup.traverse((child) => {
    if (!child.isMesh) return;
    const geo = blender
      ? child.geometry.clone().applyMatrix4(swapMat)
      : child.geometry;
    const wire = new THREE.Mesh(geo, wireMaterial.clone());
    const solid = new THREE.Mesh(geo, solidMaterial.clone());
    wire._isWire = true;
    solid._isSolid = true;
    wire.visible = showWire;
    solid.visible = showSolid;
    modelGroup.add(wire);
    modelGroup.add(solid);
  });

  const s = params.scale;
  modelGroup.scale.set(s, s, s);

  if (params.centered) {
    // Mirrors Python's compute_bounds() after convert_blender_zup.
    // Read raw attributes so modelGroup.scale isn't factored in.
    const bbox = new THREE.Box3();
    modelGroup.traverse((c) => {
      if (c.isMesh && c._isWire) {
        const pos = c.geometry.attributes.position;
        for (let i = 0; i < pos.count; i++) {
          bbox.expandByPoint(
            new THREE.Vector3(pos.getX(i), pos.getY(i), pos.getZ(i)),
          );
        }
      }
    });
    modelGroup.position.set(
      (-(bbox.min.x + bbox.max.x) / 2) * s,
      -bbox.min.y * s,
      (-(bbox.min.z + bbox.max.z) / 2) * s,
    );
  } else {
    modelGroup.position.set(0, 0, 0);
  }
}

function matchDefine(text, pattern) {
  const m = text.match(pattern);
  return m ? parseFloat(m[1]) : null;
}

// JMAP PARSING
document.getElementById("jmap-input").addEventListener("change", (e) => {
  const file = e.target.files[0];
  if (!file) return;
  const reader = new FileReader();
  reader.onload = (ev) => {
    const text = ev.target.result;
    currentJmapStem = file.name.replace(/\.jmap$/i, "");
    parseJmap(text);
    e.target.value = "";
    try { localStorage.setItem("me_last_jmap_text", text); localStorage.setItem("me_last_jmap_name", file.name); } catch (_) {}
  };
  reader.readAsText(file);
});

function parseJmap(text) {
  const lines = text.split("\n");
  const dataLines = [];
  const newCamZones = [];
  (w = 0), (h = 0);
  let inCamZones = false;

  for (const line of lines) {
    const t = line.trim();

    if (t === "[CAMERA_ZONES]") {
      inCamZones = true;
      continue;
    }
    if (t.startsWith("[") && t.endsWith("]") && t !== "[CAMERA_ZONES]") {
      inCamZones = false;
      continue;
    }

    if (inCamZones) {
      if (!t || t.startsWith("#")) continue;
      const parts = t.split(",").map((s) => s.trim());
      if (parts.length >= 5) {
        const [
          id, x1, z1, x2, z2, wxRaw, wzRaw, hRaw, smRaw,
          tlxRaw, tlzRaw, trxRaw, trzRaw, brxRaw, brzRaw, blxRaw, blzRaw,
        ] = parts;
        const camWX = wxRaw === "null" || wxRaw == null ? null : parseFloat(wxRaw);
        const camWZ = wzRaw === "null" || wzRaw == null ? null : parseFloat(wzRaw);
        const px1 = parseInt(x1), pz1 = parseInt(z1), px2 = parseInt(x2), pz2 = parseInt(z2);
        const hasCorners = blzRaw != null && blzRaw !== "";
        let cutouts = [];
        const cutoutMarker = t.indexOf("cutouts:");
        if (cutoutMarker !== -1) {
          const cutoutData = t.slice(cutoutMarker + 8).trim();
          for (const entry of cutoutData.split("|")) {
            const nums = entry.trim().split(/\s+/).map(Number);
            if (nums.length >= 4 && nums.every(isFinite)) {
              cutouts.push({ x1: nums[0], z1: nums[1], x2: nums[2], z2: nums[3] });
            }
          }
        }
        newCamZones.push({
          id: parseInt(id),
          x1: px1, z1: pz1,
          x2: px2, z2: pz2,
          camWX, camWZ,
          height:    hRaw  != null ? parseFloat(hRaw)  : 3.0,
          smoothing: smRaw != null ? parseFloat(smRaw) : 0.1,
          corners: hasCorners
            ? [
                { tx: parseFloat(tlxRaw), tz: parseFloat(tlzRaw) },
                { tx: parseFloat(trxRaw), tz: parseFloat(trzRaw) },
                { tx: parseFloat(brxRaw), tz: parseFloat(brzRaw) },
                { tx: parseFloat(blxRaw), tz: parseFloat(blzRaw) },
              ]
            : defaultCornersFromRect(px1, pz1, px2, pz2),
          cutouts,
        });
      }
      continue;
    }

    if (!t || t.startsWith("#")) {
      const sizeM = t.match(/(\d+)x(\d+)/);
      if (sizeM) {
        w = parseInt(sizeM[1]);
        h = parseInt(sizeM[2]);
      }

      const tileMatch = t.match(
        /#\s+([a-z0-9])\s*=\s*([^()]+)\s*(?:\(([^)]+)\))?/i,
      );

      if (tileMatch && tileMatch[1] !== "w") {
        const code = tileMatch[1];
        if (!TILE_DEFS[code]) {
          const parsedLabel = tileMatch[3] || tileMatch[2].trim();
          const baseColor = generateDeterministicColor(code + parsedLabel);
          TILE_DEFS[code] = {
            label: parsedLabel,
            shortLabel: tileMatch[2].trim(),
            key: "",
            bg: baseColor + "40",
            border: baseColor,
            textColor: "#ffffff",
          };
          if (TILE_CATEGORIES.length > 0) {
            TILE_CATEGORIES[0].tiles.push(code);
          }
        }
      }
      continue;
    }
    dataLines.push(t.split(",").map((s) => s.trim()));
  }

  if (dataLines.length === 0) {
    alert("No tile data found in .jmap");
    return;
  }
  const actualH = dataLines.length;
  const actualW = dataLines[0].length;

  document.getElementById("p-width").value = actualW;
  document.getElementById("p-height").value = actualH;

  applyParams();
  buildPalette();

  for (let z = 0; z < actualH; z++) {
    for (let x = 0; x < actualW; x++) {
      const type = dataLines[z][x] || "w";
      if (TILE_DEFS[type] || type === "w") tileData[z * actualW + x] = type;
    }
  }

  // Apply camera zones
  cameraZones = newCamZones;
  buildCamZoneList();
  redrawTiles();
  redrawCamZoneOverlay();
}

// EXPORT
function exportJmap() {
  const { width: W, height: H, name } = params;
  const lines = [];
  lines.push(`# ${name} collision map  ${W}x${H}`);
  lines.push(`#`);
  lines.push(`#`);
  lines.push(`# Tile key:`);
  for (const [k, def] of Object.entries(TILE_DEFS)) {
    lines.push(`#   ${k} = ${def.shortLabel} (${def.label})`);
  }
  lines.push(``);
  for (let z = 0; z < H; z++) {
    const row = [];
    for (let x = 0; x < W; x++) row.push(tileData[z * W + x] || "w");
    lines.push(row.join(", "));
  }

  // Camera zones section
  if (cameraZones.length > 0) {
    lines.push(``);
    lines.push(`[CAMERA_ZONES]`);
    lines.push(`# id, x1, z1, x2, z2, cam_wx, cam_wz, height, smoothing, tlx, tlz, trx, trz, brx, brz, blx, blz [, cutouts: cx1 cz1 cx2 cz2 ...]`);
    for (const z of cameraZones) {
      const wx = z.camWX != null ? z.camWX.toFixed(4) : "null";
      const wz = z.camWZ != null ? z.camWZ.toFixed(4) : "null";
      const h  = (z.height   ?? 3.0).toFixed(4);
      const sm = (z.smoothing ?? 0.1).toFixed(4);
      const corners = zoneCorners(z);
      const cornerStr = corners.map((c) => `${c.tx}, ${c.tz}`).join(", ");
      let cutoutStr = "";
      if (z.cutouts && z.cutouts.length > 0) {
        cutoutStr = ", cutouts: " + z.cutouts.map((c) => `${c.x1} ${c.z1} ${c.x2} ${c.z2}`).join(" | ");
      }
      lines.push(`${z.id}, ${z.x1}, ${z.z1}, ${z.x2}, ${z.z2}, ${wx}, ${wz}, ${h}, ${sm}, ${cornerStr}${cutoutStr}`);
    }
  }

  downloadFile(`${name}.jmap`, lines.join("\n"), "text/plain");
}

function downloadFile(filename, text, mime) {
  const a = document.createElement("a");
  a.href = URL.createObjectURL(new Blob([text], { type: mime }));
  a.download = filename;
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
}

// UNDO / REDO
// Snapshot before any tile-changing operation (call at start of stroke/fill).
function beginStroke() {
  strokeBefore = [...tileData];
}

// Commit snapshot to undo stack if anything changed (call at end of stroke/fill).
function endStroke() {
  if (!strokeBefore) return;
  let changed = false;
  for (let i = 0; i < tileData.length; i++) {
    if (tileData[i] !== strokeBefore[i]) {
      changed = true;
      break;
    }
  }
  if (changed) {
    undoStack.push(strokeBefore);
    redoStack = [];
    if (undoStack.length > 50) undoStack.shift();
    updateUndoRedoUI();
  }
  strokeBefore = null;
}

function snapshotZones() {
  return JSON.parse(JSON.stringify(cameraZones));
}

function pushZoneUndo() {
  zoneUndoStack.push(snapshotZones());
  zoneRedoStack = [];
  if (zoneUndoStack.length > 50) zoneUndoStack.shift();
  updateUndoRedoUI();
}

function undo() {
  if (zoneUndoStack.length > 0) {
    zoneRedoStack.push(snapshotZones());
    cameraZones = zoneUndoStack.pop();
    buildCamZoneList();
    redrawCamZoneOverlay();
    updateUndoRedoUI();
    return;
  }
  if (undoStack.length === 0) return;
  redoStack.push([...tileData]);
  tileData = undoStack.pop();
  redrawTiles();
  updateUndoRedoUI();
}

function redo() {
  if (zoneRedoStack.length > 0) {
    zoneUndoStack.push(snapshotZones());
    cameraZones = zoneRedoStack.pop();
    buildCamZoneList();
    redrawCamZoneOverlay();
    updateUndoRedoUI();
    return;
  }
  if (redoStack.length === 0) return;
  undoStack.push([...tileData]);
  tileData = redoStack.pop();
  redrawTiles();
  updateUndoRedoUI();
}

function updateUndoRedoUI() {
  const u = document.getElementById("btn-undo");
  const r = document.getElementById("btn-redo");
  if (u) u.disabled = undoStack.length === 0 && zoneUndoStack.length === 0;
  if (r) r.disabled = redoStack.length === 0 && zoneRedoStack.length === 0;
}

// MINIMAP
function initMinimap() {
  minimapCanvas = document.getElementById("minimap");
  minimapCtx = minimapCanvas.getContext("2d");
  updateMinimapSize();

  minimapCanvas.addEventListener("click", (e) => {
    if (currentView !== "top" || isFreeLook) return;
    const rect = minimapCanvas.getBoundingClientRect();
    const mx = (e.clientX - rect.left) / rect.width;
    const my = (e.clientY - rect.top) / rect.height;
    const {
      width: W,
      height: H,
      tileSize: ts,
      offsetX: ox,
      offsetZ: oz,
    } = params;
    panX = mx * W * ts - ox;
    panZ = my * H * ts - oz;
    updateCameraFrustum();
  });
}

// Resize minimap to match map aspect ratio, capped at 160×100px.
function updateMinimapSize() {
  if (!minimapCanvas) return;
  const { width: W, height: H } = params;
  const aspect = W / H;
  const MAX_W = 160,
    MAX_H = 100;
  let mw, mh;
  if (aspect >= MAX_W / MAX_H) {
    mw = MAX_W;
    mh = Math.max(4, Math.round(MAX_W / aspect));
  } else {
    mh = MAX_H;
    mw = Math.max(4, Math.round(MAX_H * aspect));
  }
  minimapCanvas.width = mw;
  minimapCanvas.height = mh;
  minimapCanvas.style.width = mw + "px";
  minimapCanvas.style.height = mh + "px";
  minimapCtx = minimapCanvas.getContext("2d");
  redrawMinimap();
}

function redrawMinimap() {
  if (!minimapCtx || !minimapCanvas) return;
  const {
    width: W,
    height: H,
    tileSize: ts,
    offsetX: ox,
    offsetZ: oz,
  } = params;
  const mw = minimapCanvas.width;
  const mh = minimapCanvas.height;
  const cellX = mw / W;
  const cellY = mh / H;

  minimapCtx.fillStyle = "#0d0d0f";
  minimapCtx.fillRect(0, 0, mw, mh);

  for (let z = 0; z < H; z++) {
    for (let x = 0; x < W; x++) {
      const type = tileData[z * W + x];
      if (type === "w") {
        minimapCtx.fillStyle = "#1a1b2a";
      } else {
        const def = TILE_DEFS[type];
        if (!def) continue;
        minimapCtx.fillStyle = def.border;
      }
      minimapCtx.fillRect(
        Math.floor(x * cellX),
        Math.floor(z * cellY),
        Math.max(1, Math.ceil(cellX)),
        Math.max(1, Math.ceil(cellY)),
      );
    }
  }

  if (currentView === "top" && !isFreeLook && camera) {
    const wx0 = camera.left;
    const wx1 = camera.right;
    const wz0 = -camera.top;
    const wz1 = -camera.bottom;

    const tx0 = (wx0 + ox) / ts;
    const tx1 = (wx1 + ox) / ts;
    const tz0 = (wz0 + oz) / ts;
    const tz1 = (wz1 + oz) / ts;

    const px0 = tx0 * cellX;
    const py0 = tz0 * cellY;
    const pw = (tx1 - tx0) * cellX;
    const ph = (tz1 - tz0) * cellY;

    minimapCtx.strokeStyle = "#f0a030";
    minimapCtx.lineWidth = 1.5;
    minimapCtx.strokeRect(
      Math.round(px0) + 0.5,
      Math.round(py0) + 0.5,
      Math.round(pw),
      Math.round(ph),
    );
  }
}

// TOOLTIPS
function initTooltips() {
  const tip = document.getElementById("tooltip");
  if (!tip) return;

  document.body.addEventListener("mouseover", (e) => {
    const el = e.target.closest("[data-tip]");
    if (el) {
      tip.textContent = el.dataset.tip;
      tip.style.display = "block";
    } else {
      tip.style.display = "none";
    }
  });

  document.body.addEventListener("mousemove", (e) => {
    if (tip.style.display === "none") return;
    tip.style.left = Math.min(e.clientX + 14, window.innerWidth - 240) + "px";
    tip.style.top = Math.min(e.clientY + 18, window.innerHeight - 52) + "px";
  });
}

async function init() {
  await loadTileDefinitions();
  buildCategorySelect();
  buildPalette();

  if (TILE_DEFS["w"]) {
    selectTool("w");
  } else {
    const firstTool = Object.keys(TILE_DEFS)[0];
    if (firstTool) selectTool(firstTool);
  }

  updateComputed();
  updateMapInfo();
  initThree();
  initMinimap();
  initCamZoneOverlay();
  buildCamZoneList();

  const camIdSel = document.getElementById("cam-zone-id");
  const camIdSwatch = document.getElementById("cam-zone-id-swatch");
  if (camIdSel && camIdSwatch) {
    camIdSel.addEventListener("change", () => {
      camIdSwatch.style.background = camZoneColor(parseInt(camIdSel.value));
    });
  }

  initTooltips();
  // Restore last session; try/catch so corrupt localStorage can't prevent boot
  try {
    const _lastJmap = localStorage.getItem("me_last_jmap_text");
    const _lastJmapName = localStorage.getItem("me_last_jmap_name");
    if (_lastJmap) {
      if (_lastJmapName) currentJmapStem = _lastJmapName.replace(/\.jmap$/i, "");
      parseJmap(_lastJmap);
    }
  } catch (e) {
    console.warn("Failed to restore last jmap from localStorage:", e);
    localStorage.removeItem("me_last_jmap_text");
    localStorage.removeItem("me_last_jmap_name");
  }
  try {
    const _lastObj = localStorage.getItem("me_last_obj_text");
    if (_lastObj) loadObjFromText(_lastObj);
  } catch (e) {
    console.warn("Failed to restore last obj:", e);
    localStorage.removeItem("me_last_obj_text");
    localStorage.removeItem("me_last_obj_name");
  }
}

function loadObjFromText(objText) {
  const blob = new Blob([objText], { type: "text/plain" });
  const url = URL.createObjectURL(blob);
  document.getElementById("no-model-hint").style.display = "none";
  new THREE.OBJLoader().load(
    url,
    (obj) => { rawModelGroup = obj; applyModelTransforms(); URL.revokeObjectURL(url); },
    undefined,
    (err) => { console.warn("Failed to reload .obj:", err); URL.revokeObjectURL(url); }
  );
}

init();
