# A second RAZ LTX: same graphics layout, different firmware and artwork

On October 19, 2026, we examined a second unopened RAZ vape with the same Flipper Zero DAP Link and USB-C breakout setup described in the [main guide](README.md). Its stock puff animation differed from the first unit's. Before changing it, we captured its complete 65,536-byte MCU image and 4 MiB external graphics flash, then compared those captures with the **first unit's pre-retheme captures with stock artwork**. This distinction matters: the first unit had subsequently received custom artwork, which was not the comparison baseline.

The exterior and SWD response were not treated as sufficient proof of compatibility. We checked the captured firmware's drawing addresses and frame geometry, the external flash identification and status, and every mapped graphics slot before preparing a write. Neither unit was opened, so physical chip markings and PCB revisions remain unknown. The full factory binaries are not published here.

## What changed in the MCU firmware

| Observation | First unit | Second unit |
| --- | --- | --- |
| MCU capture size | 65,536 bytes | 65,536 bytes |
| MCU SHA-256 | `b8e274b63656f470ace0fe106c618a9963ca91306b999c1be5bd872e39606b11` | `0f97d9d9f4c6d18dbc312bdefd48051b1888e63b81c0b9c1912c343f4abc3c56` |
| SWD debug-port ID | `0x0BB11477` | `0x0BB11477` |
| External-flash RDID | `85 60 16` | `85 60 16` |
| External-flash capacity | 4 MiB | 4 MiB |

The MCU images differ in **176 bytes**. Four one-byte differences change Thumb instructions from `CMP R0,#4` on the first unit to `CMP R0,#2` on the second, at MCU-file offsets `0x4F8E`, `0x5014`, `0x677E`, and `0x67EA`. The relevant routines use the same RAM tick-counter byte at `0x20000027`. The mode-specific animation path at `0x08006770` waits for that counter to pass its comparison before advancing a frame. **Inference:** if the counter advances at the same rate on both units, the second unit advances frames sooner. We have not measured exact on-device playback durations for a controlled comparison.

The other **172 differing bytes** occupy a high MCU-data region spanning `0x82CA` through `0x8E29`. Their role is unresolved. They should not be described as additional animation frames or a known feature change.

The graphics-read routine at `0x080074C8`, the 128 × 160 pixel drawing size, the 42-frame limit per mode, the `0xA000`-byte frame stride, and the Normal/Boost full-screen bank bases `0x09CA68` and `0x240A68` were unchanged. The previously analyzed usage-accounting routines were also byte-identical. These checks, rather than the matching SWD ID alone, supported reusing the [asset map](ASSET-MAP.md) without replacing MCU firmware.

## What changed in the factory graphics flash

The second unit's original 4 MiB graphics capture has SHA-256 `a24256fcb524df8340b12d71280a0118acf16e2a97381b17eafa0eb6ec660589`. Its flash returned status bytes `00 00` and configuration byte `20`, matching the first unit's captured state.

Compared with the first unit's [original capture](EVIDENCE.md), the second has **3,883,847 different bytes across 999 of 1,024 sectors**. Every one of the **186 mapped image slots** differs: 38 settings-screen stills, 64 moving-strip frames, and 84 full-screen puff frames. Those image slots account for 3,883,844 changed bytes. The remaining three changed bytes are in the five-byte saved usage record at `0x3FF000`; the second capture held `3C 02 00 00 BB` (572 raw weighted-usage units). That is **not a puff count**. The [usage-meter analysis](USAGE-METER.md) explains the record's encoding and why its value can change with ordinary use.

No differing bytes were found outside the mapped image slots and that saved record. The artwork was substantially different, but the observed storage layout stayed fixed. This is a finding about these two captures, not a guarantee for another unit with the same case.

## Epsilon installation on the second unit

We built the Epsilon image from the **second unit's own backup**, changing only the mapped graphics payloads and preserving the usage record and unclassified flash contents. The installation used two bounded stages:

1. The 84 full-screen Normal/Boost frames changed 841 sectors (`156`–`996`). Each sector was read back after writing, with no recovery events. A puff then showed Epsilon artwork, and the button responded normally.
2. The complete menu changed 102 more assets (38 stills and 64 moving-strip frames) across 159 sectors (`0`–`156` and `996`–`997`). Each passed live readback with no recovery events. The candidate preserved the newly installed puff-animation banks and usage record byte-for-byte.

The writer restored the saved MCU SRAM and registers and resumed the processor after each stage. The owner confirmed the Epsilon menu in Normal and Boost, including moving strips, gauges, tiles, and brightness icons; the puff animation still displayed Epsilon and the button behaved normally. The write targeted the **external graphics flash**, not the MCU firmware. There was no independent second whole-chip dump after installation, so verification rests on full-sector live readbacks, complete journals, and those on-device checks.

For another vape, make a fresh backup and recheck its firmware fingerprint, descriptors, flash identification, status, and live sector bytes. The original guide's hashes and this addendum's hashes identify captured units; they are not universal expected values.
