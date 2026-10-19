# Implementation notes for the tested firmware

These addresses were recovered from the captured `0210V9` firmware. Verify the firmware fingerprint and disassembly before using any address on another device. The included code is a research reference, not a validated port to another unit.

## SWD configuration

The successful OpenOCD sessions loaded `interface/cmsis-dap.cfg` and configured:

```tcl
transport select swd
adapter speed 100
reset_config none
gdb_port disabled
tcl_port disabled
telnet_port disabled
swd newdap vape cpu -expected-id 0x0bb11477
dap create vape.dap -chain-position vape.cpu
target create vape.cpu cortex_m -dap vape.dap
```

Adapter selection used the Flipper's `DAP_...` serial. The observed CMSIS-DAP USB VID:PID was `0483:5740`. Seeing the normal Flipper USB/CLI connection is insufficient: DAP Link must be running. The core was configured generically for debugging; no STM32 or other vendor internal-flash programming driver was needed for this method.

A minimal debug-port-only scan from our configuration is included as [a text snapshot](reference/flipper-vape-id.cfg.txt). It does not establish the MCU model, firmware match, or reliable memory access. Use a non-writing identification step before any routine invocation.

## Peripheral addresses

| Purpose | Address / action |
| --- | --- |
| Graphics flash SPI peripheral | SPI2 base `0x40014400` |
| SPI2 status | `0x40014408`, bit 1 TX-ready, bit 0 RX-ready |
| SPI2 data | Halfword access at `0x4001440C` |
| Flash CS assert | GPIOA clear register `0x40010828`, write `0x100` |
| Flash CS release | GPIOA set register `0x40010818`, write `0x100` |
| Display transport | Separate SPI1 peripheral at `0x40012000` |
| Watchdog reload | `0x40003000`, write `0xAAAA` |

The GPIO action selects PA8. The read function at `0x080074C8` drives this same CS and SPI2. Reusing it avoided reconstructing the board's SPI initialization from scratch.

The ID/status/SFDP reads asserted CS, sent the opcode and required address/dummy bytes, clocked response bytes, then released CS. For `0x5A` SFDP, that is a three-byte address followed by one dummy byte. The chip's busy/write-enable/protection state was read before programming; no protection-register change was needed on this unit.

| Command | Read result before writes |
| --- | --- |
| `0x9F` RDID | `85 60 16` |
| `0x05` status 0 | `00`, repeated |
| `0x35` status 1 | `00`, repeated |
| `0x15` configuration | `20` |

The Puya datasheet interpretation used here was WIP/WEL clear, block protection clear, no erase/program failure, and 256-byte program pages. Configuration `0x20` was a drive-strength setting, not a reason to clear protection. The SFDP table additionally reported a `0x20` erase opcode for 4 KiB sectors. Identical RDID alone does not settle package, electrical limits, or every command variant.

## Capture and read sequence

1. Establish SWD during button taps. Set `bp 0x08001948 2 hw`, wait for the graphics routine to halt, then remove the hardware breakpoint.
2. Save r0–r12, SP, LR, PC, xPSR, and PRIMASK before changing them. Save every SRAM byte that the operation will overwrite.
3. Refresh the watchdog frequently. Set PRIMASK to 1 for the controlled call. Write a Thumb `BKPT` instruction (`0xBE00`) into saved SRAM for the return sentinel.
4. Supply r0/r1/r2 and set PC to `0x080074C8`; set LR to the sentinel address with Thumb bit 0 set. Resume and wait for the sentinel halt.
5. Check the stopped PC before consuming the buffer. Dump the returned bytes, confirm the exact chunk length, and refresh the watchdog between transfers.
6. Restore overwritten SRAM and the saved registers, then resume at the original PC. A target reset invalidates the saved execution context; do not blindly restore stale registers into a restarted program.

For the full read, the buffer began at `0x20001000`, chunks were at most `0xF00` bytes, and the sentinel was at `0x20001F00` with LR `0x20001F01`. The saved buffer ends before the sentinel. Firmware read code executes from internal flash and does not overwrite the code used to retrieve data.

For the sector writer/controller, the memory arrangement changed:

