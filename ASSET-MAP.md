# Graphics map for GV2025 / 0210V9

All addresses in this document are offsets into the external 4 MiB flash, except addresses explicitly described as MCU addresses. All ends are **exclusive**. Bitmaps are contiguous, row-major, big-endian RGB565 with no per-image file header.

## Storage ranges

| Group | Start | End | Width × height | Count | Bytes per image |
| --- | --- | --- | --- | --- | --- |
| Normal background | `0x000008` | `0x00A008` | 128 × 160 | 1 | `0xA000` |
| Boost background | `0x00A008` | `0x014008` | 128 × 160 | 1 | `0xA000` |
| Normal power gauges | `0x014008` | `0x017140` | 30 × 30 | 7 | `0x0708` |
| Boost power gauges | `0x017140` | `0x01A278` | 30 × 30 | 7 | `0x0708` |
| Normal juice gauges | `0x01A278` | `0x01D3B0` | 30 × 30 | 7 | `0x0708` |
| Boost juice gauges | `0x01D3B0` | `0x0204E8` | 30 × 30 | 7 | `0x0708` |
| Normal mode panel | `0x0204E8` | `0x023328` | 74 × 80 | 1 | `0x2E40` |
| Boost mode panel | `0x023328` | `0x026168` | 74 × 80 | 1 | `0x2E40` |
| Earlier Normal night icon copy | `0x026168` | `0x0265E8` | 24 × 24 | 1 | `0x0480` |
| Earlier Normal day icon copy | `0x0265E8` | `0x026A68` | 24 × 24 | 1 | `0x0480` |
| Normal strip | `0x026A68` | `0x061A68` | 118 × 32 | 32 | `0x1D80` |
| Boost strip | `0x061A68` | `0x09CA68` | 118 × 32 | 32 | `0x1D80` |
| Full-screen animation bank 1 | `0x09CA68` | `0x240A68` | 128 × 160 | 42 | `0xA000` |
| Full-screen animation bank 2 | `0x240A68` | `0x3E4A68` | 128 × 160 | 42 | `0xA000` |
| Active Normal night icon | `0x3E4A68` | `0x3E4EE8` | 24 × 24 | 1 | `0x0480` |
| Active Normal day icon | `0x3E4EE8` | `0x3E5368` | 24 × 24 | 1 | `0x0480` |
| Active Boost night icon | `0x3E5368` | `0x3E57E8` | 24 × 24 | 1 | `0x0480` |
| Active Boost day icon | `0x3E57E8` | `0x3E5C68` | 24 × 24 | 1 | `0x0480` |

For a sequence, frame `i` starts at `base + i × width × height × 2`, with zero-based indices. The ranges contain 186 bitmaps, including the two earlier duplicate icons. These are the assets mapped for the normal status/vaping paths, not a claim that every possible diagnostic screen is stored here.

Bytes `0x000000..0x000007` and `0x3E5C68..0x3FFFFF` were outside our replacement map and stayed unchanged. Their full meaning was not established. The final sector contained five non-erased bytes. Do not substitute the older 1 MiB model's counter addresses or assume a blank-looking sector is universally disposable.

## Status-screen composition

The MCU path at `0x08002FA0` draws components at these top-left pixel coordinates:

| Component | Position | Descriptor location in MCU |
| --- | --- | --- |
| Whole background | `(0, 0)` | `0x08007FC0`, two entries |
| Mode panel | `(5, 2)` | `0x08007FB0`, two entries |
| Power gauge | `(88, 6)` | `0x08007EB0`, fourteen entries |
| Juice gauge | `(88, 48)` | `0x08007F20`, fourteen entries |
| Moving strip | `(5, 86)` | Bases and frame count recovered from code |
| Night/day icon | `(14, 127)` | `0x08007F90`, four entries |

Each descriptor is eight bytes: little-endian `uint16 width`, `uint16 height`, `uint32 external_flash_offset`. The pixels it describes use a different byte order: big-endian RGB565.

The original backgrounds contain the NORMAL/BOOST labels, fixed `12W`/`24W` text, and bottom setting-tile borders. Gauge stages and the brightness icon are overlaid by the existing firmware. The earlier duplicate Normal icons are identical in the stock capture; the descriptor table points to the later copies. Our theme updated both copies.

## Pixel conversion

For bytes `hi, lo`, form `p = (hi << 8) | lo`. Red is `(p >> 11) & 31`, green `(p >> 5) & 63`, and blue `p & 31`. Expand those channel values for a PNG preview. For replacement RGB bytes, the quantization is:

```text
p = ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3)
output = [p >> 8, p & 0xFF]
```

Useful byte-order checks: red `F8 00`, green `07 E0`, blue `00 1F`. Re-encoding arbitrary RGB888 pixels loses precision; compare at RGB565 precision. A full-screen frame occupies exactly 40,960 bytes.

Build a candidate from a copy of the full device backup, replacing only the intended slices. Keep image dimensions and frame counts unchanged. When an image crosses a sector boundary, preserve the other bytes of both sectors. The firmware controls animation timing; a host-side preview GIF's frame delay does not reconfigure the vape.
