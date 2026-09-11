/*
 * stm32_betty — Gen 2 Prius Battery ECU on ZombieVerter VCU V1.3
 *
 * CAN1 500 k: Prius 0x03B / 0x3C9 / 0x3CB / 0x3CD / 0x4D1
 * CAN2 500 k: 3 x BMW PHEV 16s CSC (addrs 2/3/4)
 * Analog GP_analog1: Prius IB sensor after 1k/1k on V1.3
 *
 * ESP8266 web UI on the VCU shows PARAM / VALUE list over USART3.
 *
 * v3: boot grace + never publish partial/zero packV
 *     Drive Mode param (Hold / CD / EV / Range)
 */
#include "anain.h"
#include "bmw_crc.h"
#include "canhardware.h"
#include "digio.h"
#include "hwdefs.h"
#include "hwinit.h"
#include "param_save.h"
#include "params.h"
#include "stm32_can.h"
#include "stm32scheduler.h"
#include "terminal.h"
#include "terminalcommands.h"
#include <libopencm3/stm32/usart.h>
#include <math.h>
#include <string.h>
#include <libopencm3/stm32/can.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/iwdg.h>
#include <libopencm3/stm32/gpio.h>
#include <math.h>
#include <string.h>
#include "canmap.h"

extern const TERM_CMD TermCmds[];

#define CELLS_PER_MOD 16
#define NMOD 3
#define ADDR0 2
#define CELLS_PACK (CELLS_PER_MOD * NMOD)
#define PACKV_HOLD_FALLBACK 181.0f

enum DriveMode {
  MODE_HOLD = 0,
  MODE_CD = 1,
  MODE_EV = 2,
  MODE_RANGE = 3
};

static Stm32Scheduler *scheduler;
static CanHardware *priusCan;
static CanHardware *cscCan;

struct CellMod {
  bool exists;
  float cell[CELLS_PER_MOD];
  float temp[4];
  uint32_t lastMs;
};
static CellMod mods[NMOD];

static float packV = PACKV_HOLD_FALLBACK, packA = 0, minCell = 0, maxCell = 0;
static float tMin = 25, tMax = 25;
static uint8_t modulesSeen = 0;
static uint8_t cellsSeen = 0;
static uint16_t faultWord = 0;
static float socAh = 15.6f, socReal = 60, socOcv = 60;
static bool socSeeded = false;
static uint32_t tRest = 0, tBorn = 0, bootMs = 0;
static uint8_t nextmes = 0, mescycle = 0;
static uint32_t uptimeSec = 0;

static volatile uint32_t millisCnt;
static uint32_t nowMs() { return millisCnt; }

static bool inBootGrace() {
  uint32_t g = (uint32_t)Param::GetInt(Param::bootgrace);
  return (nowMs() - bootMs) < g;
}

static uint8_t priusChecksum(uint16_t id, const uint8_t *data, uint8_t dlc) {
  uint16_t s = (id >> 8) + (id & 0xFF) + dlc;
  for (uint8_t i = 0; i < dlc - 1; i++)
    s += data[i];
  return (uint8_t)(s & 0xFF);
}

static void sendStd(CanHardware *bus, uint16_t id, const uint8_t *d, uint8_t len) {
  uint8_t buf[8] = {0};
  memcpy(buf, d, len);
  bus->Send(id, buf, len);
}

static int slotFromAddr(int addr) {
  if (addr < ADDR0 || addr > ADDR0 + NMOD - 1)
    return -1;
  return addr - ADDR0;
}

static float ocvSocFromCell(float v) {
  static const float vv[] = {3.20f, 3.40f, 3.50f, 3.55f, 3.62f, 3.70f,
                             3.78f, 3.85f, 3.92f, 4.00f, 4.10f};
  static const float ss[] = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
  const int n = 11;
  if (v <= vv[0])
    return ss[0];
  if (v >= vv[n - 1])
    return ss[n - 1];
  for (int i = 1; i < n; i++) {
    if (v <= vv[i]) {
      float f = (v - vv[i - 1]) / (vv[i] - vv[i - 1]);
      return ss[i - 1] + f * (ss[i] - ss[i - 1]);
    }
  }
  return 50;
}

static void applyOcv(float pct) {
  if (pct < 1)
    pct = 1;
  if (pct > 99)
    pct = 99;
  socOcv = pct;
  socReal = pct;
  socAh = Param::GetFloat(Param::packah) * (pct / 100.0f);
  socSeeded = true;
}

