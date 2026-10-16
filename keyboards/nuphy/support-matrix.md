# NuPhy support matrix

This repository maintains five NuPhy board and layout combinations. The release profile uses the shared implementation in `keyboards/nuphy/common` and the target list in `keyboards/nuphy/release-manifest.json`.

## Supported targets

All five supported board/layout combinations have a `THREE_MODE` release target. `USB_MODE` is an additional wired build mode for Gem80. The `THREE_MODE` firmware supports wired USB, 2.4 GHz RF, and Bluetooth runtime checks for each supported board and layout.

| Board | Layout | Release mode | Additional mode | USB | 2.4 GHz RF | Bluetooth |
| --- | --- | --- | --- | --- | --- | --- |
| Air75 V2 | ANSI | `THREE_MODE` | None | Check | Check | Check |
| Gem80 | ANSI | `THREE_MODE` | `USB_MODE` | Check | Check | Check |
| Gem80 | ISO | `THREE_MODE` | `USB_MODE` | Check | Check | Check |
| Halo75 V2 | ANSI | `THREE_MODE` | None | Check | Check | Check |
| Halo96 V2 | ANSI | `THREE_MODE` | None | Check | Check | Check |

`USB_MODE` builds are wired-only builds. Hardware records below must identify any transport that is unavailable on a tested hardware revision.

## Firmware and metadata contract

Release builds include keymaps `default`, `via`, and `srgb`. The release profile contains 21 firmware binaries:

| Board and layout | Build modes | Binaries |
| --- | --- | ---: |
| Air75 V2 ANSI | `THREE_MODE` | 3 |
| Gem80 ANSI | `USB_MODE`, `THREE_MODE` | 6 |
| Gem80 ISO | `USB_MODE`, `THREE_MODE` | 6 |
| Halo75 V2 ANSI | `THREE_MODE` | 3 |
| Halo96 V2 ANSI | `THREE_MODE` | 3 |
| **Total** |  | **21** |

Personal keymap `ryodeushii` is built separately for five binaries. It is not part of release support and is not attached to draft releases.

The binary naming pattern is:

```text
<prefix><keyboard>-<layout>-<keymap>-<tag>.bin
```

Gem80 wired builds use the `wired-` prefix. Gem80 `THREE_MODE` builds use the `threemode-` prefix. Other release builds use no prefix.

Every release includes these five VIA3 metadata files:

- `keyboards/nuphy/air75v2/ansi/keymaps/via/NuPhy Air75 V2 via3.json`
- `keyboards/nuphy/gem80/ansi/keymaps/via/NuPhy Gem80 via3.json`
- `keyboards/nuphy/gem80/iso/keymaps/via/NuPhy Gem80 ISO via3.json`
- `keyboards/nuphy/halo75v2/ansi/keymaps/via/NuPhy Halo75v2 via3.json`
- `keyboards/nuphy/halo96v2/ansi/keymaps/via/NuPhy Halo96v2 via3.json`

The reference QMK base is version `0.32.7` at commit `ed80e21858`.

Use [`instructions.md`](./instructions.md) for flashing, recovery, RF firmware, and VIA procedures. Use the board readmes for board-specific build details.

## Hardware support records

A compile result does not prove hardware support. Each check below uses `PASS`, `FAIL`, or `UNVERIFIED`; no physical hardware session was available for this verification, so every hardware check remains `UNVERIFIED`.

### Task 8 release records

The following record metadata applies to every board/mode row:

| Field | Value |
| --- | --- |
| Hardware revision / serial identifier | `UNAVAILABLE` (no physical hardware session) |
| Firmware tag | `task8-verify` |
| Manifest revision / commit | `b7dbb0557112f51ccbe02a9b9def8841658fc1c6` |
| Test date | `2026-08-28` |
| Build evidence | `PASS` from local verification; not a release artifact |
| Hardware evidence | `UNVERIFIED`; no physical evidence available |

