#pragma once
// Cave Copter - hold A (or stick up) to climb, release to sink. Caves narrow, pillars appear, speed grows.
#define CP_ID 4
static uint8_t cT[128], cB[128], cM[128], cH[128];
static float cY, cVy, cAcc, cDist, cTr[6];
static int cCen, cDir, cGap; static uint8_t cPil, cLevel, cFr; static uint16_t cScore, cHi; static bool cDead;

static void cpPush() {
  if (random(100) < 12) cDir = random(3) - 1;
  cCen += cDir * (1 + (random(3) == 0));
  int lo = 10 + cGap / 2, hi = 62 - cGap / 2;
  if (cCen < lo) { cCen = lo; cDir = 1; } if (cCen > hi) { cCen = hi; cDir = -1; }
  if (cPil == 0 && cLevel >= 2 && random(100) < 2) cPil = 7;
  memmove(cT, cT + 1, 127); memmove(cB, cB + 1, 127); memmove(cM, cM + 1, 127); memmove(cH, cH + 1, 127);
  cT[127] = cCen - cGap / 2; cB[127] = cCen + cGap / 2;
  if (cPil) { cM[127] = cCen - cGap / 8; cH[127] = cGap / 4; cPil--; } else cH[127] = 0;
}
static void cpInit() {
  cY = 32; cVy = 0; cAcc = 0; cDist = 0; cCen = 36; cDir = 0; cGap = 44; cPil = 0; cLevel = 1; cScore = 0; cDead = false; cFr = 0;
  cHi = hiGet(CP_ID); for (uint8_t i = 0; i < 6; i++) cTr[i] = 32;
  for (uint8_t i = 0; i < 128; i++) cpPush();
}
static void cpUpdate() {
  if (cDead) { if (overKeys()) cpInit(); return; }
  bool up = in.a || in.dy < 0;
  cVy += up ? -0.5f : 0.32f; if (cVy > 3) cVy = 3; if (cVy < -3) cVy = -3; cY += cVy;
  float spd = 1.0f + cLevel * 0.2f; if (spd > 3.0f) spd = 3.0f;
  cAcc += spd; cDist += spd;
  while (cAcc >= 1) { cAcc -= 1; cpPush(); }
  cScore = (uint16_t)(cDist / 8);
  uint8_t nl = 1 + cScore / 60;
  if (nl > cLevel) { cLevel = nl; cGap = imax(26, 44 - cLevel * 2); bannerLevel(nl); fx(SFX_LEVEL, C_CYAN, 200); }
  cFr++; for (uint8_t i = 5; i > 0; i--) cTr[i] = cTr[i - 1]; cTr[0] = cY;
  bool hit = false;
  for (uint8_t x = 24; x < 32; x++) {
    if (cY < cT[x] || cY + 5 >= cB[x]) hit = true;
    if (cH[x] && cY + 5 > cM[x] && cY < cM[x] + cH[x]) hit = true;
  }
  if (hit) { cDead = true; endRun(CP_ID, cScore, cHi); }
}
static void cpDraw() {
  for (uint8_t i = 0; i < 128; i++) {
    if (cT[i] > 8) u8g2.drawVLine(i, 8, cT[i] - 8);
    if (cB[i] < 64) u8g2.drawVLine(i, cB[i], 64 - cB[i]);
    if (cH[i]) u8g2.drawVLine(i, cM[i], cH[i]);
  }
  for (uint8_t i = 1; i < 6; i += 2) u8g2.drawPixel(22 - i * 3, (int)cTr[i] + 3);
  int y = (int)cY;
  u8g2.drawBox(24, y + 1, 7, 4); u8g2.drawBox(20, y + 2, 4, 1); u8g2.drawVLine(19, y, 3);
  u8g2.drawHLine(24 + ((cFr & 2) ? 0 : 1), y, (cFr & 2) ? 9 : 7); u8g2.drawHLine(24, y + 5, 7);
  u8g2.setDrawColor(0); u8g2.drawPixel(28, y + 2); u8g2.setDrawColor(1);
  char b[28]; u8g2.setFont(FONT_S);
  snprintf(b, sizeof b, "DIST %u LV%u  HI %u", cScore, cLevel, cHi); u8g2.setDrawColor(0); u8g2.drawBox(0, 0, 128, 8); u8g2.setDrawColor(1); u8g2.drawStr(1, 7, b);
  if (cDead) overBox(cScore, cHi);
}
static void cpIcon(int x, int y) {
  u8g2.drawBox(x, y, 32, 5); u8g2.drawBox(x, y + 27, 32, 5); u8g2.drawBox(x + 20, y + 5, 4, 5);
  u8g2.drawBox(x + 6, y + 14, 9, 5); u8g2.drawBox(x + 2, y + 15, 4, 1); u8g2.drawHLine(x + 5, y + 13, 11); u8g2.drawHLine(x + 6, y + 19, 8);
}
