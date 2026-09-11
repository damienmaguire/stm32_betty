# stm32_betty

Gen 2 Prius (NHW20) Battery ECU on a **ZombieVerter VCU V1.3**.

This software enables the use of 3 × BMW Gen 1 PHEV 16s modules (48s, 26 Ah) as the hybrid battery in a Gen 2 Toyota Prius car.

PHEV fuction via a Prius Plug-In OBC G9090-47040 in progress.

Work in progress as of Sept 2026.

CAUTION : THIS FIRMWARE IS ALMOST ENTIRELY AI WRITTEN AND HAS NOT AS YET BEEN ROAD TESTED.

Provides various drive modes on the web interface.

## Buses

| Bus | Rate | Use |
|---|---|---|
| CAN1 | 500 k | Prius `0x03B` / `0x3C9` / `0x3CB` / `0x3CD` / `0x4D1` |
| CAN2 | 500 k | BMW CSC poll `0x080–0x085`, replies addrs 2/3/4 |
| GP_analog1 (PC2) | — | Prius Hall after 1k/1k on V1.3 |
| USART3 | 115200 | OpenInverter terminal + ESP web |


## Drive modes (ESP page, category Drive Mode)

| `mode` | What the HV ECU is told |
|---|---|
| 0 Hold | Default. Real 25–85 % mapped to a Prius 50–70 % report |
| 1 CD | Report `cdspoof` (~74 %) until `socreal` ≤ `cdfloor`, then Hold |
| 2 EV | Report `evspoof` (~60 %) and keep CCL ≥ 60 so the EV button is allowed |
| 3 Range | CDL = 0 — force the engine |


## Build

```bash
sudo apt install gcc-arm-none-eabi
git clone --recurse-submodules https://github.com/damienmaguire/stm32_betty.git
cd stm32_betty
make
```

`make` runs `get-deps`: initialises the libopencm3 / libopeninv submodules (or clones them if you grabbed a zip) and builds `libopencm3` for STM32F1.

Flash like any Zombie. Linker origin is **0x08001000** (OpenInverter bootloader):

```bash
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
  -c "program stm32_betty.bin 0x08001000 verify reset exit"
```

Or the ESP web updater / CubeProgrammer. Open `stm32_betty.cbp` in Code::Blocks (ARM GCC). Target STM32F107 just runs `make`. Needs `-DSTM32F1` or CB will not see F1 headers.

## Wiring (V1.3)

- Prius hybrid CAN → CAN1
- CSC loom (Yel/Brn + Yel/Red, 5 V + GND) → CAN2
- Hall yellow after 1k/1k → GP analog 1. Rest `ibpin` ≈ 1.21–1.25 V
- Board 12 V from IGCT (or parked 12 V). Sleep-override jumper fitted
- `CANEN` / `CANSBY` driven in firmware

## Web / console

Stock ESP firmware. `save` after param changes.

Spots: `udc` `idc` `soc` `socreal` `sococv` `socah` `umin` `umax` `tmpmin` `tmpmax` `mods` `fault` `cells` `ibadc` `ibpin`

USART3 115200: `get udc`, `set packvhold 181`, `save`, `json`, `list`.

## Status (2026-09-11)

Firmware v4. Pack in, IGCT Ready, no triangle.

GPL-3.0, same as OpenInverter / Zombie.
