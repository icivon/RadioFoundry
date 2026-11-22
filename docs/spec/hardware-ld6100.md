<!-- SPDX-License-Identifier: MIT -->
<!-- Copyright (c) 2026 The RadioFoundry Authors -->

# LD6100 preliminary feasibility research

Status: preliminary research, updated 2026-11-22. This is not a complete hardware specification or a supported-radio
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
| Flash backups | Owner reports that both SPI flash chips have been backed up | Record each dump's exact chip, byte length and SHA-256; verify it is a complete raw read and confirm a tested restore path before any write |
| Other packages | `5802N` (24-pin), `TDA2822G`, and `SLM680G` beside the display flex | Their roles and connections. `5802N` may be an RDA5802N marking; `TDA2822G` may be an audio amplifier; neither assignment is confirmed |
| Display | Color, landscape; flex marked `ZYE018QQ22-S`, `BCX`, and `2018.06.07`; nearby IC marked `SLM680G` with `2020` below | Resolution, controller identity, bus, pinout, color depth and initialization requirements; whether nearby markings are model/date codes |
| Controls | Numeric keys and function keys | Key matrix/direct GPIO wiring, key labels and mapping |
| Bands | UHF and VHF | Exact RX/TX ranges, power levels and band-specific RF paths |
| Audio | 8-ohm, 2-watt speaker | Amplifier and codec routing, gain, mute and control connections |
| GPS | No GPS module populated; a board location is available for a possible later modification | Whether pads, power, UART and antenna path are routed and usable; module compatibility and firmware support |
| Programming | CH340 USB-serial K-head cable; owner reports holding SK1 + PTT1 + PTT2 while powering on enters software-update mode. The interface/data lines are reported to match the DM-32 | Exact IAP protocol, key timing, baud rate, pinout and loader identity; electrical similarity does not by itself prove protocol compatibility |
| IAPTool write profile | `Name=Program`, `Addr=0x0300C000`, `Length=0`, `Select=1`, `IsErase=1`, `Device=MCU_HOST`, `Operate=WRITE` (user-provided) | Meaning of `Length=0`, effective erase range, page size, handshake and checksum |
| Recovery | Owner reports the original firmware can be restored/flashed with the factory process | Whether either chip backup was restored and verified on this radio; the project has not verified recovery after an interrupted or invalid application write |
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

### 3.1 Key layout and count: test and adaptation plan

The owner's observations establish that the LD6100 has numeric and function keys, but not their count, labels,
electrical arrangement or exact locations. A different physical layout does not by itself require UI/core changes:
`hal_input_t.scan()` reports board-specific physical key bits, and `board.keymap` maps those bits to logical actions.
The existing input engine handles debounce, short/long presses, repeats and raw PTT edges; the UI acts on logical
actions, not key coordinates. See [Adding a Radio](../dev/adding-a-radio.md) "The key map" and [UI](ui.md) 3.

| Test stage | Method | Pass condition / issue caught |
|---|---|---|
| Inventory and scan map | Photograph the keypad and board; list every printed key, physical location, matrix row/column or direct GPIO, and intended raw bit. In a diagnostic build, press one key at a time and inspect `input.raw_keys` (debug link); also check that release clears the bit | Each physical key yields exactly its documented raw bit, no neighboring key changes, and no stuck bits. Record polarity, repeated scan stability and any shared-line behavior |
| Matrix/electrical behavior | Use the shared `keymatrix` driver when the circuit is a supported matrix; exercise every key plus realistic simultaneous presses and the update-entry chord. Check row/column intersections for ghosting, shorts, or keys that disappear in combinations | All required individual keys are detected; any matrix rollover/ghosting limitation is known. Do not infer the matrix from keypad geometry; confirm it electrically |
| Logical mapping and gestures | Test the exact board `keymap`: short press, long press, repeat for navigation, and down/up edges for PTT/monitor. Exercise digits, side-key assignments and any dedicated keys | Physical labels produce the intended logical actions; PTT releases cleanly; unavailable hardware keys are not advertised or assignable |
| Host input tests | Reuse `keymatrixTest`, `inputTest` and `keyassignTest`. Add a board key-map test only for LD6100-specific mapping invariants or special scan translation; keep it host-runnable | Matrix decoding, debounce/gesture semantics and board key assignment invariants pass without radio hardware |
| UI flows with LD6100 map | Add the verified LD6100 logical map to the UI simulator, then run `uiFlowTest` task flows and screen crawl for its geometry/map pair. Compare to the existing `SIM_MAP_F` few-key fallback if essential actions are absent | Required user tasks remain reachable within expected key presses; no screen depends on a missing QUICK, CONTEXT, digit or directional key |
| On-radio/HIL | With RF transmit inhibited, use the physical keys and debug-link `input.raw_keys` plus visible UI behavior to verify each mapping, debounce, long press and release. Exercise the update-entry combination separately. Review the radio drawing and key labels in the installer/site | The real scanner agrees with the host model, no PTT action transmits, and the drawing/labels identify the correct physical controls |

