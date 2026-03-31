# File type associations (registry schema)

This document defines how **file types**, **open-with handlers**, and **icons** are stored in OmniShell’s registry (`IRegistry` / `LocalRegistry`). Values are ordinary registry string leaves; complex structures use **JSON** as the string payload (UTF-8). On disk, each leaf is still a `.json` file whose body is a JSON **string literal** wrapping that payload (see [README](README.md) — `LocalRegistry`).

**Root namespace (recommended):** `OmniShell.FileTypes`

Use **hyphenated** segment names below so keys stay readable; they map to directories / filenames under `registry/OmniShell/FileTypes/…`.

---

## Layout overview

| Logical node | Registry role | Value shape |
|--------------|---------------|-------------|
| `extension` | Map **extension → file type** | One leaf per extension |
| `folder` | Default type for **directories** | Single **object** |
| `mime-type` | Map **MIME id → file type** | One leaf per MIME type |
| `path` | Ordered **path rules** | **Array** of rule objects |
| `path-regex` | Ordered **regex rules** | **Array** of rule objects |
| `filename` | Ordered **exact filename** rules | **Array** of rule objects |
| `filename-glob` | Ordered **glob** rules | **Array** of rule objects |

- **Object-valued** maps (`extension`, `mime-type`): each leaf holds one **file type** JSON object (`openwith` + `icon`).
- **Single object** (`folder`): one leaf holds one file type object.
- **Array-valued** groups (`path`, `path-regex`, …): one leaf holds a JSON **array**; each element is a **rule object** (matcher + file type fields).

---

## Registry keys → `LocalRegistry` paths

Segments use the dotted key; the last segment is the filename stem.

### `extension` — `{ ext → FileType }`

- **Key:** `OmniShell.FileTypes.extension.<ext>`
- **`<ext>`:** lowercase, **no leading dot** (`png`, `jpg`, `tar`, `gz`).
- **Value:** JSON object — **FileType** (see below).

Example key: `OmniShell.FileTypes.extension.png`  
On disk: `registry/OmniShell/FileTypes/extension/png.json`

### `folder` — `FileType`

- **Key:** `OmniShell.FileTypes.folder`
- **Value:** JSON object — **FileType**.

### `mime-type` — `{ mime id → FileType }`

- **Key:** `OmniShell.FileTypes.mime-type.<safeId>`
- **`<safeId>`:** MIME type with `/` and other awkward characters replaced so the key is a single path segment. Recommended: replace `/` with `_` and use lowercase (e.g. `text/plain` → `text_plain`).
- **Value:** JSON object — **FileType**.

### Array groups — `FileType[]` as JSON array

Each of these uses **one** leaf whose string value parses as a JSON **array**:

| Key | Value |
|-----|--------|
| `OmniShell.FileTypes.path` | Array of **path rules** |
| `OmniShell.FileTypes.path-regex` | Array of **regex rules** |
| `OmniShell.FileTypes.filename` | Array of **filename rules** |
| `OmniShell.FileTypes.filename-glob` | Array of **glob rules** |

Each **array element** is a JSON object that includes a **matcher** field plus **FileType** fields (`openwith`, `icon`):

| Group | Matcher field | Semantics |
|-------|----------------|-----------|
| `path` | `path` (string) | Prefix or full logical path (volume-relative or normalized app convention). |
| `path-regex` | `pattern` (string) | ECMA-style regex; evaluated in order. |
| `filename` | `name` (string) | Exact filename (e.g. `Makefile`). |
| `filename-glob` | `pattern` (string) | Glob pattern (e.g. `*.png`). |

Resolution order (recommended for implementations): **filename** rules → **filename-glob** → **path** → **path-regex** → **extension** → **mime-type** (if known) → **folder** (when opening a directory) → default.

---

## `FileType` object

```json
{
  "openwith": {
    "Paint": "omnishell.Paint",
    "Notepad": "omnishell.Notepad"
  },
  "icon": { ... ImageSet ... }
}
```

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `openwith` | object | no | Map **display label** → **module URI** (same strings as `Module::uri` / `getFullUri()`, e.g. `omnishell.Paint`). UI may show multiple choices. |
| `icon` | object | no | **ImageSet** (see below). |

Either field may be omitted; an empty object is valid.

---

## `ImageSet` object

Maps to the bas `ImageSet` concept: optional stock art id, default asset path, and optional per-scale overrides.

| Field | Type | Description |
|-------|------|-------------|
| `wxartid` | string | Name of a wx stock art id (e.g. `"wxART_CDROM"`). Used when no suitable asset is found, if the loader supports it. |
| `asset` | string | Primary vector or raster path under the app asset root (e.g. `streamline-vectors/core/pop/computer-devices/hard-drive-1.svg`). |
| `<WxH>` | string | Optional per-size asset path; key is literally like `16x16`, `32x32`, `48x48` (digits, lowercase `x`). |

Example:

```json
{
  "wxartid": "wxART_CDROM",
  "asset": "streamline-vectors/core/pop/computer-devices/hard-drive-1.svg",
  "16x16": "streamline-vectors/core/pop/computer-devices/hard-drive-1-16.png",
  "32x32": "streamline-vectors/core/pop/computer-devices/hard-drive-1-32.png"
}
```

**Note:** Use the repo’s real asset prefix **`streamline-vectors/`** (with an **s**), not `streamline-vector/`.

---

## Full example (logical JSON)

**Extension map** — separate registry leaves (each value is the inner JSON object as the registry string):

`OmniShell.FileTypes.extension.png`:

```json
{
  "openwith": {
    "Paint": "omnishell.Paint"
  },
  "icon": {
    "asset": "streamline-vectors/core/pop/images-photography/edit-image-photo.svg"
  }
}
```

`OmniShell.FileTypes.extension.txt`:

```json
{
  "openwith": {
    "Notepad": "omnishell.Notepad"
  },
  "icon": {
    "asset": "streamline-vectors/core/pop/interface-essential/text-square.svg"
  }
}
```

**Folder:**

`OmniShell.FileTypes.folder`:

```json
{
  "icon": {
    "asset": "streamline-vectors/core/pop/interface-essential/folder-add.svg"
  }
}
```

**MIME:**

`OmniShell.FileTypes.mime-type.text_plain`:

```json
{
  "openwith": {
    "Notepad": "omnishell.Notepad"
  }
}
```

**Path rules** — single leaf `OmniShell.FileTypes.path`, value = JSON array string:

```json
[
  {
    "path": "/Photos",
    "openwith": { "Paint": "omnishell.Paint" },
    "icon": { "asset": "streamline-vectors/core/pop/images-photography/camera-1.svg" }
  }
]
```

---

## Consumers

- **Explorer** and other modules should read these keys via `os::registry()` / `RegistryDb::getInstance()`, parse JSON from `getString`, and resolve **module URI** through `ModuleRegistry::getModule(uri)`.
- **Editing:** the Registry app can show raw string values; a dedicated “File types” UI could be added later to edit structured forms and emit valid JSON strings.

---

## See also

- [README.md](README.md) — registry backends, encoding, and `LocalRegistry` file layout.
