# Six-sector liquid-indicator update

After the full custom-theme transfer, we corrected its liquid-ring direction to match the original design: counterclockwise from the upper-left. This was a hardware update to the earlier theme. The Epsilon v2 images published here incorporate the same direction. They were validated offline as a separate payload before their later [installation on a second device](SECOND-DEVICE.md).

## Scope and evidence

Ten intermediate images changed: stages 1 through 5 for Normal and Boost. Empty/full endpoints and all other artwork stayed unchanged. The correction changed **1,552 bytes** in the earlier theme's candidate and affected six erase sectors:

| Sector index | Start | Exclusive end |
| --- | --- | --- |
| 26 | `0x01A000` | `0x01B000` |
| 27 | `0x01B000` | `0x01C000` |
| 28 | `0x01C000` | `0x01D000` |
| 29 | `0x01D000` | `0x01E000` |
| 30 | `0x01E000` | `0x01F000` |
| 31 | `0x01F000` | `0x020000` |

The sector payloads total 24,576 bytes because erase/program operations work on complete 4 KiB sectors. All neighboring bytes were preserved from the installed reference. We compared live bytes with that reference before each write, read back every full sector afterward, restored MCU SRAM/registers, and resumed execution. Initial button taps established the held debug connection; no repeated tapping was required during the update.

The [normalized journal](evidence/selective-update-journal.txt) records six `START` and six `VERIFIED` entries, no restores. The [verification summary](evidence/selective-update-verification.json) retains the candidate hashes and affected slots. The run exited successfully, and an independent host comparison of the retained final-sector readback matched its candidate. A second whole-chip read was not performed.

The owner checked the settings screen and corrected ring afterward and reported: **"Looks right; responds normally."**

## Keeping the two artwork records separate

The hardware-tested correction changed 1,552 pixel bytes. The [Epsilon v2 offline correction](evidence/epsilon-v2-offline-verification.json) changed 2,100 bytes compared with Epsilon v1. Both touch the same ten image slots and six sectors, but their pixel content differs. The old candidate hashes, 998-sector full transfer, and six-sector live update are not Epsilon hardware verification; the [second-device record](SECOND-DEVICE.md) documents that separate installation.