Adaptation should stay in the narrowest layer:

1. Implement or reuse the board input scanner for the measured electrical wiring: a matrix, direct GPIO keys, or a mixture.
   The project's matrix decoder and examples such as `boards/dm32/input_dm32.c` and
   `boards/dm1801pro/input_dm1801pro.c` can guide the hardware-specific scan. Keep scan bits stable and independent of
   the physical positions printed on the case.
2. In `boards/ld6100/board.c`, map each present key to the shared `input_action_t` actions. Preserve dedicated PTT as
   raw down/up edges. Use the existing short/long actions and key assignment support for available side keys instead
   of adding model-specific behavior to `core/ui`.
3. If the radio has fewer navigation keys, use the project's few-key fallback: Up/Down, Select, Back and PTT remain
   sufficient for the base UI, while optional actions use documented fallbacks. Do not expose assignment slots for
   keys the board does not have. If digits or a key function are absent, verify the affected tasks in flows and add a
   shared generic alternative only if an actual task becomes unreachable.
4. If key coordinates or labels differ, update the LD6100 SVG and tags for visual help and web-flasher key prompts; this
   is separate from changing UI behavior. Run `tools/svg_keys.py tag ld6100` and `tools/svg_keys.py check ld6100` when
   the board artwork and registry entry exist.

The exact LD6100 layout and key map remain unverified. Avoid chord-based shortcuts as a workaround for missing keys
unless a real usability requirement justifies them: they add ambiguity to PTT, key-lock, and update-entry behavior.

### 3.2 Display identification and bring-up plan

The reported `ZYE018QQ22-S` flex marking, landscape orientation, and nearby `SLM680G` marking do not identify the
display controller or its bus. The `SLM680G` package could be part of the panel or its power circuitry; do not assume
it is the LCD controller without a datasheet or trace evidence. Identify the controller and electrical interface
before choosing a driver or sending initialization commands.

| Step | Evidence / test | Decision |
|---|---|---|
| Identify panel and controller | Photograph both sides of the panel/FPC and all nearby IC top markings. Search the exact markings in public panel/controller datasheets or manufacturer documentation. Record active pixel dimensions, supply and logic voltage, and whether the controller is integrated in the glass/FPC | A verified controller and panel datasheet can select a driver family and permitted interface; a flex model string alone is not enough |
| Trace the interface | With the radio powered off, use continuity measurements to map FPC pins to SoC/power components and identify reset, chip select, command/data, clock/write strobe, data lines, read strobe, and backlight. Check for a serial bus versus 8-bit/16-bit 8080, RGB, or another interface. Use high-impedance probes only for live signals and avoid shorting adjacent FPC pins | Establish pinout, active levels, bus width, read capability, timing and voltage levels before board code is written |
| Optional ID read | If the confirmed controller and wiring support reads, use its documented ID register after the documented reset sequence and compare the result with the datasheet. Some panels are write-only or return unreliable IDs; a failed read is inconclusive | Accept ID as corroboration, not as the only identification evidence. The project's existing panel-detection path reads DCS IDs where supported and can report an unknown controller when the result is unusable |
| Passive bus check | A high-impedance logic analyzer on the known bus can confirm whether traffic is serial or parallel and measure signal timing during normal display activity. Do not short pins or inject commands. Use public datasheets to develop the project's own initialization; do not copy undocumented vendor command tables or firmware code | Verify the proposed interface electrically and make sure the SoC pins and bus timing can support it |
| Minimal display test | In an isolated bring-up build, initialize from the public datasheet and draw solid black/white/red/green/blue screens, then corner markers, a grid and a short text line. Check orientation, RGB/BGR order, address offsets, clipping, reset behavior and color depth | The image is stable, correctly oriented and colored across full-screen and partial-row updates; reset and backlight work independently |
| UI integration | Implement board bus glue and `hal_display_t` (`width`, `height`, `bpp`, insets, `init`, `flush`/`flush_indexed`, backlight) while reusing a matching generic driver where verified. Add the measured display geometry to the UI simulator/gallery and review all screens at that class | The frame buffer fits the board RAM; UI text and controls fit the visible area and bezel safe area |

The repository currently has generic `ST7735`/`HX8353`-class drivers for 8080-style and serial buses, and an `ST7789`
driver for an 8-bit 8080 bus. These are candidates only: matching landscape orientation or pixel dimensions is not
proof that the controller, bus, init sequence, offsets, color order or inversion match. If LD6100 uses a different
controller, add a clean-room driver from its public datasheet under `drivers/display/` and keep pin/bus glue in
`boards/ld6100/`.

