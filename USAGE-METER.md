# How this firmware estimates remaining liquid

On the captured **GV2025 / 0210V9** firmware, the liquid indicator is driven by **accumulated firing duration**, with a **1.67 multiplier in Boost**. It does not simply subtract the same allowance for every puff. A longer puff contributes more estimated consumption than a shorter puff in the same mode.

This is a software estimate of liquid use. The traced gauge calculation does not read a liquid-level sensor. This finding describes the captured firmware and does not establish the hardware or behavior of every RAZ model.

## Evidence and scope

The analysis used the previously captured binaries documented in [EVIDENCE.md](EVIDENCE.md):

| Input | SHA-256 |
| --- | --- |
| 65,536-byte MCU firmware | `b8e274b63656f470ace0fe106c618a9963ca91306b999c1be5bd872e39606b11` |
| Original 4 MiB external-flash dump | `9ecfcea7192b4ae4c13aeabde0dda018651fac7f3e5d66703650609f7277f46b` |

Ghidra 12.1.4 decoded the saved MCU image as Cortex/Thumb at `0x08000000`. The state-dispatch table, timer interrupt, completed-puff handler, gauge selector, and flash save/restore routines were followed together. The source hashes, floating-point multiplier, integer thresholds, and archived five-byte record were checked against the binary bytes.

This investigation was **offline**. It did not reconnect to the vape, change any counters, or write firmware or graphics. The measured-duration response has not been independently tested with timed live puffs, and the estimate has not been calibrated against actual liquid volume.

## From firing time to a usage total

The dispatch table at `0x08007E74` selects the active-puff handler at `0x08007958` for state 3. The TIM6 interrupt at `0x080035F4` increments the active-firing timer only while bit 5 at `0x20000019` is set. That increment occurs after a divide-by-two and divide-by-ten software prescaler in the interrupt path.

At the end of an ordinary puff, the active handler copies the measured duration to a completed-duration field and adds it to the cumulative usage total. The mode byte is `0` for Normal and nonzero for Boost. In simplified pseudocode, with duration expressed in the firmware's timer units:

```text
duration = completed_firing_ticks
if mode == NORMAL:
    usage += duration
else:
    usage += integer(duration * 1.67)
```

The Boost constant is the IEEE-754 double at `0x08007A60`. The multiplication helper is `0x08000458`, followed by integer conversion at `0x0800065C`. Integer conversion means very short durations need not have an exact 1.67 ratio.

The long-puff timeout path independently uses the same weighting: it adds 1000 usage units in Normal or 1670 (`0x686`) in Boost before clearing the active timer and changing state. This is the cutoff branch, not an additional fixed charge applied to every ordinary puff.

For example, a four-second Normal puff contributes approximately twice as much estimated usage as a two-second Normal puff. Equal firing time in Boost contributes about 67% more than Normal. These are deductions from the firmware's accounting rule, not measurements of how much liquid those draws physically consume. Timer units are kept distinct from milliseconds here; a live timing calibration has not been performed.

| Value in this firmware | MCU SRAM address |
| --- | --- |
| Active-firing timer, 16-bit | `0x20000124` |
| Completed-puff duration, 16-bit | `0x20000126` |
| Cumulative weighted usage, 32-bit | `0x20000144` |
| Mode byte, Normal `0` / Boost nonzero | `0x20000150` |

These are revision-specific addresses, not portable commands or a general device memory map.

## Choosing the liquid-ring image

The selector at `0x080015A0` calculates a stage from the cumulative usage total:

```text
if usage < 340000:
    stage = 6 - floor(usage / 60000)
else:
    stage = 0
```

The status path then passes the stage to `0x08001D30`, which chooses the droplet descriptor from the table at `0x08007F20`. The images are the seven liquid-gauge stages per mode in the [asset map](ASSET-MAP.md).

| Weighted usage units | Selected liquid stage |
| --- | --- |
| 0-59,999 | 6 |
| 60,000-119,999 | 5 |
| 120,000-179,999 | 4 |
| 180,000-239,999 | 3 |
| 240,000-299,999 | 2 |
| 300,000-339,999 | 1 |
| 340,000 or more | 0 |

These are icon-selection thresholds, not measured milliliters, advertised puff counts, or a guarantee that the reservoir is physically empty at stage 0. Replacing the icon pixels changes their appearance without changing this calculation.

## The saved record at the end of graphics flash

The save routine at `0x08005388` erases the sector at external-flash offset **`0x3FF000`** and writes a five-byte record there. The restore routine at `0x08004850` reads it back:

| Record offset | Meaning |
| --- | --- |
| `+0..+3` | 32-bit usage total, little-endian |
| `+4` | Validity marker `0xBB` |

With a valid marker, startup restores the usage total from those four bytes. An invalid marker takes the zero-initialization branch. This explains the five non-`FF` bytes in the final sector that the original retheme deliberately preserved: this part of the chip holds changing device state as well as artwork.

The original archived dump contains:

```text
External flash 0x3FF000: 59 B2 02 00 BB
Usage total:           176729 units
Calculated icon:       stage 4
```

That value is from the original capture, **not a current reading and not 176729 puffs**. It cannot be converted into an exact lifetime puff count because different durations and modes contribute different amounts. Current SRAM and the last saved flash record should also be recorded separately rather than assumed identical.

The existing graphics-only update range ends at `0x3E5C68`, well before this record. Preserving the complete unedited tail remains necessary; identifying one record does not make the rest of that tail disposable. No counter-reset or counter-edit procedure is supplied here.

## What remains unverified

- `0x08003B9C` increments a separate 16-bit event-counter candidate at `0x20000164` when a qualifying draw begins. Its reset behavior, persistence, treatment of failed draws, and lifetime meaning have not been established. It should not be presented as a verified lifetime puff counter.
- No exact lifetime puff total was recovered. Advertised Normal/Boost puff ratings are product specifications, not values decoded from this usage record.
- A future read-only capture can record the live usage total, mode, saved flash record, and candidate event-counter value. That capture has not happened yet.
- Physical component inspection, accuracy against actual liquid volume, and behavior on other firmware revisions remain open.