static float readCurrentA() {
  uint16_t raw = AnaIn::GP_analog1.Get();
  float vpin = raw * (3.3f / 4095.0f);
  Param::SetInt(Param::ibadc, raw);
  Param::SetFloat(Param::ibpin, vpin);
  float zero = Param::GetFloat(Param::ibzero);
  float div = Param::GetFloat(Param::ibdiv);
  float gain = Param::GetFloat(Param::ibgain);
  return ((vpin - zero) / div) * gain;
}

static void updatePackFromCsc() {
  float sumV = 0, cmin = 5, cmax = 0, tmn = 200, tmx = -100;
  uint8_t seen = 0;
  uint8_t cells = 0;
  uint32_t now = nowMs();
  for (int m = 0; m < NMOD; m++) {
    if (!mods[m].exists)
      continue;
    if (now - mods[m].lastMs > 2000) {
      mods[m].exists = false;
      continue;
    }
    seen++;
    for (int c = 0; c < CELLS_PER_MOD; c++) {
      float v = mods[m].cell[c];
      if (v < 0.5f || v > 5.0f)
        continue;
      cells++;
      sumV += v;
      if (v < cmin)
        cmin = v;
      if (v > cmax)
        cmax = v;
    }
    for (int t = 0; t < 4; t++) {
      float tp = mods[m].temp[t];
      if (tp <= -39)
        continue;
      if (tp < tmn)
        tmn = tp;
      if (tp > tmx)
        tmx = tp;
    }
  }
  modulesSeen = seen;
  cellsSeen = cells;
  minCell = (cmin < 5) ? cmin : 0;
  maxCell = cmax;
  if (cells >= CELLS_PACK && sumV > 20.0f)
    packV = sumV;
  else
    packV = Param::GetFloat(Param::packvhold);
  if (tmn < 200)
    tMin = tmn;
  if (tmx > -100)
    tMax = tmx;
}

static void updateSoc(float dt) {
  if (dt <= 0 || dt > 1.0f)
    dt = 0.01f;
  socAh -= packA * dt / 3600.0f;
  float cap = Param::GetFloat(Param::packah);
  if (socAh < 0.1f)
    socAh = 0.1f;
  if (socAh > cap)
    socAh = cap;
  socReal = 100.0f * socAh / cap;
  if (minCell > 0.5f)
    socOcv = ocvSocFromCell(minCell);

  uint32_t now = nowMs();
  bool resting = (packA > -1.5f && packA < 1.5f && minCell > 0.5f);
  if (!resting)
    tRest = now;
  if (!socSeeded && minCell > 0.5f) {
    if ((resting && (now - tRest) >= 2000) || (now - tBorn) >= 5000)
      applyOcv(socOcv);
  } else if (socSeeded && resting && (now - tRest) >= 8000) {
    applyOcv(socOcv);
    tRest = now;
  }
}

static uint8_t holdSpoof() {
  float rmin = Param::GetFloat(Param::socrealmin);
  float rmax = Param::GetFloat(Param::socrealmax);
  float smin = Param::GetFloat(Param::spoofmin);
  float smax = Param::GetFloat(Param::spoofmax);
  float real = socReal;
  if (real < rmin)
    real = rmin;
  if (real > rmax)
    real = rmax;
  float spoof = smin + ((real - rmin) / (rmax - rmin)) * (smax - smin);
  if (spoof < smin)
    spoof = smin;
  if (spoof > smax)
    spoof = smax;
  return (uint8_t)(spoof + 0.5f);
}

static uint8_t socToReport() {
  int mode = Param::GetInt(Param::mode);
  if (mode == MODE_CD) {
    if (socReal > Param::GetFloat(Param::cdfloor))
      return (uint8_t)(Param::GetFloat(Param::cdspoof) + 0.5f);
    return holdSpoof();
  }
  if (mode == MODE_EV)
    return (uint8_t)(Param::GetFloat(Param::evspoof) + 0.5f);
  /* HOLD and RANGE use the 50–70 map */
  return holdSpoof();
}

static void limitsFromHealth(uint8_t &cdl, uint8_t &ccl) {
  cdl = (uint8_t)Param::GetInt(Param::cdl);
  ccl = (uint8_t)Param::GetInt(Param::ccl);
  faultWord = 0;

  bool grace = inBootGrace();
  if (modulesSeen < NMOD && !grace) {
    cdl = 20;
    ccl = 0;
    faultWord = 0x3030;
  }
  if (minCell > 0.5f && minCell < 3.30f)
    cdl = 20;
  if (maxCell > 4.00f)
    ccl = 10;
  if (minCell > 0.5f && minCell < 3.20f) {
    cdl = 0;
    faultWord = 0x0A7F;
  }
  if (maxCell > 4.05f) {
    ccl = 0;
    faultWord = 0x0A7F;
  }

  /* Modes only apply while the pack is healthy. */
  if (faultWord == 0) {
    int mode = Param::GetInt(Param::mode);
    if (mode == MODE_EV && ccl < 60)
      ccl = 60;
    if (mode == MODE_RANGE)
      cdl = 0;
  }
}

