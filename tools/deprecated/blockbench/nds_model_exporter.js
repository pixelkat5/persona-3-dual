(function () {
  var exportAction;

  Plugin.register("nds_model_exporter", {
    title: "NDS Model Exporter for Blockbench",
    author: "P3D Team",
    description:
      "Automates exporting Hierarchical JSON and isolated OBJs to a ZIP file. Auto-detects texture size. Built for the Persona 3 Dual project.",
    version: "1.0.8",
    variant: "both",

    onload() {
      exportAction = new Action("export_nds_model", {
        name: "Export NDS Model (ZIP)",
        description: "Exports model parts and animation data to a ZIP file.",
        icon: "archive",
        click: function () {
          runExportPipeline();
        },
      });
      MenuBar.addAction(exportAction, "file.export");
    },
    onunload() {
      exportAction.delete();
    },
  });

  function runExportPipeline() {
    // grab the base name of the project
    let modelName = (Project.name || "model").replace(/[^a-zA-Z0-9]/g, "_");

    let zip = new window.JSZip();
    let dsJson = { nodes: [], animations: {} };

    let groupMap = new Map();
    let idCounter = 0;
    Group.all.forEach((group) => groupMap.set(group.uuid, idCounter++));

    let allGeometry = [];
    if (typeof Cube !== "undefined") allGeometry = allGeometry.concat(Cube.all);
    if (typeof Mesh !== "undefined") allGeometry = allGeometry.concat(Mesh.all);

    let originalExportStates = new Map();
    allGeometry.forEach((el) =>
      originalExportStates.set(el.uuid, el.export !== false),
    );

    Group.all.forEach((group) => {
      let id = groupMap.get(group.uuid);
      let parentId =
        group.parent instanceof Group ? groupMap.get(group.parent.uuid) : -1;
      let objFileName = `${modelName}_n${id}.obj`;

      dsJson.nodes.push({
        id: id,
        parent: parentId,
        name: group.name,
        obj: objFileName,
        origin: group.origin,
      });

      allGeometry.forEach((el) => (el.export = false));
      let hasGeometry = false;
      group.children.forEach((child) => {
        if (child.type === "cube" || child.type === "mesh") {
          child.export = true;
          hasGeometry = true;
        }
      });

      let objContent = hasGeometry ? Codecs.obj.compile() : "# Empty Node\n";
      zip.file(objFileName, objContent);
    });

    allGeometry.forEach((el) => {
      if (originalExportStates.has(el.uuid))
        el.export = originalExportStates.get(el.uuid);
    });

    Project.animations.forEach((anim) => {
      let animName = anim.name.replace(/[^a-zA-Z0-9]/g, "_");
      let durationFrames = Math.round(anim.length * 60);
      let animData = { duration: durationFrames, tracks: {} };

      Object.keys(anim.animators).forEach((uuid) => {
        let animator = anim.animators[uuid];
        if (!groupMap.has(uuid)) return;
        let groupId = groupMap.get(uuid);
        let track = [];

        if (animator.keyframes && animator.keyframes.length > 0) {
          animator.keyframes.forEach((kf) => {
            let frameTime = Math.round(kf.time * 60);
            let rx = kf.channel === "rotation" ? kf.calc("x") : 0;
            let ry = kf.channel === "rotation" ? kf.calc("y") : 0;
            let rz = kf.channel === "rotation" ? kf.calc("z") : 0;
            let px = kf.channel === "position" ? kf.calc("x") : 0;
            let py = kf.channel === "position" ? kf.calc("y") : 0;
            let pz = kf.channel === "position" ? kf.calc("z") : 0;

            let existing = track.find((t) => t.time === frameTime);
            if (existing) {
              if (kf.channel === "rotation") existing.rot = [rx, ry, rz];
              if (kf.channel === "position") existing.pos = [px, py, pz];
            } else {
              track.push({
                time: frameTime,
                rot: kf.channel === "rotation" ? [rx, ry, rz] : [0, 0, 0],
                pos: kf.channel === "position" ? [px, py, pz] : [0, 0, 0],
              });
            }
          });
          track.sort((a, b) => a.time - b.time);
          if (track.length > 0) animData.tracks[groupId] = track;
        }
      });
      dsJson.animations[animName] = animData;
    });

    let jsonString = JSON.stringify(dsJson, null, 2);
    zip.file(`${modelName}.json`, jsonString);

    zip.generateAsync({ type: "blob" }).then((content) => {
      Blockbench.export(
        {
          type: "Zip Archive",
          extensions: ["zip"],
          name: `${modelName}`,
          content: content,
          savetype: "zip",
        },
        () => {
          Blockbench.showQuickMessage(`Exported ${modelName}.zip`);
        },
      );
    });
  }
})();
