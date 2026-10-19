# Implementation reference snapshots

These files accompany the [writeup](../README.md). The C helper is copied without changes. OpenOCD configuration snapshots have local paths and adapter identity redacted, and project-specific theme paths/log prefixes normalized to `theme` / `THEME`. The `.txt` extension makes their role as inspectable references explicit. The control flow and input hashes are retained from the hardware-tested earlier theme, not an Epsilon flash run.

| File | Role in the tested implementation |
| --- | --- |
| `flipper-vape-id.cfg.txt` | Minimal debug-port identification scan |
| `flipper-vape-flash-dump.cfg.txt` | Chunked read through the original firmware routine |
| `flipper-vape-sector-loader-pilot.cfg.txt` | Shared SPI/read/loader helpers and full-sector scratch pilot |
| `flipper-vape-theme-full-flash.cfg.txt` | Hash-gated per-sector controller, journal, and recovery |
| `flash_sector_loader.c` | Exact SRAM helper source; compiled binary was 528 bytes |

The redacted values are `/PATH/TO/PROJECT` and `YOUR_DAP_SERIAL`. These snapshots retain the `work/` and `outputs/` layout with normalized theme directory names and exact image hashes. Dependent inputs such as the original firmware, graphics backup, candidate, SRAM-loader binary, and scratch patterns are deliberately absent. This directory is not a drop-in command bundle, and the configurations were not live-tested after redaction.

The live project used `run_pilot=1` or `run_full=1` to enable writes; the controller defaulted to one sector. Those controls remain visible in the snapshots. Do not treat changing the flags or removing the hash checks as target validation. Read the [technical notes](../TECHNICAL-NOTES.md), verify the firmware and peripheral map, and retain your own recovery data before adapting the implementation.

The helper's source is project-authored; upstream software and Epsilon artwork retain their respective rights. This publication does not select a new blanket distribution license.