static void tx03B() {
  int32_t raw = (int32_t)(packA * 10.0f);
  if (raw > 2047)
    raw = 2047;
  if (raw < -2048)
    raw = -2048;
  uint16_t u = (uint16_t)(raw & 0x0FFF);
  uint16_t v = (uint16_t)packV;
  if (v > 510)
    v = 510;
  uint8_t d[5];
  d[0] = (u >> 8) & 0x0F;
  d[1] = u & 0xFF;
  d[2] = v >> 8;
  d[3] = v & 0xFF;
  d[4] = priusChecksum(0x03B, d, 5);
  sendStd(priusCan, 0x03B, d, 5);
}

static void tx3CB() {
  uint8_t cdl, ccl;
  limitsFromHealth(cdl, ccl);
  uint8_t soc = socToReport();
  uint8_t d[7];
  d[0] = cdl;
  d[1] = ccl;
  d[2] = 0;
  d[3] = (uint8_t)(soc * 2);
  d[4] = (uint8_t)((int)tMin & 0xFF);
  d[5] = (uint8_t)((int)tMax & 0xFF);
  d[6] = priusChecksum(0x3CB, d, 7);
  sendStd(priusCan, 0x3CB, d, 7);
}

static void tx3CD() {
  uint16_t v = (uint16_t)packV;
  if (v > 510)
    v = 510;
  uint8_t d[5];
  d[0] = faultWord >> 8;
  d[1] = faultWord & 0xFF;
  d[2] = v >> 8;
  d[3] = v & 0xFF;
  d[4] = priusChecksum(0x3CD, d, 5);
  sendStd(priusCan, 0x3CD, d, 5);
}

static void tx3C9() {
  uint16_t blockCv = (uint16_t)(packV / 14.0f * 100.0f + 0.5f);
  if (blockCv < 800)
    blockCv = 800;
  if (blockCv > 1800)
    blockCv = 1800;
  uint8_t d[8];
  d[0] = 0x03;
  d[1] = 0xFF;
  d[2] = 0x21;
  d[3] = (uint8_t)(blockCv >> 8);
  d[4] = (uint8_t)(blockCv & 0xFF);
  d[5] = d[3];
  d[6] = d[4];
  d[7] = priusChecksum(0x3C9, d, 8);
  sendStd(priusCan, 0x3C9, d, 8);
}

static void tx4D1() {
  uint8_t d[8] = {0x11, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00};
  sendStd(priusCan, 0x4D1, d, 8);
}

static void pollBmw() {
  if (!cscCan)
    return;
  if (nextmes >= 6) {
    nextmes = 0;
    mescycle++;
    if (mescycle > 0x0F)
      mescycle = 0;
  }
  uint16_t id = 0x080 + nextmes;
  uint8_t d[8] = {0xC7, 0x10, 0x00, 0x00, 0x40, 0x01, 0x00, 0x00};
  d[6] = (uint8_t)(mescycle << 4);
  d[7] = BmwCrc::calc(id, d, 8, nextmes);
  sendStd(cscCan, id, d, 8);
  nextmes++;
}

static void handleBmw(uint32_t canId, uint32_t data[2], uint8_t /*dlc*/) {
  uint8_t b[8];
  memcpy(b, data, 8);
  int addr = (int)(canId & 0xF);
  int slot = slotFromAddr(addr);
  if (slot < 0)
    return;
  mods[slot].exists = true;
  mods[slot].lastMs = nowMs();
  int mid = (int)((canId >> 4) & 0xF);
  if (mid >= 2 && mid <= 7) {
    int base = (mid - 2) * 3;
    for (int i = 0; i < 3; i++) {
      int idx = base + i;
      if (idx >= CELLS_PER_MOD)
        break;
      uint16_t mv = (uint16_t)b[i * 2] | ((uint16_t)b[i * 2 + 1] << 8);
      if (mv > 500 && mv < 5000)
        mods[slot].cell[idx] = mv * 0.001f;
    }
  }
  if (mid == 8 || mid == 0) {
    for (int i = 0; i < 4; i++)
      mods[slot].temp[i] = (float)b[i] - 40.0f;
  }
}

static bool CscRx(uint32_t canId, uint32_t *data, uint8_t dlc) {
  handleBmw(canId, data, dlc);
  return false;
}
static void CscClear() {}

