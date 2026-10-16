# STM32F072 GPIO warning investigation

## Decision

No NuPhy source change is required. The `-Wint-to-pointer-cast` diagnostics are host-width diagnostics emitted when ChibiOS encodes a GPIO port pointer in its 32-bit `ioline_t` line identifier and later decodes that identifier back to `stm32_gpio_t *`. On the STM32F072 target, pointers and `uint32_t` are both 32 bits, and the verified GPIO addresses fit that representation exactly.

No warning suppression, compiler flag change, or `lib/chibios` change was made.

## Diagnostics captured

Repository base was clean at `4ef82a63d0b696a2daeece60d4d5b870b3fa0033` on branch `feature/nuphy-release-hardening`.

The real firmware builds were run with QMK CLI 1.2.0, QMK Firmware 0.33.7, `arm-none-eabi-gcc` 16.2.0, the normal `-Werror`, and the warning explicitly enabled:

```text
qmk compile -kb nuphy/air75v2/ansi -km default -j2 -c -e EXTRAFLAGS=-Wint-to-pointer-cast
qmk compile -kb nuphy/gem80/ansi -km default -j2 -c -e EXTRAFLAGS=-Wint-to-pointer-cast
qmk compile -kb nuphy/halo75v2/ansi -km default -j2 -c -e EXTRAFLAGS=-Wint-to-pointer-cast
qmk compile -kb nuphy/halo96v2/ansi -km default -j2 -c -e EXTRAFLAGS=-Wint-to-pointer-cast
```

All four builds passed with zero compiler warnings. These builds use the actual Cortex-M0 target ABI, so they do not reproduce a pointer-width mismatch.

To capture the reported warning class, each target's generated ARM compile flags were replayed with host Clang 22.1.8, dropping only ARM-only options and adding `-fsyntax-only -Wint-to-pointer-cast -Wpointer-to-int-cast -fmacro-backtrace-limit=0`. Every probed source parsed successfully. Results below count `-Wint-to-pointer-cast` first and its paired `-Wpointer-to-int-cast` diagnostic second:

| Target | Source | Warning counts | Affected source lines |
| --- | --- | ---: | --- |
| Air75 V2 | `keyboards/nuphy/common/core/keyboard.c` | 8 / 16 | 163, 165, 167, 169, 222, 224, 231, 236 |
| Air75 V2 | `keyboards/nuphy/common/wireless/rf.c` | 4 / 8 | 485, 487, 552, 556 |
| Air75 V2 | `keyboards/nuphy/air75v2/ansi/ansi.c` | 16 / 32 | 72, 73, 76, 77, 79, 81, 82, 84, 86, 88, 90, 91, 93, 94, 96, 97 |
| Gem80 | `keyboards/nuphy/common/core/keyboard.c` | 8 / 16 | 163, 165, 167, 169, 222, 224, 231, 236 |
| Gem80 | `keyboards/nuphy/common/wireless/rf.c` | 4 / 8 | 485, 487, 552, 556 |
| Gem80 | `keyboards/nuphy/gem80/gem80-common.c` | 16 / 32 | 78, 79, 83, 84, 86, 89, 90, 92, 95, 98, 101, 102, 104, 105, 107, 108 |
| Halo75 V2 | `keyboards/nuphy/common/core/keyboard.c` | 8 / 16 | 163, 165, 167, 169, 222, 224, 231, 236 |
| Halo75 V2 | `keyboards/nuphy/common/wireless/rf.c` | 4 / 8 | 485, 487, 552, 556 |
| Halo75 V2 | `keyboards/nuphy/halo75v2/ansi/ansi.c` | 10 / 20 | 73, 74, 76, 78, 79, 81, 83, 85, 87, 88 |
| Halo96 V2 | `keyboards/nuphy/common/core/keyboard.c` | 8 / 16 | 163, 165, 167, 169, 222, 224, 231, 236 |
| Halo96 V2 | `keyboards/nuphy/common/wireless/rf.c` | 4 / 8 | 485, 487, 552, 556 |
| Halo96 V2 | `keyboards/nuphy/halo96v2/ansi/ansi.c` | 10 / 20 | 75, 76, 78, 80, 81, 83, 85, 87, 89, 90 |

The `-Wint-to-pointer-cast` notes resolve to `lib/chibios/os/hal/ports/STM32/LLD/GPIOv2/hal_pal_lld.h:177` (`PAL_PORT`). The paired pointer-to-integer diagnostics resolve to `hal_pal_lld.h:171` (`PAL_LINE`). The NuPhy source locations above are macro expansion locations, not independent hand-written integer addresses.

## Complete fix-round diagnostic inventory

The table above covered the original core, RF, and board-initialization sites. The following inventory adds every requested source range and every additional cast site observed while probing the complete source files. Counts are `-Wint-to-pointer-cast / -Wpointer-to-int-cast`; a parenthesized multiplier records repeated macro expansions at one source line. All listed sites have the same decision: **valid target representation; leave unchanged**.

