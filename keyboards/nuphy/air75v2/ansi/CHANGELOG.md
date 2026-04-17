# CHANGELOG

## 0.32.7-sync

- synced the keyboard with upstream QMK `0.32.7`
- migrated major shared behavior to `keyboards/nuphy/common`
- switched Air75v2 to the shared/common startup, key processing, config, debounce, RF, sleep, and power-management path
- normalized Air75v2 ambient/logo naming and VIA wording
- fixed boot/runtime regressions introduced by the upstream sync, including the shared debounce ABI mismatch
- fixed BLE device naming to use the Air75v2-specific shared RF naming path
- fixed wireless wake input handling so wake typing is queued and delivered reliably