static void publish() {
  Param::SetFloat(Param::udc, packV);
  Param::SetFloat(Param::idc, packA);
  Param::SetInt(Param::soc, socToReport());
  Param::SetFloat(Param::socreal, socReal);
  Param::SetFloat(Param::sococv, socOcv);
  Param::SetFloat(Param::socah, socAh);
  Param::SetFloat(Param::umin, minCell);
  Param::SetFloat(Param::umax, maxCell);
  Param::SetFloat(Param::tmpmin, tMin);
  Param::SetFloat(Param::tmpmax, tMax);
  Param::SetInt(Param::mods, modulesSeen);
  Param::SetInt(Param::fault, faultWord);
  Param::SetInt(Param::cells, cellsSeen);
  Param::SetInt(Param::version, 4);
  Param::SetInt(Param::uptime, (int)uptimeSec);
  float uaux = AnaIn::uaux.Get() * (3.3f / 4095.0f) * 9.2f; // typical zombie scale-ish
  Param::SetFloat(Param::uaux, uaux);
}

static void Ms1Task() {
  millisCnt++;
  static uint8_t div = 0;
  if (++div < 8)
    return;
  div = 0;
  tx03B();
}

static void Ms10Task() {
  packA = readCurrentA();
  updatePackFromCsc();
  updateSoc(0.010f);
  pollBmw();
}

static void Ms100Task() {
  iwdg_reset();
  tx3CB();
  tx3CD();
  tx3C9();
  publish();
  if (scheduler)
    Param::SetFloat(Param::cpuload, scheduler->GetCpuLoad() / 10.0f);
  DigIo::led_out.Toggle();
}

static void Ms1000Task() {
  tx4D1();
  uptimeSec++;
}

extern "C" void tim4_isr(void) { scheduler->Run(); }

void Param::Change(Param::PARAM_NUM /*paramNum*/) {}

static void SetCanFilters() {
  cscCan->RegisterUserMessage(0x122);
  cscCan->RegisterUserMessage(0x132);
  cscCan->RegisterUserMessage(0x142);
  cscCan->RegisterUserMessage(0x152);
  cscCan->RegisterUserMessage(0x162);
  cscCan->RegisterUserMessage(0x172);
  cscCan->RegisterUserMessage(0x123);
  cscCan->RegisterUserMessage(0x133);
  cscCan->RegisterUserMessage(0x143);
  cscCan->RegisterUserMessage(0x153);
  cscCan->RegisterUserMessage(0x163);
  cscCan->RegisterUserMessage(0x173);
  cscCan->RegisterUserMessage(0x124);
  cscCan->RegisterUserMessage(0x134);
  cscCan->RegisterUserMessage(0x144);
  cscCan->RegisterUserMessage(0x154);
  cscCan->RegisterUserMessage(0x164);
  cscCan->RegisterUserMessage(0x174);
  cscCan->RegisterUserMessage(0x182);
  cscCan->RegisterUserMessage(0x183);
  cscCan->RegisterUserMessage(0x184);
  cscCan->RegisterUserMessage(0x202);
  cscCan->RegisterUserMessage(0x203);
  cscCan->RegisterUserMessage(0x204);
}

int main(void) {
  iwdg_reset();
  clock_setup();
  gpio_primary_remap(AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON,
                   AFIO_MAPR_CAN2_REMAP);
  rtc_setup();
  nvic_setup();
  DIG_IO_CONFIGURE(DIG_IO_LIST);
  ANA_IN_CONFIGURE(ANA_IN_LIST);

  for (int i = 0; i < 16; i++) {
    DigIo::led_out.Toggle();
    for (volatile int d = 0; d < 300000; d++)
      ;
    iwdg_reset();
  }

  Stm32Scheduler s(TIM4);
  scheduler = &s;

  parm_load();
  iwdg_reset();
  AnaIn::Start();

  DigIo::CANEN.Set();
  DigIo::CANSBY.Set();
  DigIo::inv_out.Clear();

  Terminal t(USART3, TermCmds, false, true, !Param::GetBool(Param::UseRS232));

  Stm32Can can0(CAN1, CanHardware::Baud500);
  Stm32Can can1(CAN2, CanHardware::Baud500, true);
  priusCan = &can0;
  cscCan = &can1;

  CanMap cm(&can0);
  TerminalCommands::SetCanMap(&cm);

  FunctionPointerCallback cb(CscRx, CscClear);
  can1.AddCallback(&cb);
  SetCanFilters();

  packV = Param::GetFloat(Param::packvhold);
  packA = 0;
  faultWord = 0;
  bootMs = nowMs();
  tBorn = nowMs();

  s.AddTask(Ms1Task, 1);
  s.AddTask(Ms10Task, 10);
  s.AddTask(Ms100Task, 100);
  s.AddTask(Ms1000Task, 1000);

  while (1) {
    //iwdg_reset();
    t.Run();
  }
}