| Source/range | Host diagnostic count | Warning lines or per-line counts | Decision |
| --- | ---: | --- | --- |
| `keyboards/nuphy/common/power/mcu_pwr.c:159-164` | 3 / 0 | 159 (1/0), 160 (1/0), 164 (1/0) | Valid `pin_t` values decoded by `PAL_PORT`; leave unchanged |
| `keyboards/nuphy/common/power/mcu_pwr.c:194-206` | 10 / 20 | 194 (1/2), 195 (1/2), 197 (1/2), 198 (1/2), 200 (2/4), 202 (1/2), 203 (1/2), 205 (1/2), 206 (1/2) | Valid GPIO helper line IDs, including the two-call sleep-side-driver macro at 200; leave unchanged |
| `keyboards/nuphy/common/lighting/ws2812-side-driver.c:81-89` | 4 / 8 | 81 (1/2), 83 (1/2), 87 (1/2), 89 (1/2) | Valid `DRIVER_SIDE_DI_PIN` line ID through GPIO helpers; leave unchanged |
| `keyboards/nuphy/air75v2/ansi/side.c:545-592` | 8 / 16 | 545, 546, 547, 549, 550, 590, 591, 592 (1/2 each) | Valid GPIO helper line IDs; leave unchanged |
| `keyboards/nuphy/air75v2/ansi/matrix.c:41-63` | 10 / 28 | 41 (1/1), 42 (1/1), 47 (1/5), 48 (1/3), 53 (1/1), 55 (1/1), 60 (1/5), 61 (1/3), 62 (1/5), 63 (1/3) | Valid direct `PAL_PORT` use of encoded line IDs; leave unchanged |

The complete-file probes also recorded these sites outside the requested ranges. They are listed to keep the diagnostic inventory lossless:

- `keyboards/nuphy/common/power/mcu_pwr.c:108-109`: pointer-to-int only, 0 / 23 total (line 108: 6, line 109: 17), while initializing row/column arrays from `MATRIX_*_PINS`.
- `keyboards/nuphy/common/power/mcu_pwr.c:218,220,223,224,304,311,318,325,374,386,390`: 17 / 28 total (`218,220,223,224` each 1/2; `304` 3/6; `311` 4/8; `318` 1/2; `325` 2/4; `374,386,390` each 1/0). These are further GPIO helper calls and legacy register-address accesses in the same adapter; all retain the verified target representation.
- `keyboards/nuphy/common/power/mcu_stm32f0xx.h:1355,1374,1376`: 2 / 1 total. `1355` is a legacy USART pointer-to-int conversion; `1374` and `1376` are legacy integer-to-register-pointer conversions. They are non-GPIO, outside the permitted fix files, and remain unchanged.
- `keyboards/nuphy/air75v2/ansi/matrix.c:64-65,77`: 4 / 55 total (`64` 1/8, `65` 1/11, `77` 2/36). These are the same direct `PAL_PORT` matrix operations outside requested range and remain unchanged.

The Air source-only fix-round probe used the existing generated target flags from the earlier build command, removed only ARM-only options, and supplied a temporary header equivalent to tracked `keyboard.json` matrix definitions (6 rows, 17 columns) because later clean builds had removed Air's generated `info_config.h`. It did not alter repository files. Each requested source parsed successfully; the temporary header is not part of the commit.

## Expansion and source-rule coverage

For GPIO-helper sites, `platforms/chibios/gpio.h:25-43` defines `gpio_set_pin_*`, `gpio_write_pin_*`, and `gpio_read_pin` in terms of `palSetLineMode`, `palSetLine`, `palClearLine`, and `palReadLine`. ChibiOS's line operations expand through `PAL_PORT(line)` and `PAL_PAD(line)`; `PAL_PORT` is the integer-to-`stm32_gpio_t *` conversion at `hal_pal_lld.h:176-177`, while `PAL_LINE` is the pointer-to-`uint32_t` encoding at `hal_pal_lld.h:170-171`. Air matrix sites call `PAL_PORT` directly, so they bypass only the GPIO helper wrapper, not the same address decode.

The source rules that compile these files are:

- `keyboards/nuphy/rules.mk:2-14` adds `common/power/mcu_pwr.c`, `common/core/keyboard.c`, and `common/wireless/rf.c` for every NuPhy board.
- `keyboards/nuphy/air75v2/ansi/rules.mk:1-8` adds `common/lighting/ws2812-side-driver.c`, Air `side.c`, and Air `matrix.c`.
- `keyboards/nuphy/gem80/rules.mk:1-10` adds `common/lighting/ws2812-side-driver.c`, Gem80 `side.c`, `gem80-common.c`, and Gem80 `matrix.c`.
- `keyboards/nuphy/halo75v2/ansi/rules.mk:1-7` adds Halo75 `side.c` and `matrix.c`.
- `keyboards/nuphy/halo96v2/ansi/rules.mk:1` adds Halo96 `side.c`; `:7-9` adds `matrix.c`.