| SRAM use | Location |
| --- | --- |
| Resident writer entry | `0x20000000`, 528 bytes |
| Temporary stack | SP `0x20000F00` |
| Return sentinel | `0x20000FF0`, LR `0x20000FF1` |
| Sector staging buffer | `0x20001000..0x20001FFF`, 4 KiB |
| Preservation | All 8 KiB SRAM, saved as two 4 KiB blocks |

Full-sector readbacks used two `0x800`-byte calls to the original read routine, keeping that routine's buffer separate from the low-RAM sentinel. The controller restored all saved SRAM and registers before resuming. These arrangements are specific to the observed RAM and firmware; they are not general Cortex-M scratch addresses.

## SRAM writer

[The C source](reference/flash_sector_loader.c) is the exact source used for the helper. It accepts a sector offset in r0 and `0x20001000` in r1, rejects unaligned/out-of-range sectors, and returns a status in r0. The host is responsible for installing a valid stack/return sentinel, preserving the MCU state, providing recovery data, checking payloads/readback, and validating the device. Calling the C entry alone does not supply those protections.

The helper waits for an idle status, sends WREN `0x06`, checks WEL, erases the selected sector with `0x20`, polls status `0x05`, then programs sixteen 256-byte pages with opcode `0x02`, re-enabling writes for each page. It ends with WRDI `0x04`. The byte-transfer path refreshes the MCU watchdog. Status/timeouts return an error to the host controller.

The build used Homebrew LLVM clang 22.1.8:

```sh
clang -target armv6m-none-eabi -mthumb -Os -ffreestanding -fno-builtin \
  -fno-unwind-tables -fno-asynchronous-unwind-tables \
  -c flash_sector_loader.c -o flash-sector.o
llvm-readelf -r flash-sector.o
llvm-objcopy -O binary --only-section=.text flash-sector.o flash-sector-loader.bin
```

In this particular object, the entry function was at `.text` offset zero, `.text` had no relocations, and the only relocations were in unused unwind metadata. The extracted `.text` was 528 bytes with SHA-256:

```text
c5e493b5c51c251e9b7fee494b448bee17b2d9a9aeb00334d85852472b2a81c1
```

This does not make extracting arbitrary unlinked object code valid. A rebuild must inspect symbol position, relocations, section dependencies, and generated code again; compiler changes may change size and layout. The binary is not bundled here.

## Sector controller and recovery

The controller had explicit enable flags and defaulted to a one-sector batch. It checked exact input hashes/lengths, live scratch contents, flash ID, and status/configuration. Its hardware ID check did not substitute for the earlier firmware analysis; the controller did not independently hash the MCU on every invocation.

For each differing sector it read the live 4 KiB, required original backup bytes or already-verified desired bytes, journaled `START index`, staged the desired bytes, invoked the SRAM helper, read back all 4 KiB, then journaled `VERIFIED index`. Input files were hash-checked before programming; the controller did not separately read SRAM staging back before each invocation. A pending journal entry allowed recovery of an interrupted current sector using its original backup before retrying. Unexplained live data caused a stop. Normal completion restored RAM/registers and resumed.

On a write/readback error, the controller attempted to restore the original current sector and verify it, then restore MCU state. This is best-effort recovery: a lost electrical connection can defeat the recovery operation too. The `START`/`VERIFIED` journal uses ordinary host file writes and is not a power-fail-atomic transaction or an fsync-backed guarantee. Resume skips earlier verified sectors rather than rereading every earlier sector on each batch. The final verification audit matched the complete changed-sector set to the journal.

The scratch round trips preceded the artwork write. The live full run required no recovery. Offline tests exercised deliberately bad readback, interrupted-sector recovery, and resume after a verified batch; they did not simulate every real MCU/peripheral or power-loss behavior.

## Reference versus reusable tooling

The `.cfg.txt` snapshots preserve the tested control flow with host paths/adapter identity redacted and theme paths/log prefixes normalized. They retain the exact backup/candidate hashes of the earlier hardware-tested theme; those hashes do not describe the Epsilon artwork. They are supplied so other researchers can inspect the control flow. A reusable tool would need configuration, firmware verification, reviewed error handling, and validation on its intended target. Do not remove the input checks merely to make an unfamiliar dump pass.
