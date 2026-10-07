#pragma once
// Dino Run - Chrome style: A/Up = jump, B/Down = duck (fast-fall in the air). Birds, clusters, day/night.
#define DN_ID  0
#define DN_GND 56

struct DnO { float x; uint8_t w, h, y, t; bool on; };   // t: 0 cactus, 1 bird. y = height above ground
static DnO dO[4];
static float dY, dVy, dSp, dDist, dNext, dCx[3];
static bool dDead, dDuck, dNight;
static uint16_t dScore, dHi;
static uint8_t dFr, dCy[3];
static uint32_t dBlink;

static void dnInit() {
  dY = DN_GND - 14; dVy = 0; dSp = 3.0f; dDist = 0; dNext = 60; dScore = 0;
  dDead = false; dDuck = false; dNight = false; dFr = 0; dBlink = 0; dHi = hiGet(DN_ID);
  for (uint8_t i = 0; i < 4; i++) dO[i].on = false;
  for (uint8_t i = 0; i < 3; i++) { dCx[i] = 20 + i * 45; dCy[i] = 14 + (i * 7) % 18; }
}

static void dnSpawn() {
  for (uint8_t i = 0; i < 4; i++) {
    if (dO[i].on) continue;
    DnO &o = dO[i]; o.on = true; o.x = 128;
    if (dScore >= 120 && random(100) < 30) {
      static const uint8_t by[3] = {2, 10, 22};          // low = jump, mid = duck/jump, high = stay low
      o.t = 1; o.w = 12; o.h = 8; o.y = by[random(3)];
      dNext = dSp * 16 + random(0, (long)(dSp * 12)) + 20;
    } else {
      static const uint8_t cw[4] = {5, 7, 13, 19}, ch[4] = {10, 15, 10, 10};
      uint8_t v = random(4); o.t = 0; o.w = cw[v]; o.h = ch[v]; o.y = 0;
      dNext = dSp * 14 + random(0, (long)(dSp * 14));
    }
    return;
  }
}

static void dnUpdate() {
  if (dDead) { if (overKeys()) dnInit(); return; }
  bool onG = dY >= DN_GND - 14;
  bool jump = in.a || in.dy < 0, down = in.b || in.dy > 0;
  if (onG && jump) { dVy = -5.8f; sfx(SFX_JUMP); }
  bool wasDuck = dDuck;
  dDuck = onG && down && !jump;
  if (dDuck && !wasDuck) sfx(SFX_DUCK);
  dVy += (!onG && down) ? 1.5f : 0.6f;
  dY += dVy;
  if (dY >= DN_GND - 14) { dY = DN_GND - 14; dVy = 0; }

  dSp = 3.0f + dScore * 0.0045f; if (dSp > 7.5f) dSp = 7.5f;
  dDist += dSp;
  uint16_t ns = (uint16_t)(dDist / 8);
  if (ns / 100 > dScore / 100) { fx(SFX_COIN, C_CYAN, 150); dBlink = millis() + 700; }
  dScore = ns; dNight = (dScore / 400) & 1; dFr++;
  for (uint8_t i = 0; i < 3; i++) { dCx[i] -= dSp * 0.25f; if (dCx[i] < -14) { dCx[i] = 128 + random(30); dCy[i] = 12 + random(20); } }

  float right = -1000;
  for (uint8_t i = 0; i < 4; i++) {
    if (!dO[i].on) continue;
    dO[i].x -= dSp;
    if (dO[i].x + dO[i].w < 0) dO[i].on = false;
    else if (dO[i].x + dO[i].w > right) right = dO[i].x + dO[i].w;
  }
  if (right < 128 - dNext) dnSpawn();

  float hx = 13, hw = 8, hy = dY + 2, hh = 12;
  if (dDuck) { hx = 11; hw = 14; hy = DN_GND - 7; hh = 7; }
  for (uint8_t i = 0; i < 4; i++) {
    if (!dO[i].on) continue;
    float oy = DN_GND - dO[i].y - dO[i].h;
    if (hitBox(hx, hy, hw, hh, dO[i].x + 1, oy + 1, dO[i].w - 2, dO[i].h - 1)) { dDead = true; endRun(DN_ID, dScore, dHi); break; }
  }
}

