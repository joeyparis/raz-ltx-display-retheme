# Third RAZ: revised display system and operating-code comparison

On October 28, 2026, we captured a third unopened RAZ through the same Flipper DAP Link and USB-C breakout setup. The owner confirmed normal screen/button operation after the pilot and full capture. No flash erase, program or write-enable commands were issued. The capture temporarily used CPU registers, RAM and peripheral accesses, then restored state and resumed the MCU. This addendum compares the three saved factory controller captures and the extracted graphics. The complete factory binaries and stock artwork are retained locally and are not distributed here. The subsequent code comparison was entirely offline. This report separates actual traced behavior from code relocation and from untested interpretations.

The third firmware is primarily a **display-system revision**, with a smaller explicit shutdown change. It is substantially more than replacement artwork, but the inspected operating paths do not reveal a new vaping mode or new button command.

## Changes that affect features or procedures

| Area | First/second devices | Third device | What that means |
| --- | --- | --- | --- |
| Charging while switched off | The traced USB-detection path enters the startup/wake transition. The normal menu already animates its battery ring during charging. | A dedicated off-state screen draws a large animated battery plus a numeric percentage; unplugging clears the screen and backlight. | An additional charging presentation. The battery-percentage calculation existed before; displaying it is new in this path. |
| Five-click-off transition | Existing command clears transition state and screen. | Additionally checks whether firing is active, clears that flag, zeros the heater outputs and resets post-firing state. | An explicit additional heater-stop/reset step in an existing command. This does not establish that the old firmware would leave the heater on; other paths can stop it. |
| Mode-change redraw | Ordinary menu mode changes redraw the complete menu, including its full-screen background. | Ordinary menu mode changes redraw just the mode panel and bottom power tile. Other entry paths still perform a full redraw. | A real rendering-procedure change. Less display traffic/flicker is a plausible benefit, not a measured result. |
| Menu construction | Mode-specific full-screen backgrounds with smaller overlaid icons. | Separate components, including larger battery/liquid gauges and complete brightness/power tiles. Battery, liquid and brightness artwork is shared between modes. | Different asset organization and compositor, not just different addresses. Seven battery/liquid stages and two brightness settings remain. |
| Moving menu strip | Two banks of 32 frames, 118×32; advance when the counter exceeds 6. | Two banks of 29 frames, 120×32; advance when the counter exceeds 4. | Different artwork geometry and pacing. With equal counter cadence, updates occur every five increments rather than seven. Exact FPS is not measured. |
| Full-screen puff animation | Separate Normal/Boost banks of 42 frames each. First-device threshold >4; second-device threshold >2. | One shared 54-frame bank used by both traced puff renderers, threshold >4. | Normal and Boost share puff artwork. There are more unique frames, and a longer cycle is plausible at equal tick cadence. Exact cycle time is unmeasured. |
| Startup LCD identification | Includes an alternative `7C 89 F0` ID branch assigning type 2, plus a default assignment. | Keeps the `33 30 25` / command `D3` path but omits those separate branches. | A reduced startup display-detection procedure. Compatibility with other physical panels is untested. |
| Internal memory and drawing buffers | Earlier image tables and RAM allocations. | Moved tables, changed buffers and added RAM glyph composition for the numeric percentage. | Our earlier firmware helper addresses and theme map cannot be reused blindly. This device requires its own map. |

The off-state here is a software state, not complete electrical power removal. Charging graphics do not establish a change in the charger circuit, current or termination behavior.

The operating-control traces below compare the first and third binaries directly. The second-device baseline comes from its earlier 176-byte comparison; its unresolved data differences are not treated as proof of complete equivalence.

## What appears retained in the inspected operating paths

- **Controls:** the same button-input/debounce structure, mode toggle, brightness event, long-hold information screen and five-click-off command. The same raw hold thresholds remain; they were not converted into measured seconds.
- **Vaping modes and output:** the same Normal/Boost branches, draw-input qualification, inspected PWM targets and cap, and duration-limit structure. No third power mode was found.
- **Brightness:** the same two settings and output policy. The images and refresh machinery changed.
- **Usage/liquid accounting:** duration accumulation remains weighted by mode, with Boost at 1.67×. The liquid-stage selector still uses 60,000-unit steps and the 340,000-unit cutoff. This is not a new per-puff liquid estimator or evidence of a new liquid sensor.
- **Saved usage:** the same external-flash location `0x3FF000`, four-byte little-endian usage value and `BB` marker. RAM addresses moved; the record format did not.
- **State machine:** both still dispatch 15 states. Their individual addresses are mapped in the operating report; equal state count does not prove every state is equivalent.

There are also small fade-counter and wake-flag implementation differences whose user-visible effects are not established. They are recorded in the detailed operating comparison rather than presented as new features.

## Evidence and reproducibility

Controller snapshots are 65,536-byte captured windows mapped at `0x08000000`:

| Device | SHA-256 |
| --- | --- |
| First | `b8e274b63656f470ace0fe106c618a9963ca91306b999c1be5bd872e39606b11` |
| Second | `0f97d9d9f4c6d18dbc312bdefd48051b1888e63b81c0b9c1912c343f4abc3c56` |
| Third | `d2b349becabc85d4b302e697497c94a67125dd8b88944fe6e780dd5a7134d6d9` |

The third controller was verified by three matching captures. Its full 4 MiB graphics backup has SHA-256 `62ccdadd4497b1856ff8ff8e3ee936868129c5b97f00241a8a8846fb902e719d`; its initial 4 KiB also matched an independent pilot. The complete graphics chip was not independently read twice.

Analysis used Ghidra's **ARM:LE:32:Cortex** decoding, explicitly recovered indirect state handlers, resolved literal values from each binary, and traced callers and state transitions. The REA generic-ARM snapshot was not used for Cortex-M semantic claims.