These rules explain why the shared adapter appears in all four board builds, why WS2812 side-driver warnings apply to Air/Gem, and why the requested Air side/matrix sites are compiled only by Air's rules. The previously documented core/RF/ANSI sites remain covered by the first diagnostics table.


## STM32F072 address and type evidence

The CMSIS device header `lib/chibios/os/common/ext/ST/STM32F0xx/stm32f072xb.h` defines:

- `PERIPH_BASE` as `0x40000000U` and `AHB2PERIPH_BASE` as `PERIPH_BASE + 0x08000000` (lines 656-661).
- `GPIOA_BASE` through `GPIOF_BASE` at offsets `0x0000`, `0x0400`, `0x0800`, `0x0C00`, `0x1000`, and `0x1400` from AHB2 (lines 719-724), giving `0x48000000`, `0x48000400`, `0x48000800`, `0x48000C00`, `0x48001000`, and `0x48001400`.
- CMSIS `GPIOA` through `GPIOF` as `GPIO_TypeDef *` casts of those base constants (lines 783-788).

ChibiOS `lib/chibios/os/hal/ports/STM32/LLD/GPIOv2/stm32_gpio.h` intentionally undefines the vendor `GPIOA`-style names (lines 26-37), then redefines them as `stm32_gpio_t *` at the same `GPIOx_BASE` constants (lines 53-58). Its `stm32_gpio_t` register block (lines 81-101) contains `MODER`, `OTYPER`, `OSPEEDR`, `PUPDR`, `IDR`, `ODR`, `BSRR`, `LOCKR`, `AFRL`, and `AFRH`.

The CMSIS `GPIO_TypeDef` in `stm32f072xb.h:389-400` has the same offsets through `AFR[2]`; it adds `BRR` after that prefix. The ChibiOS driver accesses only the common register prefix for this GPIOv2 path.

ChibiOS line identifiers are deliberately address-encoded in `lib/chibios/os/hal/ports/STM32/LLD/GPIOv2/hal_pal_lld.h`:

```c
#define PAL_LINE(port, pad) \
  ((ioline_t)((uint32_t)(port)) | ((uint32_t)(pad)))
#define PAL_PORT(line) \
  ((stm32_gpio_t *)(((uint32_t)(line)) & 0xFFFFFFF0U))
typedef uint32_t ioline_t;
typedef stm32_gpio_t * ioportid_t;
```

The lower four address bits are guaranteed zero for these GPIO bases and hold the pad number. An ARM compile-time probe passed assertions for all six F072 GPIO bases, common register offsets, `PAL_LINE(GPIOA, 0) == 0x48000000`, `PAL_LINE(GPIOB, 7) == 0x48000407`, `PAL_LINE(GPIOC, 15) == 0x4800080F`, and `PAL_PORT(PAL_LINE(GPIOx, pad))` restoring each exact base address.

## Comparison with board code

The ChibiOS F072 board implementation at `lib/chibios/os/hal/boards/ST_NUCLEO64_F072RB/board.c:143,163-178` declares its initializer as `gpio_init(stm32_gpio_t *gpiop, ...)` and passes `GPIOA` through `GPIOF` directly. This matches the ChibiOS redefined pointer type and address model.

QMK's `platforms/chibios/gpio.h:25-43` implements `gpio_set_pin_*`, `gpio_write_pin_*`, and `gpio_read_pin` through `palSetLine*`, `palClearLine`, and `palReadLine`. NuPhy's affected calls use this standard line-ID interface. Replacing individual calls with ad-hoc pointer casts would bypass the API and would not remove the underlying `ioline_t` encoding/decoding requirement.

The explicit compatibility cast in `keyboards/nuphy/common/power/mcu_pwr.c:135` (`GPIO_Init((GPIO_TypeDef *)GPIOA, ...)`) and comparisons in `keyboards/nuphy/common/power/mcu_stm32f0xx.h:896-909` operate after ChibiOS has made `GPIOA` a typed `stm32_gpio_t *`; they preserve the same base address and are not the reported integer-to-pointer line-ID conversion.

## Per-site decision and risk

All warning sites in the brief-listed NuPhy sources are **valid representation / leave unchanged**. No typed constant is needed: the constants already originate from ChibiOS's typed `GPIOx` definitions, and `pin_t` is intentionally `ioline_t` (`uint32_t`). A local `uintptr_t` or direct-pointer rewrite would change the public line-ID representation and would not be a semantics-preserving NuPhy-only fix.

The remaining host-width warning is useful evidence for future portability work: if this code were built for a target with pointers wider than 32 bits, line IDs would truncate addresses. That is not the STM32F072 firmware ABI, and changing shared ChibiOS GPIOv2 or suppressing the warning was explicitly outside this task. Successful firmware compilation is not hardware validation; physical NuPhy wiring and wake behavior remain unverified by this investigation.
