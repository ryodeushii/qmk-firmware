# Nuphy Common Code

`keyboards/nuphy/common` contains the shared implementation used by the refactored Nuphy keyboards.

## Current Scope

The common layer is currently used by:

- `nuphy/air75v2/ansi`
- `nuphy/gem80`
- `nuphy/halo75v2/ansi`
- `nuphy/halo96v2/ansi`

Gem80 is the main extraction reference, while the other boards consume the shared code through board-local wrappers and board-specific hooks.

## What Lives Here

The common layer currently owns or provides shared implementations for:

- keyboard startup and housekeeping flow in `keyboard.c/.h`
- shared key processing and custom key behavior in `keys.c/.h`
- keyboard config storage/defaults in `config.c/.h`
- VIA integration helpers in `via.c/.h`
- debounce implementation in `debounce.c/.h`
- RF helpers, RF protocol handling, and report queueing in `rf.c`, `rf_driver.c/.h`, and `rf_queue.c/.h`
- sleep/wake handling in `sleep.c`
- shared MCU power-management implementation in `mcu_pwr.c`
- shared timer and link helpers in `timer.c` and `links.c`
- shared side-light helper declarations in `side.h`
- shared WS2812 side-driver support in `ws2812-side-driver.c`
- small shared headers such as `ambient.h` and `mcu_stm32f0xx.h`

## What Stays Board-Local

Each keyboard still keeps board-specific files for hardware details that are not yet identical across boards, including:

- matrix scanning
- board lighting layouts and LED placement
- board-specific side or logo light implementations
- board-specific wrapper files that include the shared `mcu_pwr.c` body with the right board config in scope
- keyboard JSON, VIA definitions, and board-specific metadata

## Notes

- The shared `mcu_pwr.c` file is an include-body implementation by design. It is expected to be compiled through board-local wrappers such as `air75v2/ansi/mcu_pwr.c` and `gem80/mcu_pwr.c`.
- Standalone LSP diagnostics on `common/mcu_pwr.c` are expected and are not meaningful unless wrapper-based board builds fail.
- The common layer has been updated for upstream QMK `0.32.7` compatibility.