First versus second differs at 176 aligned bytes. First versus third differs at 29,678, but relocation of code, pointers, tables and buffers inflates that count. A normalized-C navigation pass found 257 unique shape matches; **these are review candidates, not 257 proven equivalent procedures**. Different calls and pointed-to data can alter matching-looking code. Likewise, additional recovered functions do not count new features.

### Concrete procedure anchors

| Finding | Original address | Third address / supporting path |
| --- | --- | --- |
| Five-click-off handling | `0x08007D18` | Click grouping `0x08006ABC` → five-click gate `0x08004128` → state 5 `0x08007E64` → state 4. Added heater stop `0x080015D4` and reset `0x08006624`. |
| Off-state charging | State 4 `0x08007CE0`, detection `0x08005458` | State 4 `0x08007E28`, renderer `0x0800547C`, animated battery `0x08001338`, numeric composer `0x08001E58`. |
| Existing percentage computation | `0x08005A30` | `0x08005BA8`; structure byte `+0x0D` remains the source value. |
| Full menu | `0x08002FA0` | `0x08003060`; partial mode renderer `0x080020A0`. |
| Mode-change callers | `0x080071F4`, `0x08007AE8` | `0x08007300`, `0x08007BF0`. |
| Menu-strip progression | `0x08003DD0` | `0x08003E74`. |
| Puff animation | `0x08006770` and the corresponding active renderer | `0x08005044`, `0x08006924`, shared external-flash base `0x124710`. |
| Button scanning | `0x08007800` | `0x08007908`. |
| Timer interrupt | `0x080035F4` | `0x0800367C`. |
| Ordinary completed-puff accounting | `0x08007958` | `0x08007A60`. |
| Usage restore / save | `0x08004850` / `0x08005388` | `0x080048F4` / `0x080053AC`. |
| Startup display identification | `0x08006FD0` | `0x08007104`. |

The [asset-map evidence](evidence/third-device-asset-map.json) supplies each mapped still/sequence's external-flash address, geometry, display position and raw-payload hashes. The [dispatch evidence](evidence/third-device-state-dispatch.json) records all 15 third-device handler pointers. These are derived analysis records, not firmware or bitmap payloads. The original [usage-meter analysis](USAGE-METER.md) and [second-device addendum](SECOND-DEVICE.md) provide the earlier baselines.

## Third-device graphics map

All ends are exclusive, and image data is row-major big-endian RGB565. The traced map includes **33 stills** (including 13 glyphs), **58 menu-strip frames**, **110 charging frames**, and **54 shared puff frames**: 255 mapped stored image positions. A separately decoded alternate Boost panel has no established active reference and is excluded from those counts.

| Start | End | Contents |
| --- | --- | --- |
| `0x000000` | `0x00A000` | Repeating `0003` words; active role unconfirmed |
| `0x00A000` | `0x00F320` | Seven shared 40×38 battery images |
| `0x00F320` | `0x014640` | Seven shared 40×38 liquid images |
| `0x014640` | `0x01A400` | Two 75×80 mode panels |
| `0x01A400` | `0x01BC30` | Two 43×36 brightness tiles |
| `0x01BC30` | `0x01E4B0` | Two 72×36 bottom mode/power tiles |
| `0x01E4B0` | `0x054AB0` | Normal strip: 29 frames, 120×32 |
| `0x054AB0` | `0x08B0B0` | Boost strip: 29 frames, 120×32 |
| `0x08B0B0` | `0x124710` | Ten charging bands × 11 frames, 42×68 |
| `0x124710` | `0x340710` | Shared puff sequence: 54 frames, 128×160 |
| `0x340710` | `0x341374` | Thirteen numeric/symbol glyphs |
| `0x341374` | `0x344254` | Alternate 75×80 Boost panel; active use unconfirmed |
| `0x344254` | `0x3FF000` | Erased `FF` space in this capture |
| `0x3FF000` | `0x3FF005` | Saved usage record |
| `0x3FF005` | `0x400000` | Erased `FF` space in this capture |

The recorded usage bytes were `35 06 00 00 BB`, or **1,589 weighted usage units**, matching the captured RAM accumulator at `0x20000148`. This is neither a puff count nor a measured liquid volume. The raw timer units have not been calibrated to seconds. The final record and all unclassified regions must be preserved when adapting graphics.

## Identity and compatibility boundary

The third device reports the same SWD ID `0x0BB11477` and external-flash RDID `85 60 16`. Its 128-byte SFDP table matches the first device's, consistent with the previously identified Puya P25Q32SH family; the physical suffix remains unknown. Its initial stack pointer is `0x200019D0` instead of `0x20000DC8`, while the reset vector remains `0x080000D9`. These observations do not identify the exact MCU ordering code or establish compatibility by themselves.

The existing first-device reference scripts and Epsilon payload have **not** been validated for this third revision. The third device has only been captured and analyzed; no theme installation was performed on it in this work. Graphics routines, staging buffers, descriptors and replacement payloads need revision-specific mapping before a write. The earlier scratch-sector choice is not a universal free-space guarantee.

## Remaining limits

This is a static comparison of saved artifacts, not a complete reconstruction of the vendor source. Some ADC/interrupt code and floating-point conditions are incompletely recovered. We have **not** established equivalence of every short-circuit, undervoltage, thermal or charge-protection path, exact screen timings, or physical panel compatibility. The exact MCU ordering code remains unknown. An extra decoded Boost panel has no currently traced active reference and is not counted as a new feature.

For theme work, the third vape needs its own 54-frame shared puff bank, 29-frame menu strips and revised component/charging asset map. No firmware transplant is needed merely to replace those graphics, but the old theme image is not a drop-in fit.
