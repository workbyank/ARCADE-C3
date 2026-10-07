#pragma once
// Flappy - A or stick up = flap. Every 6 pipes: level up (faster, narrower). From level 3 the pipes float.
#define FB_ID 8
#define FB_PW 12
struct FbP { float x, gy, ph; bool passed; };
static FbP fbP[3]; static float fbY, fbVy; static uint16_t fbScore, fbHi; static uint8_t fbLvl, fbFr; static bool fbDead, fbStarted;
static int fbGap() { int g = 25 - fbLvl; return g < 16 ? 16 : g; }
static float fbSpd() { float s = 1.8f + 0.15f * fbLvl; return s > 3.2f ? 3.2f : s; }
static float fbGy(uint8_t i) {
  float g = fbP[i].gy; if (fbLvl >= 3) g += sinf(fbP[i].ph) * 6;
  float hi = 64 - 9 - fbGap(); if (g < 9) g = 9; if (g > hi) g = hi; return g;
}
static void fbNew(uint8_t i, float x) { fbP[i].x = x; fbP[i].gy = random(10, 64 - 9 - fbGap()); fbP[i].ph = random(628) / 100.0f; fbP[i].passed = false; }
static void fbInit() {
  fbY = 28; fbVy = 0; fbScore = 0; fbLvl = 1; fbFr = 0; fbDead = false; fbStarted = false; fbHi = hiGet(FB_ID);
  for (uint8_t i = 0; i < 3; i++) fbNew(i, 128 + i * 64);
}
static void fbUpdate() {
  if (fbDead) { if (overKeys()) fbInit(); return; }
  bool flap = in.aP || in.py < 0; fbFr++;
  if (!fbStarted) { if (flap) { fbStarted = true; fbVy = -3.0f; sfx(SFX_JUMP); } return; }
  if (flap) { fbVy = -3.0f; sfx(SFX_JUMP); }
  fbVy += 0.28f; if (fbVy > 4) fbVy = 4; fbY += fbVy;
  float sp = fbSpd();
  for (uint8_t i = 0; i < 3; i++) {
    fbP[i].x -= sp; fbP[i].ph += 0.06f;
    if (fbP[i].x < -FB_PW) fbNew(i, fbP[i].x + 192);
    if (!fbP[i].passed && fbP[i].x + FB_PW < 24) {
      fbP[i].passed = true; fbScore++;
      uint8_t nl = 1 + fbScore / 6;
      if (nl > fbLvl) { fbLvl = nl; bannerLevel(nl); fx(SFX_LEVEL, C_CYAN, 200); } else fx(SFX_COIN, C_GOLD, 80);
    }
    float gy = fbGy(i);
    if (24 + 8 > fbP[i].x && 24 < fbP[i].x + FB_PW && (fbY < gy || fbY + 6 > gy + fbGap())) fbDead = true;
  }
  if (fbY < 0 || fbY + 6 > 64) fbDead = true;
  if (fbDead) endRun(FB_ID, fbScore, fbHi);
}
static void fbDraw() {
  for (uint8_t i = 0; i < 3; i++) {
    int x = (int)fbP[i].x, gy = (int)fbGy(i), g = fbGap();
    u8g2.drawBox(x, 0, FB_PW, gy); u8g2.drawBox(x - 1, gy - 3, FB_PW + 2, 3);
    u8g2.drawBox(x, gy + g, FB_PW, 64 - gy - g); u8g2.drawBox(x - 1, gy + g, FB_PW + 2, 3);
  }
  int y = (int)fbY;
  u8g2.drawBox(24, y, 8, 6); u8g2.drawBox(32, y + 2, 2, 2);
  u8g2.setDrawColor(0); u8g2.drawPixel(29, y + 1); u8g2.drawBox(24, y + ((fbVy < 0) ? 1 : 3), 3, 1); u8g2.setDrawColor(1);
  char b[16]; snprintf(b, sizeof b, "%u", fbScore);
  u8g2.setDrawColor(0); u8g2.drawBox(52, 0, 24, 11); u8g2.setDrawColor(1); u8g2.setFont(FONT_M); textC(b, 9);
  if (!fbStarted) { u8g2.setFont(FONT_S); u8g2.setDrawColor(0); u8g2.drawBox(24, 53, 80, 10); u8g2.setDrawColor(1); textC("press A to flap", 61); }
  if (fbDead) overBox(fbScore, fbHi);
}
static void fbIcon(int x, int y) {
  u8g2.drawBox(x + 2, y + 12, 9, 7); u8g2.drawBox(x + 11, y + 14, 2, 2);
  u8g2.drawBox(x + 20, y, 8, 10); u8g2.drawBox(x + 19, y + 8, 10, 2); u8g2.drawBox(x + 20, y + 21, 8, 11); u8g2.drawBox(x + 19, y + 21, 10, 2);
}

