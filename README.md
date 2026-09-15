# stm32_betty

Gen 2 Prius (NHW20) Battery ECU on a **ZombieVerter VCU V1.3**.

This software enables the use of 3 × BMW Gen 1 PHEV 16s modules (48s, 26 Ah) as the hybrid battery in a Gen 2 Toyota Prius car.

PHEV function via a Prius Plug-In OBC G9090-47040 is on branch **PHEV_Testing**.
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
| 4 Charge | Engine-on Park charge via SOC lie |

## PHEV_Testing — G9090-47040 (v12)

`vehmode` = Hybrid (default, charger pins idle) or PHEV (charger code runs).
`chg` must also be On, CPLT present, pack healthy, `udc` < `Voltspnt`, `umax` < 4.00 V.
CHST 78 % duty forces PWM off.

| OBC | Wire | Zombie V1.3 | MCU |
|---|---|---|---|
| CHRQ | Green | PWM1 | PA6 TIM3_CH1 10 Hz, 0 or 100 % |
| CHPW | White | PWM2 | PA7 TIM3_CH2 10 Hz, duty = `chpwdty` |
| CHST | Orange | brake in | PA15, 1 ms sample → `chstdty` / `chst` |
| VCHG | Grey | Throttle 1 | PC0 after 1k/1k. `vchgpin` is the STM32 pin |
| ICHG | Yellow | Throttle 2 | PC1 after 1k/1k. `ichgpin` is the STM32 pin |
| CGND | Brown | GND | — |
| PIMR | Red | IGCT 12 V | S20-1 |
| CHEN | Green | 12 V permit | S20-3, GPIO later |
| CHG | Black | hold at GND | S20-5 |
| CPLT | — | start in | PD7 after external pilot circuit, polarity `cpltpol` |

Bench zeros (no HV, no AC): VCHG 2.32 V / ICHG 2.30 V at the OBC. Params `vchgzero` / `ichgzero`. Pull the 1 µF across the brake 1k5 before relying on `chstdty`.

Do not drive SMRs. Fuse DCHB (DC+).

## Build

```bash
sudo apt install gcc-arm-none-eabi
git clone --recurse-submodules -b PHEV_Testing https://github.com/damienmaguire/stm32_betty.git
cd stm32_betty
make
```

Flash like any Zombie. Linker origin is **0x08001000**.

## Wiring (V1.3)

- Prius hybrid CAN → CAN1
- CSC loom → CAN2
- Hall yellow after 1k/1k → GP analog 1
- Board 12 V from IGCT. Sleep-override jumper fitted

GPL-3.0, same as OpenInverter / Zombie.