The display layer tests are `st7735Test` and `st7789Test` in `tests/host/`; they verify command sequences and pixel
transfer behavior but cannot validate LD6100 wiring. The radio-specific bring-up must check the actual bus on hardware.
For a 16-color panel the project can use a 4-bpp indexed frame buffer (`GFX_INDEXED=1`); `GFX_FB_BYTES` and
`GFX_MAX_ROWS` in `board.mk` must match the measured geometry and fit available RAM. A new UI display class also needs
a gallery entry and a UI key-map pair in the host flows ([Adding a Radio](../dev/adding-a-radio.md) "The display",
[Testing](../dev/testing.md) "The UI simulator and the gallery").

These are conditional reuse options, not confirmed LD6100 hardware support. Similar firmware size or a version string
is not evidence of a shared platform.

## 4. Development readiness and remaining evidence

The reported HR_C7000 makes `platform/hrc7000` the expected starting platform, and the repository already defines its
C-SKY toolchain as `csky-elf-gcc` ([Building](../dev/building.md)). Board code can begin now as a compile-only skeleton
and host-testable modules; a successful link is not evidence that the image is safe to flash or will boot on the LD6100.

Before attempting the first on-radio boot, close the following gates:

1. **Backups and recovery:** keep the vendor image and both full SPI dumps offline and separate. Record the chip
   marking, byte length, and SHA-256 for each dump. Confirm that the backups are complete and that the factory tool can
   restore them. A backup is not proven restorable until the tool accepts it and the radio returns to stock operation.
2. **Boot and memory map:** confirm which chip is the executable flash, the actual program/data boundaries, erase
   geometry, application base, first-stage/loader ownership, codec location, and calibration regions. The reported
   factory application address `0x0300C000` is useful evidence but does not define the complete map or a safe install
   image. Do not use the LD6100 backups as write inputs until their format and target chip are established.
3. **Power-on safe state and RF inhibit:** map the reset/power-latch behavior and the FD6818/PA control pins. Implement
   `board_rf_safe()` so it leaves the transmitter disabled before normal initialization and in fault/reset paths. The
   first test image must have RF transmit disabled; if that state cannot be guaranteed from the measured wiring, do
   not flash or power the image on the radio.
4. **Debug and minimum peripherals:** identify the debug UART pins, baud and levels, plus enough clock/SRAM details to
   bring up the SoC and get a reliable log. Then identify the display and keypad wiring for the first interactive test.
   The detailed display and key plans are in sections 3.1 and 3.2.
5. **Build and validation:** install/check `csky-elf-gcc`, create `boards/ld6100/` with its board descriptor, linker
   memory map and build options, then compile the smallest boot/debug image. Add host tests for any new board-independent
   logic and run the board build and its required switch-off configurations before considering a radio test.

RF receive/transmit behavior, calibration writing, codec hosting, CPS and Web IAP are later milestones. They are not
required to prove that the platform starts and the debug link works, and should not be enabled merely to make the first
image feature-complete.

## 4.1 Evidence needed before implementation

| Question | Minimum useful evidence |
|---|---|
| Exact model and revision | Radio label, manufacturer, FCC ID or equivalent filing, hardware revision, and the intended supported variant |
| MCU / SoC and DMR modem | Record a clear HR_C7000 package marking; identify the exact variant, clocks and memory map from public documentation and non-destructive measurements |
| RF chain | Confirm the FD6818 variant; identify synthesizer, IF and PA parts; establish exact bands, power and receive/transmit paths from public documents or measurements |
| Display and controls | Identify the `SLM680G` function and panel/controller; establish dimensions, bus and pinout; map numeric/function keys, LEDs, power switch and battery sensing |
| Memory and calibration | Confirm both SPI flash part markings and capacities, identify program/data roles, record lengths/hashes for the reported full-chip backups, then establish memory protection, actual bus clock, erase geometry and calibration regions |
| Boot and recovery | Confirm the SK1 + PTT1 + PTT2 update entry, K-head loader protocol with a read-only probe, DM-32-compatible interface/pinout and baud; validate factory restore from the backups and establish image format, application load address, erase range and recovery route |
| Safe first boot | Map reset/power-latch and FD6818/PA control wiring, then prove `board_rf_safe()` can keep RF transmit disabled before initialization and through fault/reset paths |
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
3. Before any write, record and verify the full-chip backups, confirm the K-head loader protocol and SK1 + PTT1 + PTT2
   entry steps, and test that the original updater can restore stock operation. Do not treat an untested recovery route
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
