# Multieffect Daisy

STM32 (Daisy Seed) port of the [Multieffect](https://github.com/kamilkopyra/digitalMultieffect)
guitar multi-effect engine. Independent repo from the PC/Qt version — this is
where the embedded rewrite lives.

## Status

Toolchain bring-up done: built and flashed `libDaisy`/`DaisySP` and confirmed
two stock examples running on real hardware (`Blink`, `seed/bypass` audio
passthrough). Actual effect porting hasn't started yet.

## Dependencies (not committed, fetch separately)

[DaisyExamples](https://github.com/electro-smith/DaisyExamples) (which nests
`libDaisy` and `DaisySP`) is vendor code, intentionally left out of this repo.
Fetch it with:

```sh
git submodule add https://github.com/electro-smith/DaisyExamples.git daisy/DaisyExamples
cd daisy/DaisyExamples
git submodule update --init --recursive
```

Build `libDaisy`/`DaisySP` (needs `arm-none-eabi-gcc` and a `make`, e.g.
`mingw32-make` on Windows):

```sh
cd daisy/DaisyExamples/libDaisy && mingw32-make -f Makefile MAKE=mingw32-make -j4
cd ../DaisySP && mingw32-make -f Makefile MAKE=mingw32-make -j4
```

## Flashing

Put the board in DFU mode (hold **BOOT**, tap **RESET**, release **BOOT**),
then:

```sh
./daisy/tools/flash.sh path/to/firmware.bin
```
