/*
 * stm32_betty - Gen 2 Prius Battery ECU on ZombieVerter VCU V1.3
 */
#ifndef PARAM_PRJ_H_INCLUDED
#define PARAM_PRJ_H_INCLUDED

#define VER 11

#define CAT_SETUP "General Setup"
#define CAT_SENS  "Current Sensor"
#define CAT_BMS   "Battery"
#define CAT_MODE  "Drive Mode"
#define CAT_CHG   "PHEV Charger"

#define DRVMODES  "0=Hold, 1=CD, 2=EV, 3=Range, 4=Charge"
#define OPMODES   "0=Off, 1=Hold, 2=CD, 3=EV, 4=Range, 5=Charge"
#define VEHMODES  "0=Hybrid, 1=PHEV"
#define OBCSTAT   "0=Idle, 1=WaitPilot, 2=Run, 3=Stop, 4=Fault"
#define ONOFF     "0=Off, 1=On, 2=na"
#define POLARITY  "0=ActiveLow, 1=ActiveHigh"

#define PARAM_LIST                                                             \
  PARAM_ENTRY(CAT_SENS, ibzero, "V", 1.20, 2.00, 1.586, 1)                     \
  PARAM_ENTRY(CAT_SENS, ibgain, "A/V", 50, 200, 100, 2)                        \
  PARAM_ENTRY(CAT_SENS, ibdiv, "", 0.40, 0.80, 0.6357, 3)                      \
  PARAM_ENTRY(CAT_BMS, packah, "Ah", 5, 80, 26, 4)                             \
  PARAM_ENTRY(CAT_BMS, socrealmin, "%", 0, 50, 25, 5)                          \
  PARAM_ENTRY(CAT_BMS, socrealmax, "%", 50, 100, 85, 6)                        \
  PARAM_ENTRY(CAT_BMS, spoofmin, "%", 30, 70, 50, 7)                           \
  PARAM_ENTRY(CAT_BMS, spoofmax, "%", 50, 90, 70, 8)                           \
  PARAM_ENTRY(CAT_BMS, cdl, "A", 0, 200, 80, 9)                                \
  PARAM_ENTRY(CAT_BMS, ccl, "A", 0, 200, 60, 10)                               \
  PARAM_ENTRY(CAT_BMS, packvhold, "V", 160, 210, 181, 17)                      \
  PARAM_ENTRY(CAT_SETUP, UseRS232, ONOFF, 0, 1, 0, 11)                         \
  PARAM_ENTRY(CAT_SETUP, vehmode, VEHMODES, 0, 1, 0, 20)                       \
  PARAM_ENTRY(CAT_MODE, mode, DRVMODES, 0, 4, 0, 12)                           \
  PARAM_ENTRY(CAT_MODE, cdfloor, "%", 10, 50, 25, 13)                          \
  PARAM_ENTRY(CAT_MODE, cdspoof, "%", 60, 80, 74, 14)                          \
  PARAM_ENTRY(CAT_MODE, evspoof, "%", 50, 70, 60, 15)                          \
  PARAM_ENTRY(CAT_MODE, bootgrace, "ms", 0, 10000, 4000, 16)                   \
  PARAM_ENTRY(CAT_MODE, chargespoof, "%", 30, 50, 40, 18)                      \
  PARAM_ENTRY(CAT_MODE, chargeceil, "%", 50, 95, 80, 19)                       \
  PARAM_ENTRY(CAT_CHG, chg, ONOFF, 0, 1, 0, 21)                                \
  PARAM_ENTRY(CAT_CHG, Voltspnt, "V", 170, 210, 197, 22)                       \
  PARAM_ENTRY(CAT_CHG, chglim, "A", 0, 16, 8, 23)                              \
  PARAM_ENTRY(CAT_CHG, chpwdty, "%", 0, 100, 40, 24)                           \
  PARAM_ENTRY(CAT_CHG, vchgscale, "V/V", 0, 100, 40, 25)                       \
  PARAM_ENTRY(CAT_CHG, cpltpol, POLARITY, 0, 1, 1, 26)                         \
  VALUE_ENTRY(opmode, OPMODES, 2000)                                           \
  VALUE_ENTRY(udc, "V", 2001)                                                  \
  VALUE_ENTRY(idc, "A", 2002)                                                  \
  VALUE_ENTRY(soc, "%", 2003)                                                  \
  VALUE_ENTRY(socreal, "%", 2004)                                              \
  VALUE_ENTRY(sococv, "%", 2005)                                               \
  VALUE_ENTRY(socah, "Ah", 2006)                                               \
  VALUE_ENTRY(umin, "V", 2007)                                                 \
  VALUE_ENTRY(umax, "V", 2008)                                                 \
  VALUE_ENTRY(tmpmin, "°C", 2009)                                              \
  VALUE_ENTRY(tmpmax, "°C", 2010)                                              \
  VALUE_ENTRY(mods, "", 2011)                                                  \
  VALUE_ENTRY(fault, "hex", 2012)                                              \
  VALUE_ENTRY(uaux, "V", 2013)                                                 \
  VALUE_ENTRY(ibadc, "dig", 2014)                                              \
  VALUE_ENTRY(ibpin, "V", 2015)                                                 \
  VALUE_ENTRY(cpuload, "%", 2016)                                              \
  VALUE_ENTRY(version, "", 2017)                                                \
  VALUE_ENTRY(uptime, "sec", 2018)                                              \
  VALUE_ENTRY(cells, "", 2019)                                                 \
  VALUE_ENTRY(deltav, "V", 2020)                                               \
  VALUE_ENTRY(power, "kW", 2021)                                               \
  VALUE_ENTRY(obcstat, OBCSTAT, 2022)                                          \
  VALUE_ENTRY(obc_udc, "V", 2023)                                              \
  VALUE_ENTRY(chst, "", 2024)                                                  \
  VALUE_ENTRY(cplt, "", 2025)                                                  \
  VALUE_ENTRY(chrq, "", 2026)                                                  \
  VALUE_ENTRY(chpw, "%", 2027)

#endif
