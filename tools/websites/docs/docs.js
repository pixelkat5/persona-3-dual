/*
   docs.js  —  powers /docs/index.html
   - No ?doc param  → fetch manifest.json, render card listing
   - ?doc=file.md   → fetch that .md, render with marked
 */

const app = document.getElementById("app");
const params = new URLSearchParams(location.search);
const docParam = params.get("doc");

/*  Marked config  */
marked.setOptions({
  gfm: true,
  breaks: false,
});

/*  Router  */
if (docParam) {
  loadDoc(docParam);
} else {
  loadIndex();
}

/*  Index (card listing)  */
async function loadIndex() {
  let manifest;

  try {
    const res = await fetch("/docs/manifest.json");
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    manifest = await res.json();
  } catch (err) {
    renderError(
      "Could not load docs manifest.",
      `Make sure <code>manifest.json</code> exists in the docs folder. (${err.message})`,
    );
    return;
  }

  const docs = manifest.docs ?? [];

  // Group by optional `section` key
  const sections = {};
  for (const doc of docs) {
    const key = doc.section ?? "General";
    (sections[key] ??= []).push(doc);
  }

  // Build card HTML
  const sectionsHtml = Object.entries(sections)
    .map(
      ([title, items]) => `
        <div class="section">
            <div class="section-title">${escHtml(title)}</div>
            <div class="card-list">
                ${items.map(docCard).join("")}
            </div>
        </div>
    `,
    )
    .join("");

  app.innerHTML = `
        <div class="page-hero">
            <h1>${escHtml(manifest.title ?? "Documentation")}</h1>
            <p class="page-desc">${escHtml(manifest.desc ?? "Guides and references for The P3 Dual project.")}</p>
        </div>
        ${docs.length > 0 ? sectionsHtml : '<p class="page-desc">No docs found in manifest.json.</p>'}
    `;

  document.title = `${manifest.title ?? "Docs"} — The P3 Dual Project`;
}

function docCard(doc) {
  // ?doc= is relative — just the filename, resolved from /docs/
  const href = `?doc=${encodeURIComponent(doc.file)}`;
  const tag = doc.tag
    ? `<span class="card-tag">${escHtml(doc.tag)}</span>`
    : "";

  return `
        <a class="card" href="${href}">
            <div class="card-body">
                <div class="card-title">${escHtml(doc.title)}</div>
                <div class="card-desc">
                    ${escHtml(doc.desc ?? "")}
                    ${tag}
                </div>
            </div>
            <span class="card-arrow">
                <svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24">
                    <path d="M5 12h14M12 5l7 7-7 7"/>
                </svg>
            </span>
        </a>
    `;
}

/*  Single doc view  */
async function loadDoc(file) {
  // Safety: only allow simple filenames/paths, no traversal, must end in .md
  if (!/^[\w][\w\-./ ]*\.md$/i.test(file) || file.includes("..")) {
    renderError(
      "Invalid document path.",
      "The requested file path is not allowed.",
    );
    return;
  }

  let text;
  try {
    // Relative fetch — resolves to /docs/<file> since the page lives at /docs/
    const res = await fetch(`/docs/${file}`);
    if (!res.ok)
      throw new Error(`HTTP ${res.status} — is the file in the docs folder?`);
    text = await res.text();
  } catch (err) {
    renderError(`Could not load "${escHtml(file)}"`, err.message);
    return;
  }

  // Pull title from first h1 for the browser tab
  const titleMatch = text.match(/^#\s+(.+)/m);
  const pageTitle = titleMatch ? titleMatch[1].replace(/[*_`]/g, "") : file;
  document.title = `${pageTitle} — P3 Dual`;

  const html = marked.parse(text);

  app.innerHTML = `
        <div class="page-hero" style="margin-bottom:32px; padding-bottom:24px; border-bottom: 1px solid var(--border);">
            <nav class="doc-breadcrumb" style="margin-bottom:0; padding-bottom:0; border:none;">
                <a href="?">← All Docs</a>
            </nav>
        </div>
        <article class="doc-content">${html}</article>
    `;
}

/*  Error state  */
function renderError(title, detail) {
  app.innerHTML = `
        <div class="doc-error">
            <div class="doc-error-box">
                <strong>${escHtml(title)}</strong>
                <span>${detail}</span>
            </div>
        </div>
    `;
}

/*  Helpers  */
function escHtml(str) {
  return String(str)
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;");
}
