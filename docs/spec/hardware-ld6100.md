<!-- SPDX-License-Identifier: MIT -->
<!-- Copyright (c) 2026 The RadioFoundry Authors -->

# LD6100 preliminary feasibility research

Status: preliminary research, updated 2026-11-20. This is not a complete hardware specification or a supported-radio
claim. It records the supplied firmware's identity and the owner's teardown observations. The physical observations
have not yet been independently checked against schematics, pin measurements, or operation on the radio.

## 1. Finding

**The owner's teardown identifies an HR_C7000 and FD6818, so the C7000 port path is now the primary target.** The
firmware starts with `2C 11 01 C0`, matching the application-entry prefix documented for an HR_C7000 radio in
[hardware-db25d.md](hardware-db25d.md) 2. This is corroborating evidence, not a substitute for the teardown
observation or a full board map.

Under these observations, the port has a promising reuse path through `platform/hrc7000` and
`drivers/modem/hrc7000`. The reported FD6818 also has driver code in the repository. Board-specific RF setup, display,
keypad, audio, flash, boot/update and calibration work remain. The reported color landscape display and
numeric/function keys can use the shared display and input abstractions once their hardware interfaces are known.
UHF/VHF operation is also reported.

The owner's observations also identify two 8-pin flash packages, marked `25Q128ESIG` and `25Q16ESIG` (one `1`/`I`
character is unclear), plus other unidentified packages. Their roles are not yet established. The GPS module is not
populated; its board location may allow a later modification, but it is not a current GPS capability. No LD6100 board,
registry entry, or bring-up record exists in the current tree.

### Owner-reported teardown observations

The owner reports these markings and physical observations from dismantling the radio. They are not yet project
measurements or confirmed functional assignments.

| Item | Reported information | Still unknown |
|---|---|---|
| DMR SoC/modem | HR_C7000 marking observed | Exact package variant, memory map, clock and board connections |
| RF transceiver | FD6818 marking observed | Exact variant, bus, wiring, RF paths, board-specific register profile and calibration |
| Flash packages | `25Q128ESIG`: SPI, 128 Mbit (16 MiB), rated clock 133 MHz; `25Q16ESIG`: SPI, 16 Mbit (2 MiB), rated clock 133 MHz (owner-provided part information; one marking character may be `1` or `I`) | Which device is XIP program flash versus data storage; actual bus clock, chip-select wiring, erase geometry and protected/calibration ranges |
| Other packages | `5802N` (24-pin), `TDA2822G`, and `SLM680G` beside the display flex | Their roles and connections. `5802N` may be an RDA5802N marking; `TDA2822G` may be an audio amplifier; neither assignment is confirmed |
| Display | Color, landscape; flex marked `ZYE018QQ22-S`, `BCX`, and `2018.06.07`; nearby IC marked `SLM680G` with `2020` below | Resolution, controller identity, bus, pinout, color depth and initialization requirements; whether nearby markings are model/date codes |
| Controls | Numeric keys and function keys | Key matrix/direct GPIO wiring, key labels and mapping |
| Bands | UHF and VHF | Exact RX/TX ranges, power levels and band-specific RF paths |
| Audio | 8-ohm, 2-watt speaker | Amplifier and codec routing, gain, mute and control connections |
| GPS | No GPS module populated; a board location is available for a possible later modification | Whether pads, power, UART and antenna path are routed and usable; module compatibility and firmware support |
| Programming | CH340 USB-serial K-head cable; owner reports holding SK1 + PTT1 + PTT2 while powering on enters software-update mode. The interface/data lines are reported to match the DM-32 | Exact IAP protocol, key timing, baud rate, pinout and loader identity; electrical similarity does not by itself prove protocol compatibility |
| IAPTool write profile | `Name=Program`, `Addr=0x0300C000`, `Length=0`, `Select=1`, `IsErase=1`, `Device=MCU_HOST`, `Operate=WRITE` (user-provided) | Meaning of `Length=0`, effective erase range, page size, handshake and checksum |
| Recovery | Owner reports the original firmware can be restored/flashed with the factory process | The project has not verified recovery after an interrupted or invalid application write |
| First test image | Boot and debug link only; RF transmit disabled | Debug UART pins/baud, board power behavior and safe RF-off pin state |

