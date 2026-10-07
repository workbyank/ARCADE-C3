#pragma once
// Road Racer - 4 lanes. Stick up/down = change lane, A = nitro, B = brake. Grab coins to refill nitro.
#define RC_ID 3
#define RC_N  7
struct RcV { float x, v; uint8_t lane, w, t; bool on; };   // t: 0 car, 1 truck, 2 coin
static RcV rV[RC_N];
static float rY, rSpd, rDist, rNitro, rOff;
static uint8_t rLane, rLevel; static uint16_t rScore, rHi, rCoins; static int16_t rCd; static bool rDead;
static inline int rLaneTop(uint8_t l) { return 8 + l * 14; }

static void rcInit() {
  rLane = 2; rY = rLaneTop(2) + 3; rSpd = 2.0f; rDist = 0; rNitro = 60; rOff = 0; rLevel = 1;
  rScore = 0; rCoins = 0; rCd = 40; rDead = false; rHi = hiGet(RC_ID);
  for (uint8_t i = 0; i < RC_N; i++) rV[i].on = false;
}
static void rcSpawn() {
  uint8_t lane = random(4); int busy = 0; bool laneBusy = false;
  for (uint8_t i = 0; i < RC_N; i++) if (rV[i].on && rV[i].x > 84) { busy++; if (rV[i].lane == lane) laneBusy = true; }
  bool coin = random(100) < 14;
  if (laneBusy || (!coin && busy >= 3)) return;
  for (uint8_t i = 0; i < RC_N; i++) if (!rV[i].on) {
    RcV &c = rV[i]; c.on = true; c.x = 128; c.lane = lane;
    if (coin) { c.t = 2; c.w = 6; c.v = 0; }
    else if (rLevel >= 2 && random(100) < 22) { c.t = 1; c.w = 26; c.v = 0.6f; }
    else { c.t = 0; c.w = 14; c.v = 0.8f + random(0, 8) / 10.0f; }
    return;
  }
}
static void rcUpdate() {
  if (rDead) { if (overKeys()) rcInit(); return; }
  if (in.py < 0 && rLane > 0) { rLane--; sfx(SFX_MOVE); }
  if (in.py > 0 && rLane < 3) { rLane++; sfx(SFX_MOVE); }
  float ty = rLaneTop(rLane) + 3;
  if (rY < ty) { rY += 3; if (rY > ty) rY = ty; } else if (rY > ty) { rY -= 3; if (rY < ty) rY = ty; }
  float base = 2.0f + rLevel * 0.3f; if (base > 5.5f) base = 5.5f; float tgt = base;
  bool boost = in.a && rNitro > 1;
  if (boost) { tgt = base + 2.2f; rNitro -= 1.0f; if (rNitro < 0) rNitro = 0; ledFlash(C_PURPLE, 60); }
  else { if (in.b) tgt = base - 1.0f < 1.0f ? 1.0f : base - 1.0f; rNitro += 0.12f; if (rNitro > 100) rNitro = 100; }
  rSpd += (tgt - rSpd) * 0.2f;
  rDist += rSpd * 0.08f * (boost ? 1.5f : 1.0f);
  uint8_t nl = 1 + (uint16_t)(rDist / 120);
  if (nl > rLevel) { rLevel = nl; bannerLevel(nl); fx(SFX_LEVEL, C_CYAN, 200); }
  rScore = (uint16_t)rDist + rCoins * 10;
  rOff += rSpd; if (rOff >= 16) rOff -= 16;
  if (--rCd <= 0) { rcSpawn(); rCd = (int)(random(20, 46) * 3.0f / (rSpd + 1.0f)); if (rCd < 8) rCd = 8; }
  for (uint8_t i = 0; i < RC_N; i++) {
    RcV &c = rV[i]; if (!c.on) continue;
    float sp = (c.t == 2) ? rSpd : (rSpd - c.v < 0.5f ? 0.5f : rSpd - c.v);
    c.x -= sp; if (c.x + c.w < 0) { c.on = false; continue; }
    float cy = rLaneTop(c.lane) + 3;
    if (hitBox(15, rY + 1, 12, 6, c.x + 1, cy + 1, c.w - 2, 6)) {
      if (c.t == 2) { c.on = false; rCoins++; rNitro += 25; if (rNitro > 100) rNitro = 100; fx(SFX_COIN, C_GOLD, 100); }
      else { rDead = true; endRun(RC_ID, rScore, rHi); }
    }
  }
}
static void rcDraw() {
  u8g2.drawHLine(0, 8, 128); u8g2.drawHLine(0, 63, 128);
  for (int l = 1; l < 4; l++) for (int x = -(int)rOff; x < 128; x += 16) u8g2.drawHLine(x, 8 + l * 14, 8);
  for (uint8_t i = 0; i < RC_N; i++) {
    if (!rV[i].on) continue;
    int x = (int)rV[i].x, y = rLaneTop(rV[i].lane) + 3, w = rV[i].w;
    if (rV[i].t == 2) u8g2.drawDisc(x + 3, y + 4, 3);
    else {
      u8g2.drawFrame(x, y, w, 8);
      if (rV[i].t == 1) { u8g2.drawVLine(x + w - 8, y, 8); u8g2.drawBox(x + w - 6, y + 2, 4, 4); }
      else u8g2.drawBox(x + 3, y + 2, 4, 4);
    }
  }
  int py = (int)rY;
  u8g2.drawBox(14, py, 14, 8); u8g2.setDrawColor(0); u8g2.drawBox(21, py + 1, 3, 6); u8g2.drawVLine(17, py + 2, 4); u8g2.setDrawColor(1);
  if (in.a && rNitro > 1) { u8g2.drawBox(8, py + 2, 5, 1); u8g2.drawBox(5, py + 5, 8, 1); }
  char b[24]; u8g2.setFont(FONT_S);
  snprintf(b, sizeof b, "%05u LV%u", rScore, rLevel); u8g2.drawStr(1, 7, b);
  u8g2.drawStr(78, 7, "N"); u8g2.drawFrame(85, 1, 40, 6); u8g2.drawBox(86, 2, (int)(rNitro * 0.38f), 4);
  if (rDead) overBox(rScore, rHi);
}
static void rcIcon(int x, int y) {
  u8g2.drawHLine(x, y + 6, 32); u8g2.drawHLine(x, y + 26, 32);
  for (int i = 0; i < 32; i += 8) u8g2.drawHLine(x + i, y + 16, 4);
  u8g2.drawBox(x + 3, y + 18, 12, 6); u8g2.setDrawColor(0); u8g2.drawBox(x + 9, y + 19, 3, 4); u8g2.setDrawColor(1);
  u8g2.drawFrame(x + 18, y + 8, 12, 6);
}

