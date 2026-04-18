# Nuphy Common Code

`keyboards/nuphy/common` contains the shared implementation used by the refactored Nuphy keyboards.

## Current Scope

The common layer is currently used by:

- `nuphy/air75v2/ansi`
- `nuphy/gem80`
- `nuphy/halo75v2/ansi`
- `nuphy/halo96v2/ansi`

Gem80 is the main extraction reference, while the other boards consume the shared code through board-local wrappers and board-specific hooks.

## Layout

The common layer is grouped by subsystem:

- `config/`: shared config structs, defaults, EEPROM storage, and VIA helpers
- `core/`: keyboard flow, key processing, and debounce
- `lighting/`: shared lighting-facing headers and the WS2812 side driver
- `power/`: sleep support and the shared MCU power include-body
- `system/`: timer helpers
- `wireless/`: RF transport, queueing, and link helpers
- `features/`: opt-in shared features such as SOCD cleaning

## What Stays Board-Local

Each keyboard still keeps board-specific files for hardware details that are not yet identical across boards, including:

- matrix scanning
- board lighting layouts and LED placement
- board-specific side or logo light implementations
- board-specific wrapper files that include the shared `power/mcu_pwr_shared.inc` body with the right board config in scope
- keyboard JSON, VIA definitions, and board-specific metadata

## Notes

- The shared `power/mcu_pwr_shared.inc` file is an include-body implementation by design. It is expected to be compiled through board-local wrappers such as `air75v2/ansi/mcu_pwr.c` and `gem80/mcu_pwr.c`.
- Standalone LSP diagnostics on `common/power/mcu_pwr_shared.inc` are expected and are not meaningful unless wrapper-based board builds fail.
- The common layer has been updated for upstream QMK `0.32.7` compatibility.
