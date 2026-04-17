# Nuphy Keyboards

This directory contains the NuPhy keyboards maintained in this tree, along with shared documentation and the common implementation used across the newer refactor path.

## Keyboards

- [Air75 V2 ANSI](./air75v2/ansi/readme.md)
- [Gem80](./gem80/readme.md)
- [Halo75 V2 ANSI](./halo75v2/ansi/readme.md)
- [Halo96 V2 ANSI](./halo96v2/ansi/readme.md)

`Halo96v2` is introduced as a supported keyboard starting with this release line.

## Shared Docs

- [Nuphy keyboard instructions](./instructions.md)
- [Shared common implementation notes](./common/README.md)

## Notes

- VIA JSON files for the supported boards live under each keyboard's `keymaps/default/` directory.
- The shared implementation used by the refactored boards lives in `keyboards/nuphy/common`.
- Board-specific readmes still contain per-keyboard flashing/build details.
