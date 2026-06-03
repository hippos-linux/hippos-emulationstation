# hippos-emulationstation

HippOS frontend — a heavily modified fork of [batocera-emulationstation](https://github.com/batocera-linux/batocera-emulationstation) tailored for HippOS.

---

## What's changed from upstream

- **Content store** — integrated package/wine-runner downloader (`hippos-store`)
- **OTA updates** — update and rollback UI backed by btrfs subvolume snapshots (`hippos-upgrade`)
- **BIOS checker** — per-system missing BIOS report (`hippos-systems`)
- **Keyboard layout selector** — live layout/variant picker (`hippos-keyboard`)
- **Emulator selector** — per-system emulator/core override in the game menu
- **Recent games** — last-played collection enabled by default
- **Wine runner download** — three separate tabs (Wine-GE, Wine-TKG, Proton-GE)
- **HippOS theming** — branding, colour scheme, and asset updates

---

## Building

This repo is a submodule of [hippos-linux/hippos](https://github.com/hippos-linux/hippos). Build it through the parent repo:

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

Based on [batocera-emulationstation](https://github.com/batocera-linux/batocera-emulationstation) which is itself based on [EmulationStation](https://github.com/Aloshi/EmulationStation).

---

## License

GPLv2 — see [COPYING](COPYING).
