#pragma once
// Snake - levels every 6 foods: faster + more rocks. Bonus star every 5th food (timed, worth 3).
#define SN_ID     5
#define SN_CW     32
#define SN_CH     14
#define SN_MAX    (SN_CW * SN_CH)
#define SN_STONES 24
static uint8_t snX[SN_MAX], snY[SN_MAX], stX[SN_STONES], stY[SN_STONES];
static uint16_t snLen, snScore, snHi, snEat; static uint8_t snStn, snLvl, snFx, snFy, snBx, snBy;
static int8_t snDx, snDy, snNx, snNy; static int16_t snBt; static bool snDead; static uint32_t snLast;

static bool snOcc(uint8_t x, uint8_t y) {
  for (uint16_t i = 0; i < snLen; i++) if (snX[i] == x && snY[i] == y) return true;
  for (uint8_t i = 0; i < snStn; i++) if (stX[i] == x && stY[i] == y) return true;
  return (x == snFx && y == snFy) || (snBt > 0 && x == snBx && y == snBy);
}
static void snPlace(uint8_t &ox, uint8_t &oy) {   // pick into temporaries: ox/oy may alias snFx/snBx
  uint8_t x, y; do { x = random(SN_CW); y = random(SN_CH); } while (snOcc(x, y)); ox = x; oy = y;
}
static void snInit() {
  snLen = 3; for (uint8_t i = 0; i < 3; i++) { snX[i] = 16 - i; snY[i] = 7; }
  snDx = snNx = 1; snDy = snNy = 0; snScore = 0; snEat = 0; snLvl = 1; snStn = 0; snBt = 0; snDead = false;
  snLast = millis(); snHi = hiGet(SN_ID); snFx = 255; snFy = 255; snPlace(snFx, snFy);
}
static void snAddStones() {
  for (uint8_t k = 0; k < 3 && snStn < SN_STONES; k++)
    for (uint8_t t = 0; t < 40; t++) {
      uint8_t x = random(SN_CW), y = random(SN_CH);
      if (abs((int)x - snX[0]) + abs((int)y - snY[0]) < 6 || snOcc(x, y)) continue;
      stX[snStn] = x; stY[snStn] = y; snStn++; break;
    }
}
static void snDie() { snDead = true; endRun(SN_ID, snScore, snHi); }
static void snUpdate() {
  if (snDead) { if (overKeys()) snInit(); return; }
  if (in.dx && snDx == 0) { snNx = in.dx; snNy = 0; } else if (in.dy && snDy == 0) { snNx = 0; snNy = in.dy; }
  if (snBt > 0) snBt--;
  uint32_t iv = (snLvl >= 10) ? 55 : (140 - snLvl * 9);
  if (millis() - snLast < iv) return;
  snLast = millis(); snDx = snNx; snDy = snNy;
  int nx = snX[0] + snDx, ny = snY[0] + snDy;
  if (nx < 0 || ny < 0 || nx >= SN_CW || ny >= SN_CH) { snDie(); return; }
  for (uint8_t i = 0; i < snStn; i++) if (stX[i] == nx && stY[i] == ny) { snDie(); return; }
  bool food = (nx == snFx && ny == snFy), bonus = (snBt > 0 && nx == snBx && ny == snBy), grow = food || bonus;
  uint16_t lim = grow ? snLen : snLen - 1;
  for (uint16_t i = 0; i < lim; i++) if (snX[i] == nx && snY[i] == ny) { snDie(); return; }
  if (grow && snLen < SN_MAX) snLen++;
  for (uint16_t i = snLen - 1; i > 0; i--) { snX[i] = snX[i - 1]; snY[i] = snY[i - 1]; }
  snX[0] = nx; snY[0] = ny;
  if (bonus) { snScore += 3; snBt = 0; fx(SFX_POWER, C_GOLD, 120); }
  if (food) {
    snScore++; snEat++; fx(SFX_COIN, C_GOOD, 70); snFx = 255; snPlace(snFx, snFy);
    if (snEat % 5 == 0 && snBt == 0) { snBt = 180; snPlace(snBx, snBy); }
    uint8_t nl = 1 + snEat / 6;
    if (nl > snLvl) { snLvl = nl; snAddStones(); bannerLevel(nl); fx(SFX_LEVEL, C_CYAN, 200); }
  }
}
static void snDraw() {
  char b[32]; u8g2.setFont(FONT_S);
  snprintf(b, sizeof b, "%u  LV%u  HI %u", snScore, snLvl, snHi); u8g2.drawStr(1, 7, b);
  if (snBt) u8g2.drawBox(100, 2, snBt / 6, 4);
  u8g2.drawFrame(0, 8, 128, 56);
  for (uint8_t i = 0; i < snStn; i++) { u8g2.drawFrame(stX[i] * 4, 8 + stY[i] * 4, 4, 4); u8g2.drawPixel(stX[i] * 4 + 1, 9 + stY[i] * 4); }
  for (uint16_t i = 0; i < snLen; i++) u8g2.drawBox(snX[i] * 4, 8 + snY[i] * 4, i ? 3 : 4, i ? 3 : 4);
  u8g2.drawBox(snFx * 4 + 1, 9 + snFy * 4, 2, 3); u8g2.drawBox(snFx * 4, 10 + snFy * 4, 4, 1);
  if (snBt) { if ((snBt / 6) & 1) u8g2.drawBox(snBx * 4, 8 + snBy * 4, 4, 4); else u8g2.drawFrame(snBx * 4, 8 + snBy * 4, 4, 4); }
  if (snDead) overBox(snScore, snHi);
}
static void snIcon(int x, int y) {
  for (int i = 0; i < 6; i++) u8g2.drawBox(x + 2 + i * 4, y + 20 - (i > 2 ? 8 : 0), 3, 3);
  for (int i = 0; i < 3; i++) u8g2.drawBox(x + 14, y + 12 + i * 4 + 4, 3, 3);
  u8g2.drawFrame(x + 24, y + 6, 5, 5); u8g2.drawBox(x + 4, y + 6, 4, 4);
}

