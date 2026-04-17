# Nuphy Keyboard Instructions

This document recreates and preserves the useful parts of a previously shared UpNote page that is no longer available.

Archived source:
- [Wayback Machine capture](https://web.archive.org/web/20250723110311/https://getupnote.com/share/notes/ihWZQ7d8mebhejrlrJWo322IxQD3/d745e165-ed4c-4322-a693-d6d6f4b6c37b)

Where possible, links have been updated to point at files in this repository or at the current upstream project pages instead of the original note.

## Related Files

- [Nuphy directory overview](./README.md)
- [Air75 V2 ANSI VIA JSON](./air75v2/ansi/keymaps/default/NuPhy%20Air75%20V2%20via3.json)
- [Gem80 ANSI VIA JSON](./gem80/ansi/keymaps/default/NuPhy%20Gem80%20via3.json)
- [Gem80 ISO VIA JSON](./gem80/iso/keymaps/default/NuPhy%20Gem80%20ISO%20via3.json)
- [Halo75 V2 ANSI VIA JSON](./halo75v2/ansi/keymaps/default/NuPhy%20Halo75v2%20via3.json)
- [Halo96 V2 ANSI VIA JSON](./halo96v2/ansi/keymaps/default/NuPhy%20Halo96v2%20via3.json)
- [QMK Toolbox releases](https://github.com/qmk/qmk_toolbox/releases)
- [Official NuPhy firmware page](https://nuphy.com/pages/qmk-firmwares)
- [Official NuPhy VIA guide](https://nuphy.com/pages/via-usage-guide-for-nuphy-keyboards)

## Updating The QMK Firmware

1. Download [QMK Toolbox](https://github.com/qmk/qmk_toolbox/releases).
2. Download the firmware you want to flash.
   - You can use an official NuPhy release.
   - You can also flash a build produced from this repository.
3. Open QMK Toolbox and load the firmware via the `Open` button.
4. With the keyboard unplugged and set to wired mode, hold `Esc` and plug the keyboard in.
5. Yellow text should appear in QMK Toolbox indicating the device is connected.
6. If you see `(NO DRIVER)`, install the drivers before continuing.
   - On Windows, run QMK Toolbox as Administrator if needed.
   - Use `Ctrl+N` in QMK Toolbox to install the drivers.
7. Click `Flash`.
8. Do not disconnect the device until the flash completes successfully.

## Recovering From An Incomplete Or Corrupt Flash

1. Unplug the keyboard.
2. Remove the `Caps Lock` keycap.
3. Find the small black button next to the `Caps Lock` switch.
4. Press and hold that button.
5. While holding the button, plug the keyboard in.
6. This should restore the keyboard to a factory state so you can try flashing again.
7. Before retrying, resolve the original problem such as a bad cable, power loss, or a corrupt firmware file.

## Updating The RF Firmware

Updating the RF firmware usually is not necessary. Only try this if other attempts to improve RF reliability have failed.

### Keyboard

Firmware: latest archived reference as of Jun 12, 2024

- [NUPHY_RF_OTA_V1.1.2.zip](https://web.archive.org/web/20250723110311/https://firebasestorage.googleapis.com/v0/b/get-senna.appspot.com/o/users%2FihWZQ7d8mebhejrlrJWo322IxQD3%2Ffebdf21d-91d4-4553-8334-6f25e98084ff__zip?alt=media&token=a7b0d89a-b5f7-45df-a9e6-536f0d4aab5d)

Instructions:

1. Download the firmware to your phone.
2. Do not unzip the file.
3. Install the `nRF Connect` app.
4. Connect the keyboard to your phone in wired mode.
5. Press `Fn+[` to restore the keyboard to factory settings.
6. Refresh the `nRF Connect` app until a device named `DfuTarg` appears.
7. Connect to that device.
8. Use the `DFU` action in the upper-right corner.
9. Choose `Distribution packet (ZIP)`.
10. Select the downloaded firmware.
11. Wait for the flash to complete.
12. Power cycle the keyboard.

### Dongle

If updating the keyboard RF firmware still does not help, you can also try the dongle firmware.

Firmware: latest archived reference as of May 16, 2024

- [dongle_52820_upgrade_v1.0.2.exe](https://web.archive.org/web/20250723110311/https://firebasestorage.googleapis.com/v0/b/get-senna.appspot.com/o/users%2FihWZQ7d8mebhejrlrJWo322IxQD3%2F396c42d3-52af-4430-857e-ab19bfa7e604__exe?alt=media&token=0e801a38-84b3-4a92-908c-89025f7f82b6)

Run this at your own risk. Do not blindly trust random executables.

Instructions:

1. Connect the dongle to a Windows computer.
2. This process cannot be done natively on macOS.
3. Download and run the firmware upgrade tool linked above.
4. Click `Start`.
5. Wait until the upgrade is fully complete.
6. Click `Finish`.

## Customizing With VIA

1. Download the [VIA desktop app](https://github.com/the-via/releases/releases/tag/v3.0.0) or open [usevia.app](https://usevia.app/) in a Chromium-based browser.
2. Load the JSON file that matches your keyboard.
   - For builds from this repository, prefer the JSON files linked above from `keyboards/nuphy/.../keymaps/default/`.
3. In VIA, open the `Settings` tab and enable `Show Design Tab`.
4. Open the `Design` tab and load the JSON file.
5. Open the `Configure` tab to customize keymaps, macros, and lighting.
6. NuPhy keyboards use layers `0` and `1` for macOS, and layers `2` and `3` for Windows.

Future versions of VIA may be able to recognize more NuPhy keyboards automatically, but manual JSON import is still the safe path for custom/community builds.
