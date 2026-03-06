# OmniShell Icon Mapping

This document maps OmniShell UI elements and modules to their icon assets.

## Icon Source

All icons are from **Streamline Vectors Core Pop** collection:
`/usr/share/streamline-vectors/core/pop/`

Copied to: `assets/streamline-vectors/core/pop/`

---

## Module Icons

### Built-in Applications

| Module | Icon Path | Description |
|--------|-----------|-------------|
| Notepad | `interface-essential/blank-notepad.svg` | Text editor |
| Control Panel | `interface-essential/cog-1.svg` | System settings |
| Paint | `interface-essential/paint-palette.svg` | Drawing application |
| File Manager | `interface-essential/new-folder.svg` | File browser |
| Settings | `interface-essential/cog-1.svg` | System preferences |

### System Tools

| Tool | Icon Path | Description |
|------|-----------|-------------|
| Desktop | `computer-devices/computer-pc-desktop.svg` | Desktop environment |
| Taskbar | `interface-essential/horizontal-menu-circle.svg` | Taskbar |
| Start Menu | `interface-essential/dashboard-3.svg` | Application launcher |
| Search | `interface-essential/search-visual.svg` | Search functionality |
| Help | `interface-essential/help-question-1.svg` | Help system |
| Information | `interface-essential/information-circle.svg` | System info |

### Services

| Service | Icon Path | Description |
|---------|-----------|-------------|
| Clock Service | `interface-essential/circle-clock.svg` | System clock |
| Reminder | `interface-essential/alarm-clock.svg` | Reminder service |
| Printer Service | `computer-devices/printer.svg` | Print spooler |
| Network Service | `computer-devices/network.svg` | Network monitor |
| Storage Service | `computer-devices/hard-disk.svg` | Storage monitor |
| Database Service | `computer-devices/database.svg` | Database service |

### User Management

| Feature | Icon Path | Description |
|---------|-----------|-------------|
| User Profile | `interface-essential/user-circle-single.svg` | User account |
| User Group | `interface-essential/user-multiple-group.svg` | User groups |
| Add User | `interface-essential/user-add-plus.svg` | Create user |
| User Settings | `interface-essential/user-identifier-card.svg` | User config |

### Window Management

| Action | Icon Path | Description |
|--------|-----------|-------------|
| Maximize | `interface-essential/line-arrow-expand-window-1.svg` | Expand window |
| Minimize | `interface-essential/line-arrow-minimize-window-1.svg` | Minimize window |
| Window | `interface-essential/layout-window-1.svg` | Window icon |
| Application | `programming/application-add.svg` | App launcher |

### File Operations

| Operation | Icon Path | Description |
|-----------|-----------|-------------|
| New File | `interface-essential/new-file.svg` | Create file |
| New Folder | `interface-essential/new-folder.svg` | Create folder |
| Archive | `interface-essential/archive-box.svg` | Archive/zip |
| Upload | `interface-essential/upload-file.svg` | Upload file |
| Download | `interface-essential/download-file.svg` | Download file |
| Delete File | `interface-essential/file-delete-alternate.svg` | Delete file |

### Media & Photography

| Feature | Icon Path | Description |
|---------|-----------|-------------|
| Photo Editor | `images-photography/edit-image-photo.svg` | Image editing |
| Camera | `images-photography/camera-1.svg` | Camera/capture |
| Photo Gallery | `interface-essential/pictures-folder-memories.svg` | Photo folder |
| Webcam | `computer-devices/webcam-video.svg` | Video capture |

### Development Tools

| Tool | Icon Path | Description |
|------|-----------|-------------|
| Code Editor | `programming/code-monitor-1.svg` | Code development |
| Terminal | `computer-devices/command-line.svg` | Command prompt |
| Debugger | `programming/bug-virus-document.svg` | Debugging |

### Communication

| Feature | Icon Path | Description |
|---------|-----------|-------------|
| Mail | `mail/inbox-tray-1.svg` | Email inbox |
| Chat | `mail/chat-bubble-text-square.svg` | Messaging |

### Business

| Feature | Icon Path | Description |
|---------|-----------|-------------|
| Payment | `money-shopping/credit-card-1.svg` | Payment processing |
| Business User | `money-shopping/business-user-curriculum.svg` | Business profile |

---

## UI Element Icons

### Desktop Icons

```cpp
// Desktop icon
image = ImageSet(Path("streamline-vectors/core/pop/computer-devices/computer-pc-desktop.svg"));
```

### Start Menu

```cpp
// Start menu button
image = ImageSet(Path("streamline-vectors/core/pop/interface-essential/dashboard-3.svg"));
```

### Taskbar

```cpp
// Taskbar icon
image = ImageSet(Path("streamline-vectors/core/pop/interface-essential/horizontal-menu-circle.svg"));
```

### System Tray

```cpp
// Tray icon (clock)
image = ImageSet(Path("streamline-vectors/core/pop/interface-essential/circle-clock.svg"));
```

---

## Icon Usage in Code

### Setting Module Icons

```cpp
#include "src/util/Path.hpp"
#include "src/ui/arch/ImageSet.hpp"

void MyModule::initializeMetadata() {
    // ... other metadata ...
    
    // Set icon with multiple sizes
    image = ImageSet(Path("streamline-vectors/core/pop/interface-essential/cog-1.svg"));
    image.scale(16, 16, Path("streamline-vectors/core/pop/interface-essential/cog-1.svg"));
    image.scale(32, 32, Path("streamline-vectors/core/pop/interface-essential/cog-1.svg"));
    image.scale(48, 48, Path("streamline-vectors/core/pop/interface-essential/cog-1.svg"));
}
```

### Loading Icons

```cpp
#include "src/wx/wx_assets.hpp"

wxBitmap icon = loadBitmapFromAsset(
    "streamline-vectors/core/pop/interface-essential/cog-1.svg",
    32, 32
);
```

---

## Icon Categories

### interface-essential/
UI controls, actions, and general purpose icons

### computer-devices/
Hardware, devices, and system-related icons

### images-photography/
Photo and image-related icons

### programming/
Development and coding tools

### mail/
Communication and messaging

### money-shopping/
Business and finance

### health/
Medical and health (for future use)

### food-drink/
Food and beverage (for future use)

### entertainment/
Media and entertainment

### map-travel/
Location and travel (for future use)

---

## Adding New Icons

To add new icons:

1. Find the icon in `/usr/share/streamline-vectors/core/pop/`
2. Copy to assets preserving directory structure:
   ```bash
   cp /usr/share/streamline-vectors/core/pop/category/icon.svg \
      assets/streamline-vectors/core/pop/category/icon.svg
   ```
3. Update this mapping document
4. Use in code via `ImageSet` or `loadBitmapFromAsset()`

---

## Icon Format

- **Primary format**: SVG (scalable, small size)
- **Fallback**: PNG (if SVG not available)
- **Recommended sizes**: 16x16, 24x24, 32x32, 48x48

---

## License

Streamline Vectors Core Pop icons are used under the appropriate license.
Check the original source for licensing terms.