### 1.1 Board identification from teardown photos

The owner-provided PCB photos show the SoC package marked `HR_C7000`. The visible board marking is `LD-504D`,
`20210303`; a sticker reads `LD6100# 22021400282`. These are photo observations, not a complete schematic or revision
history.

## 2. Firmware file examined

The local file is `codecs/LD6100_V1.01.02.0010.bin`.

| Property | Observation |
|---|---|
| Name/version | The filename identifies it as `LD6100_V1.01.02.0010`; the version has not been authenticated against the image or a vendor source |
| Size | 725,656 bytes (0xB1298) |
| SHA-256 | `7a6a2b75d5c9067a51074d0f940be4e234a4c44adb38081127cbcc7dd234f72f` |
| First four bytes | `2C 11 01 C0`; matches the HR_C7000 application-entry prefix documented in [hardware-db25d.md](hardware-db25d.md) 2, but is not conclusive identification |
| Generic file identification | `file` reports `data`; this does not establish an image format, architecture, load address, or encryption state |
| Limited printable-data check | The tested model and familiar chip-family labels were not found. This narrow check cannot prove that the image contains no such information |

### 2.1 Static image findings with the HR_C7000 teardown observation

The first four bytes are the strongest architecture clue found so far. The image does not begin with the common ELF,
ZIP, GZIP, or LZ4-frame signatures checked. Its prefix is therefore **consistent with** a raw HR_C7000 application
image, but this check is not exhaustive and does not establish that the file has no vendor-specific wrapper or that it
is linked for a particular address.

The file has 6,146 trailing zero bytes; the last nonzero byte is at offset 719,509. This is an observed byte pattern,
not proof that the zeros are padding or that the image is complete.

For a size comparison only, the DM-32's HR_C7000 layout places the application at `0x0300C000` and the codec slot at
`0x030E8000` ([hardware-dm32.md](hardware-dm32.md) 2). That leaves 901,120 bytes for the application. If the LD6100
file is a raw image linked at the same address and uses the same slot boundary, its 725,656 bytes would end at
`0x030BD298`, leaving 175,464 bytes before the slot. This is a plausible fit for that particular layout, not evidence
that the LD6100 has a 1 MB program flash, the same codec placement, or the same load address.

**Progress assessment:** the owner's teardown identifies HR_C7000 and FD6818, while the firmware prefix agrees with an
existing HR_C7000 image signature. The C7000 platform is therefore the primary implementation target. The two flash
markings may indicate enough program storage for the observed image, but neither their exact identities nor their
roles have been verified.

The firmware inspection alone is insufficient to establish the memory map, bootloader, codec location, or peripheral
wiring. The HR_C7000 and FD6818 identification above comes from the owner's teardown, not from claims about the
firmware's internals. No disassembly, code reconstruction, codec extraction, or implementation based on firmware
internals was performed. The hash identifies this particular local file; it does not prove its origin or authenticity.

The file is ignored by `codecs/.gitignore` and is not tracked. Keep the vendor image, any radio readout, extracted
codec, and merged image local; do not commit or publish them.

### 2.2 Serial-IAP and Web flasher fit

