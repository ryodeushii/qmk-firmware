# Nuphy Common Code

`keyboards/nuphy/common` contains the shared implementation used by the refactored Nuphy keyboards.

## Current Scope

The common layer is currently used by:

- `nuphy/air75v2/ansi`
- `nuphy/gem80`
- `nuphy/halo75v2/ansi`
- `nuphy/halo96v2/ansi`

Gem80 is the main extraction reference, while the other boards consume the shared code through stable `common/*.h` entry points and a small set of board-specific hooks.

## Layout

The common layer is grouped by subsystem:

- `common/*.h`: the supported shared include surface for board code
- `config/`: shared config structs, defaults, EEPROM storage, and VIA helpers
- `core/`: keyboard flow, key processing, and debounce internals
- `lighting/`: shared lighting-facing headers and the WS2812 side driver
- `power/`: sleep support and shared MCU power implementation
- `system/`: housekeeping timer helpers
- `wireless/`: RF transport, queueing, and link helpers

## Public Headers

Board-local code should include the root forwarding headers under `common/` instead of reaching into subsystem folders directly.

- `common/config.h`: shared keyboard config types, EEPROM helpers, and config mutation helpers
- `common/keyboard.h`: shared keyboard lifecycle entry points
- `common/keys.h`: shared custom keycodes and key processing entry point
- `common/wireless.h`: shared wireless state and high-level RF operations
- `common/debounce.h`: shared debounce helpers that boards legitimately call
- `common/ambient.h`, `common/indicator.h`, `common/side.h`: shared lighting interfaces
- `common/timer.h`: shared timer step and housekeeping timer entry point
- `common/mcu_stm32f0xx.h`: STM32 support header needed by board-local power hooks

Anything under a subsystem directory such as `common/wireless/` or `common/core/` should be treated as internal implementation detail unless there is a concrete reason to expose it.

## What Stays Board-Local

Each keyboard still keeps board-specific files for hardware details that are not yet identical across boards, including:

- matrix scanning
- board lighting layouts and LED placement
- board-specific side or logo light implementations
- board-specific wake or hardware hooks such as `halo96v2/ansi/mcu_pwr.c`
- keyboard JSON, VIA definitions, and board-specific metadata

## Dependency Map

The shared layer is intentionally directional:

- `config` provides persistent config state and shared helpers used by `core`, `lighting`, and `power`
- `core` depends on `config`, `wireless`, and `lighting`
- `power` depends on `config`, `wireless`, and board-local hardware headers
- `system` depends on `wireless` for shared runtime counters
- `wireless` owns RF transport details and exposes a smaller high-level API through `common/wireless.h`
- boards should prefer `common/*.h` and avoid depending on subsystem-internal headers directly

## Notes

- `common/power/mcu_pwr.c` is now compiled as a normal shared source file through `keyboards/nuphy/rules.mk`.
- Boards that need wake behavior different from the default `matrix_init_custom()` path can override the weak `nuphy_matrix_wake_init()` hook in a board-local file.
- `common/wireless/rf_protocol.h` is intentionally internal. Boards should use the high-level helpers in `common/wireless.h` instead of depending on RF command constants.
- The common layer has been updated for upstream QMK `0.32.7` compatibility.