static void dnDino(int x, int y, bool duck, bool dead, bool legs) {
  if (!duck) {
    u8g2.drawBox(x + 7, y, 7, 6); u8g2.drawBox(x + 3, y + 5, 8, 6); u8g2.drawBox(x, y + 6, 4, 3); u8g2.drawBox(x + 11, y + 8, 2, 1);
    u8g2.setDrawColor(0); u8g2.drawPixel(x + 11, y + 1); if (dead) u8g2.drawPixel(x + 12, y + 2); u8g2.setDrawColor(1);
    u8g2.drawBox(x + 3, y + 11, 2, 3);
    if (legs) u8g2.drawBox(x + 7, y + 11, 2, 3); else u8g2.drawBox(x + 7, y + 10, 2, 2);
  } else {
    y = DN_GND - 8;
    u8g2.drawBox(x, y + 2, 12, 5); u8g2.drawBox(x + 10, y, 6, 5); u8g2.drawBox(x - 2, y + 3, 3, 2);
    u8g2.setDrawColor(0); u8g2.drawPixel(x + 13, y + 1); u8g2.setDrawColor(1);
    u8g2.drawBox(x + 2, y + 7, 2, 1); if (legs) u8g2.drawBox(x + 7, y + 7, 2, 1);
  }
}

static void dnDraw() {
  for (uint8_t i = 0; i < 3; i++) { int cx = (int)dCx[i], cy = dCy[i]; u8g2.drawBox(cx, cy, 10, 2); u8g2.drawBox(cx + 2, cy - 1, 6, 1); }
  if (dNight) { u8g2.drawDisc(100, 18, 4); u8g2.setDrawColor(0); u8g2.drawDisc(102, 17, 3); u8g2.setDrawColor(1); }
  u8g2.drawHLine(0, DN_GND, 128);
  for (int i = 0; i < 6; i++) { int x = ((i * 29) - (int)dDist) % 128; if (x < 0) x += 128; u8g2.drawPixel(x, DN_GND + 3 + (i & 1) * 3); }
  bool air = dY < DN_GND - 14;
  dnDino(12, (int)dY, dDuck, dDead, air || ((dFr / 4) & 1));
  for (uint8_t i = 0; i < 4; i++) {
    if (!dO[i].on) continue;
    int ox = (int)dO[i].x, w = dO[i].w, h = dO[i].h;
    if (dO[i].t == 0) {
      for (int k = 0; k + 3 <= w; k += 6) {
        int tx = ox + k + 1, th = ((k / 6) & 1) ? h - 3 : h;
        u8g2.drawBox(tx, DN_GND - th, 3, th); u8g2.drawBox(tx - 2, DN_GND - th + 3, 2, 3); u8g2.drawBox(tx + 3, DN_GND - th + 4, 2, 3);
      }
    } else {
      int by = DN_GND - dO[i].y - h;
      u8g2.drawBox(ox + 2, by + 3, 8, 3); u8g2.drawBox(ox, by + 3, 3, 2); u8g2.drawBox(ox + 10, by + 4, 2, 1);
      if ((dFr / 5) & 1) u8g2.drawBox(ox + 4, by, 4, 3); else u8g2.drawBox(ox + 4, by + 6, 4, 2);
    }
  }
  if (dNight) { u8g2.setDrawColor(2); u8g2.drawBox(0, 0, 128, 64); u8g2.setDrawColor(1); }
  char b[28]; u8g2.setFont(FONT_S);
  snprintf(b, sizeof b, "HI %05u", dHi); u8g2.drawStr(60, 7, b);
  if (!(dBlink && millis() < dBlink && ((millis() / 120) & 1))) { snprintf(b, sizeof b, "%05u", dScore); u8g2.drawStr(100, 7, b); }
  if (dDead) overBox(dScore, dHi);
}
static void dnIcon(int x, int y) {
  u8g2.drawHLine(x, y + 28, 32); dnDino(x + 3, y + 14, false, false, true);
  u8g2.drawBox(x + 23, y + 18, 3, 10); u8g2.drawBox(x + 21, y + 21, 2, 3); u8g2.drawBox(x + 26, y + 22, 2, 3);
  u8g2.drawBox(x + 14, y + 4, 8, 2); u8g2.drawBox(x + 4, y + 8, 6, 1);
}