The project already maps the `serial-iap` flash family to the HR_C7000 Web Serial backend. Its shared
[IAP writer](../../site/static/js/flasher/iap.js) erases the application range, writes 2 KB pages, and checks the
loader's additive checksum. The owner requests that the LD6100 reuse this backend and reports that its programming
interface/data lines match the DM-32. The DM-32 documentation describes its K-head cable as UART0 at 115200 baud and
its updater as the Baofeng serial-loader route ([hardware-dm32.md](hardware-dm32.md) 3.1, 13.2); the repository also
has a distinct `serial-iap` route used by the DB25-D. Reusing the physical cable/UART path does not prove that the
LD6100 uses the same byte-level IAP protocol. A read-only probe of IAPTool mode must confirm the loader replies before
the Web IAP writer is enabled. The Web Serial layer recognizes CH340 adapters, but the project warns that some CH340
cables fail backup or install operations; this particular cable still needs testing.

#### Read-only IAP identification procedure

1. With the radio off, hold SK1 + PTT1 + PTT2 while powering on, then connect/use the CH340 cable as required by the
   factory tool. Passive capture while entering this mode does not require sending a command.
2. If the factory IAPTool offers a clearly labeled **Identify**, **Version**, or **Read** operation, use only that
   read-only operation, and only after preserving a verified stock backup. The supplied profile has `IsErase=1` and
   `Operate=WRITE`; do not execute it for protocol identification. If no verified backup exists or the tool's
   read-only behavior is unclear, stop before pressing its action button.
3. Optionally capture the CH340 USB traffic with USBPcap/Wireshark, starting capture before opening IAPTool. This is
   passive observation; do not inject serial bytes or run a write/erase command. Keep the capture local because an
   accidental write session could contain firmware data.
4. Compare the host-to-radio and radio-to-host bytes with the project's DB25-D IAP protocol description in
   [hardware-db25d.md](hardware-db25d.md) 7: an IAP loader identifies itself with `IAPReady`; the known read request
   starts with ASCII `Flash Read ` followed by a 32-bit big-endian address and page count. Its response includes a
   loader-version string, page data, and `ChecksumR`. A matching banner alone is not enough; confirm a complete
   read-only request/response and verify that no `Erase` request was sent.
5. The DB25-D project's read request changes nothing on that radio, but LD6100 behavior is not yet verified. Do not
   manually send even the known read packet to the LD6100 until the factory tool's read-only exchange or the owner
   confirms that query is safe in IAP mode. Never use `Addr=0x0300C000, IsErase=1` as a probe.

The current Web flasher's IAP install is not a direct write of an arbitrary `.bin`. It validates a combined image
containing a board-specific first stage followed by a RadioFoundry application, and the registry gets the first-stage
size and signature from the board build. The DB25-D profile is one example: it uses a 4 KB first stage and signature
`2C 11 01 C0` ([hardware-db25d.md](hardware-db25d.md) 2). The LD6100 prefix makes that signature a candidate to
investigate, but its first-stage size, loader checks, erase range and memory map must be verified before adopting that
profile.

The reported entry combination is SK1 + PTT1 + PTT2 held while powering on, using the CH340 K-head cable. A user-provided
IAPTool write profile names the target `MCU_HOST` at `0x0300C000` and requests erase (`IsErase=1`). This independently
supports `0x0300C000` as the application write address used by the factory tool. `Length=0` does not reveal the
effective image length or erase extent. The owner reports that the factory process can restore the original firmware;
the project has not verified that recovery or the precise IAP protocol. A stock-console path, if one exists, needs its
own confirmed protocol. Before the first RadioFoundry write, confirm the restore procedure and exact loader behavior.

#### Backup feasibility and IAPTool on Ubuntu

The owner expects the factory application image to exclude calibration data. That does not establish that the updater's
erase operation preserves calibration: `Addr=0x0300C000` and `IsErase=1` do not specify the actual erase boundaries,
flash-chip target, or protected regions. Do not rely on presumed partition protection; back up all identified
program/data flash and calibration regions before any write.