| Board and layout | Build mode | Release keymaps | Build result | Hardware result | Evidence location |
| --- | --- | --- | --- | --- | --- |
| Air75 V2 ANSI | `THREE_MODE` | `default`, `via`, `srgb` | `PASS` (3/3) | `UNVERIFIED` | Local-only Task 8 build; not a release artifact; reproduce via `.github/workflows/build_nuphy_keyboards.yml` |
| Gem80 ANSI | `USB_MODE` | `default`, `via`, `srgb` | `PASS` (3/3) | `UNVERIFIED` | Local-only Task 8 build; not a release artifact; reproduce via `.github/workflows/build_nuphy_keyboards.yml` |
| Gem80 ANSI | `THREE_MODE` | `default`, `via`, `srgb` | `PASS` (3/3) | `UNVERIFIED` | Local-only Task 8 build; not a release artifact; reproduce via `.github/workflows/build_nuphy_keyboards.yml` |
| Gem80 ISO | `USB_MODE` | `default`, `via`, `srgb` | `PASS` (3/3) | `UNVERIFIED` | Local-only Task 8 build; not a release artifact; reproduce via `.github/workflows/build_nuphy_keyboards.yml` |
| Gem80 ISO | `THREE_MODE` | `default`, `via`, `srgb` | `PASS` (3/3) | `UNVERIFIED` | Local-only Task 8 build; not a release artifact; reproduce via `.github/workflows/build_nuphy_keyboards.yml` |
| Halo75 V2 ANSI | `THREE_MODE` | `default`, `via`, `srgb` | `PASS` (3/3) | `UNVERIFIED` | Local-only Task 8 build; not a release artifact; reproduce via `.github/workflows/build_nuphy_keyboards.yml` |
| Halo96 V2 ANSI | `THREE_MODE` | `default`, `via`, `srgb` | `PASS` (3/3) | `UNVERIFIED` | Local-only Task 8 build; not a release artifact; reproduce via `.github/workflows/build_nuphy_keyboards.yml` |

Manifest-driven release command (use dedicated `release-artifacts` staging; local output is not a durable release artifact):

```text
SILENT=true PARALLEL=2 ./compile_all.sh --profile release --manifest keyboards/nuphy/release-manifest.json --tag task8-verify --staging-dir release-artifacts
```

The verification run produced 21 firmware binaries and five VIA3 metadata files. The personal-profile run produced five separate binaries in dedicated `personal-artifacts` staging; no personal binary was present in release staging. Durable references are the manifest and workflow definitions at commit `b7dbb05571`; local command output is not retained as release evidence.

### Hardware check matrix

`UNVERIFIED` means no physical board, hardware revision, serial identifier, or hardware test evidence was available. Compile success is not used as hardware evidence. The evidence location for every cell is this tracked support record; the verification build was local-only and is not a durable release artifact.

| Board and layout | Build mode | Wired typing and matrix scan | RF pairing and reconnect | Bluetooth profiles and reconnect | Sleep entry | First-key wake | Charging behavior | RGB matrix and side indicators | VIA import | Factory reset | Bootloader flashing | Recovery |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Air75 V2 ANSI | `THREE_MODE` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` |
| Gem80 ANSI | `USB_MODE` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` |
| Gem80 ANSI | `THREE_MODE` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` |
| Gem80 ISO | `USB_MODE` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` |
| Gem80 ISO | `THREE_MODE` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` |
| Halo75 V2 ANSI | `THREE_MODE` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` |
| Halo96 V2 ANSI | `THREE_MODE` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` | `UNVERIFIED` |

### Personal build evidence

The personal keymap is build-only evidence and is not release support:

| Profile | Binaries | Build result | Hardware result | Evidence location |
| --- | ---: | --- | --- | --- |
| `personal` / `ryodeushii` | 5 | `PASS` (5/5) | `UNVERIFIED` | Local-only Task 8 build; not a release artifact; reproduce via `.github/workflows/build_nuphy_keyboards.yml` |

Do not mark a profile fully supported until its available hardware checks have recorded results. Mark unavailable or untested checks `UNVERIFIED` instead of inferring support from a successful build.
