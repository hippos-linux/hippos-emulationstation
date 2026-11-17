# hippos-emulationstation

HippOS frontend — a heavily modified fork of [batocera-emulationstation](https://github.com/batocera-linux/batocera-emulationstation) tailored for HippOS.

---

## What's changed from upstream

### System integration

- **OTA updates** — update and rollback UI backed by btrfs subvolume snapshots, with live progress notifications and per-component update support (OS, EmulationStation, emulators, kernel, patches)
- **Content store** — integrated package and Wine runner downloader with an Emulators tab, Apps tab, and theme catalogue sourced from hippos-linux.org
- **BIOS checker** — per-system missing BIOS report
- **Installer** — partition-select and free-space install modes, disk clone via btrfs send/receive
- **Log upload** — Upload Logs in the main menu for support

### Controllers and input

- **Handheld controls** — controller settings in the controller menu for handheld PC built-in controls
- **Light gun settings** — Sinden configuration; restarts HippOS light gun services on change
- **Pad-to-keyboard key editor** — live key mapping from the in-game quick menu
- **Help bar** — shows face buttons by position rather than letters

### Display and audio

- **CRT output** — manual CRT output picker in System Settings
- **Power, display and LED options** — brightness, power mode, and LED settings

### Networking and storage

- **Network share browser** — browse, add and authenticate SMB/NFS shares
- **Per-share save sync** — switch per share to write saves back or keep read-only
- **Drive merge prompt** — drives present at startup are offered for merging, same as hot-plug
- **Desktop install/uninstall** — install and remove desktop environments with live progress
- **Faster scraper** — faster start on network shares; Windows system load improved

### Session

- **Media centers** — menu to launch and switch between Kodi, Jellyfin, Emby, and Plex HTPC
- **Login screen** — optional login screen and auto-login switch in Security settings
- **Log out** — Log out option in the Quit menu
- **Sleep timer, HDMI-CEC, wake-on-controller, WoL** — in System Settings
- **Keyboard layout selector** — live layout and variant picker
- **Language setting** — persists correctly across reboots

### HTTP API extensions

The ES HTTP server runs on `http://127.0.0.1:1234`. Additions over upstream:

| Endpoint | Description |
|---|---|
| `POST /confirm` | Show a YES/NO dialog. Body: `message\nshell command to run on YES` |
| `POST /progress` | Create or update a progress notification on the UI thread |
| `GET /shutdown` | Quit ES and shut the system down |
| `POST /storage/event` | Trigger storage dialogs: `REQUEST_MERGE`, `REQUEST_NETWORK_MERGE`, `REQUEST_FORMAT` |

### Emulator selector

- Per-system emulator and core override in the game options menu
- Selection persists to `hippos.conf`

### Theme and branding

- HippOS colour scheme, branding, and asset updates
- Recent games collection enabled by default
- Capitalised built-in collection names

---

## Building

This repo is a submodule of [hippos-linux/hippos-linux](https://github.com/hippos-linux/hippos-linux). Build it through the parent repo:

```bash
./build/build-frontend.sh
```

Or standalone with CMake (Debian Trixie):

```bash
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DHIPPOS=1
cmake --build build -j"$(nproc)"
```

---

## Credits

Based on [batocera-emulationstation](https://github.com/batocera-linux/batocera-emulationstation), itself based on [EmulationStation](https://github.com/Aloshi/EmulationStation).

---

## License

GPLv2 — see [LICENSE.md](LICENSE.md).