The project IAP module has a `Flash Read` packet used for a one-page probe, not a general LD6100 backup workflow. On
the measured DB25-D, that loader command reads NAND rather than program flash ([hardware-db25d.md](hardware-db25d.md)
7); this behavior cannot be assumed for the LD6100. A backup script is feasible once LD6100 read semantics are
verified, including command framing, readable address ranges for both flash chips, page size, checksum, and calibration
locations. The resulting script should be read-only, save raw regions separately, and verify lengths and hashes. Those
facts are not yet known, so a script based on the DB25-D packet today could produce a misleading or incomplete backup.

This computer reports Ubuntu 26.04 x86_64. No `IAPTool`/`iaptool` command or Wine executable is present on its PATH.
The project contains no IAPTool installer, and an official Linux build/download source has not been verified. This does
not rule out a vendor-provided Windows tool or a copy supplied by the owner. The project's Web Serial flasher can run
in Chromium on Linux, but there is no LD6100 flasher profile or validated LD6100 protocol yet; it is not currently a
backup option for this radio.

The two SPI flash capacities make a 2 MiB program flash and 16 MiB data flash a plausible arrangement, but the
assignment is unconfirmed. The supplied 725,656-byte image fits within 2 MiB; that size fit does not prove which chip
stores it or the erase geometry. The stated 133 MHz is each flash part's rated maximum, not a measured board clock.

The selected first test is a boot/debug-link image with RF transmit disabled. It cannot yet be built as a reliable
radio-specific test image because the debug UART pins and baud, power-latch behavior, and safe RF-off pins are unknown.
The reported side-key + PTT entry and CH340 cable are a useful starting point, but a verified protocol match is needed
before the Web flasher can safely write. The likely reuse is the existing HR_C7000 IAP backend plus LD6100-specific
board metadata and image layout; whether a new first-stage implementation is needed depends on the radio's actual
loader.

No LD6100 firmware image or board-specific Web flasher profile has been generated yet. The pin map, memory ownership,
loader behavior, first-stage requirements and tested recovery route are still gating facts; generating an image before
those are known would risk a non-booting radio or an unrecoverable write.

## 3. Reuse paths in RadioFoundry

The project architecture puts radio-specific wiring in `boards/<id>/`, shared chip logic in `drivers/`, and SoC
support in `platform/`. The core should remain radio-independent. See
[Adding a Radio](../dev/adding-a-radio.md) and [Architecture](../dev/architecture.md).

| If verified on the LD6100 | Likely reuse | Still to verify or implement |
|---|---|---|
| HR_C7000 / C-SKY CK803S SoC and modem (owner-reported teardown) | `platform/hrc7000`, `drivers/modem/hrc7000`, and compatible shared drivers | Exact SoC variant and memory; FD6818 bus and exact variant; pin map; display/input/audio; boot path; calibration and codec hosting |
| A different SoC, modem, or incompatible peripheral set | Generic core and any individually compatible drivers | New platform and/or drivers; scope cannot be estimated until the chips and interfaces are identified |

The repository has FD6818 driver code under `drivers/rf/fd6818/`, including variants and setup profiles used by other
boards. This makes RF-driver reuse a reasonable candidate, but does not prove that the LD6100's FD6818 variant, bus,
register profile, or TX/RX sequence matches an existing board. The actual bus and profile must be established first.

The `5802N` marking may identify an RDA5802N broadcast-FM receiver. The repository has an RDA5807M driver; the
project's [broadcast-radio spec](airband-and-broadcast.md) says it may serve the RDA5802N after a bring-up check, not
that it is already compatible. Keep broadcast FM out of the first test image until the chip identity, bus and audio
route are confirmed. No matching `SLM680G` or `TDA2822G` board integration was found; their roles and connections
remain open.

The display controller and bus are unknown, so a landscape orientation alone does not establish that an existing
display driver fits. The shared UI/input model should accommodate the keypad, but the board still needs a measured key
map. GPS support may be kept as an optional future path in the board design, as requested by the owner; do not report
GPS as a working capability until a module is installed and its power, UART and antenna path are verified.

