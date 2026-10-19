# Capture and verification record

Tested October 18, 2026 on one unopened device showing `GV2025 / 0210V9`. This record describes an earlier hardware-tested custom theme. The Epsilon artwork was an offline example at the time of this test; it was subsequently installed on a second device, documented in the [second-device addendum](SECOND-DEVICE.md). Project-specific names and log prefixes are normalized to `THEME`; numerical evidence is preserved.

## Captures and build

| Artifact | Bytes | SHA-256 |
| --- | --- | --- |
| Internal MCU capture, two matching reads | 65,536 | `b8e274b63656f470ace0fe106c618a9963ca91306b999c1be5bd872e39606b11` |
| Original external graphics capture | 4,194,304 | `9ecfcea7192b4ae4c13aeabde0dda018651fac7f3e5d66703650609f7277f46b` |
| Initial hardware-tested custom-theme candidate | 4,194,304 | `50f3ab970717de95936c0a57533d7574776d626c7a0ee3f82c846f336579f7c6` |
| Extracted SRAM helper `.text` | 528 | `c5e493b5c51c251e9b7fee494b448bee17b2d9a9aeb00334d85852472b2a81c1` |
| Erased 4 KiB scratch-sector contents | 4,096 | `f47a8ec3e9aff2318d896942282ad4fe37d6391c82914f54a5da8a37de1300c6` |

Hashes identify our artifacts. Neither a graphics backup hash nor a theme candidate is a universal device signature. The original binaries and full candidate are kept privately and are not in this documentation package.

The 4 MiB capture matched a previous 64 KiB pilot over its entire length and a 2 KiB sample at `0x000A1A68`. The initial 16-byte sample there was:

```text
B7 FE B7 DE B7 DE B7 DE B7 DE B7 DE B7 DE B7 DE
```

JEDEC/SFDP identified the P25Q32SH family, with package suffix unconfirmed. See [the raw identification capture](evidence/flash-sfdp.txt).

## Progression of live writes

| Stage | Evidence | Owner observation |
| --- | --- | --- |
| 16-byte scratch test at `0x3F0000` | Program/readback, erase, separate full-sector re-read matched original | Usual screen/button behavior |
| 4 KiB scratch round trip | `SECTOR_PILOT_PREFLIGHT_OK`, `SECTOR_PILOT_PROGRAM_VERIFIED`, `SECTOR_PILOT_RESTORED`; full before/pattern/restore comparisons | Normal response |
| First artwork sector | `THEME_FULL_BATCH_COMPLETE writes=1`, complete readback | Normal response, partial theme expected |
| Remaining artwork sectors | `THEME_FULL_COMPLETE writes=997`, exit 0, complete per-sector readbacks | Custom screen appeared; normal response |
| Theme acceptance | Owner checked animation and Normal/Boost states | Animation and matching mode borders looked correct |
| Later indicator correction | Six sectors, complete per-sector readbacks, exit 0 | Corrected ring looked right; normal response |

The first 4 KiB pilot invocation failed on an unsupported Jim Tcl file option before OpenOCD `init`, so that invocation did not change the device. The successful corrected pilot is the one recorded above.

The final candidate contains 186 bitmap replacements and differs from the original in 4,072,283 bytes across 998 four-kilobyte sectors. An offline audit recomputed the changed-sector set and confirmed exactly 998 unique `START` and 998 unique `VERIFIED` journal entries, no duplicates, and zero `RESTORED` entries. The included [journal](evidence/sector-journal.txt) and [audit JSON](evidence/verification.json) capture that accounting.

All candidate differences fall inside the mapped image range. The first eight bytes, the area after `0x3E5C68`, the scratch sector, and the final sector were preserved in the candidate. This is a host comparison of original versus candidate. The live write verification was each changed sector's complete readback against that candidate.

## Later offline analysis of the usage meter

The [usage-meter report](USAGE-METER.md) traces the captured MCU firmware's active-puff timer, Normal/Boost accounting, liquid-icon selection, and five-byte persistent record. The input firmware and graphics-dump hashes are the same original capture hashes listed above. The state-dispatch table resolves the previously unexported active-puff handler at `0x08007958`.

Direct checks against the saved bytes confirmed the Boost double constant of 1.67 at MCU address `0x08007A60`, selector constants 340000 and 60000, and external record `59 B2 02 00 BB` at `0x3FF000`. The record decodes to 176729 weighted usage units and selects liquid stage 4. These are archived bytes, not a new or current device reading. The timer and completed-puff code explain why the total depends on duration and mode rather than a fixed allowance for every puff.

Ghidra opened the existing Cortex/Thumb analysis project in read-only mode and discarded temporary analysis changes. No live device was accessed and no counter, firmware or graphics data was written. Timed-puff behavior and real liquid-volume accuracy have not been measured independently. A separate 16-bit event-counter candidate remains unverified as a lifetime puff count.

## Limits of this evidence

- This original test covered one device and one captured firmware revision; the later [second-device addendum](SECOND-DEVICE.md) has its own comparison and installation record. Exterior product identification does not confirm physical board revisions or component package markings.
- The owner checked the earlier theme's behavior and appearance. Published Epsilon previews are host-generated, not photos of that first result. Epsilon's offline validation record is [separate](evidence/epsilon-v2-offline-verification.json), as is its later [second-device installation](SECOND-DEVICE.md).
- No independent second full-chip dump or post-write MCU firmware dump was made after programming. The method wrote only external flash, and every changed sector was read back, but those separate whole-device checks were not performed.
- The later usage analysis identifies the five-byte record at `0x3FF000`; the rest of the unclassified external-flash bytes remain incompletely understood. The retheme preserved those regions rather than calling them unused.
- The scratch sector was erased in this capture and checked live before use; that does not prove another device never uses it.
- Simulated recovery tests validate controller branches, not immunity to real cable/power faults. The journal and rollback are best effort.
- Ordinary operation after flashing was checked; long-term reliability and additional hardware revisions were not tested.