These are conditional reuse options, not confirmed LD6100 hardware support. Similar firmware size or a version string
is not evidence of a shared platform.

## 4. Evidence needed before implementation

| Question | Minimum useful evidence |
|---|---|
| Exact model and revision | Radio label, manufacturer, FCC ID or equivalent filing, hardware revision, and the intended supported variant |
| MCU / SoC and DMR modem | Record a clear HR_C7000 package marking; identify the exact variant, clocks and memory map from public documentation and non-destructive measurements |
| RF chain | Confirm the FD6818 variant; identify synthesizer, IF and PA parts; establish exact bands, power and receive/transmit paths from public documents or measurements |
| Display and controls | Identify the `SLM680G` function and panel/controller; establish dimensions, bus and pinout; map numeric/function keys, LEDs, power switch and battery sensing |
| Memory and calibration | Confirm both SPI flash part markings and capacities, identify program/data roles, then establish memory protection, actual bus clock, erase geometry, calibration regions and non-destructive backup coverage |
| Boot and recovery | Confirm the SK1 + PTT1 + PTT2 update entry, K-head loader protocol with a read-only probe, DM-32-compatible interface/pinout and baud; establish full backup coverage, test restore with the original updater, and establish image format, application load address, erase range and recovery route |
| GPS option | Check whether the unpopulated module location has connected power, UART, antenna path and suitable pads; do not enable GPS based on the footprint alone |
| Audio and codec | Audio routing and the project's supported local codec-hosting path, established without committing or sharing codec data |
| Operating behavior | Owner-authorized measurements on the radio; safe receive tests first and transmit tests only on an appropriate bench setup |

Do not flash the supplied image or attempt a write based on its file size. Before any porting or update experiment, establish
a verified recovery route and make complete, non-destructive backups of relevant memories.

## 5. Recommended research and bring-up order

1. Use HR_C7000 as the target and aim first for a boot/debug-link image with RF transmit disabled. Confirm the model,
   board revision and full SoC marking against clear PCB photos, and find any applicable public manual or regulatory
   filing. Revisit the platform choice only if new evidence contradicts the teardown observation.
2. Identify the SoC, modem, RF parts, memories, display, and buses from markings, public datasheets, and measurements.
   Record unknowns explicitly; do not infer pins or register behavior from vendor code.
3. Before any write, confirm the K-head loader protocol and SK1 + PTT1 + PTT2 entry steps, make a complete backup where possible, and
   verify that the original updater can restore the stock image. Do not treat the reported but untested recovery route
   as proven.
4. Document reset, boot, recovery, update transport, memory map, calibration, and codec hosting in this hardware page.
   Keep observations and sources separate from hypotheses.
5. Compare the verified hardware against the reuse paths in section 3. Only then create `boards/ld6100/`, select a
   platform, and add board-specific files or shared drivers as needed. Put new flash-costing features behind the
   appropriate `CONFIG_` switches.
6. Follow the project bring-up sequence: platform start-up and logging; display/input; debug link; flash and
   calibration; FM receive; FM transmit; DMR and codec; other modes; then CPS and install/update support. Add host tests
   for new logic and HIL coverage for hardware behavior that can be scripted. See
   [Adding a Radio](../dev/adding-a-radio.md) and [DM-32UV bring-up](../testing/dm32-bringup.md) for project examples.
7. Do not touch connected radio hardware or transmit without the owner's authorization. RF tests require an
   appropriate dummy load or bench setup and must follow the project's `--allow-rf` safeguards.

## 6. Clean-room boundary

This note records file identity and feasibility questions; it does not reproduce vendor code, function structure,
disassembly, comments, menus, or proprietary tables. Hardware facts for a future implementation must come from
measurements, public datasheets, manuals, and standards, following [CLEANROOM.md](../CLEANROOM.md). Do not add the
stock image or codec to Git or share them in a report or pull request.
