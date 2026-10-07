#pragma once
// Tetris - 10x16 board. Stick L/R = move (hold repeats), Down = soft drop, A/Up = rotate, B = hard drop.
#define TT_ID 2
static const int8_t TP[7][4][2] = {
  {{-1,0},{0,0},{1,0},{2,0}}, {{0,0},{1,0},{0,1},{1,1}}, {{-1,0},{0,0},{1,0},{0,-1}},
  {{-1,0},{0,0},{1,0},{1,-1}}, {{-1,0},{0,0},{1,0},{-1,-1}}, {{-1,0},{0,0},{0,-1},{1,-1}}, {{-1,-1},{0,-1},{0,0},{1,0}}};
static uint16_t tB[16];
static int8_t tPc, tRot, tX, tY, tNx; static uint8_t tBag[7], tBi, tTick, tLevel;
static uint32_t tScore; static uint16_t tHi, tLines; static bool tDead;

static void tCells(int8_t pc, int8_t rot, int8_t px, int8_t py, int8_t out[4][2]) {
  for (uint8_t i = 0; i < 4; i++) {
    int8_t x = TP[pc][i][0], y = TP[pc][i][1];
    if (pc != 1) for (int8_t r = 0; r < rot; r++) { int8_t nx = -y, ny = x; x = nx; y = ny; }
    out[i][0] = px + x; out[i][1] = py + y;
  }
}
static bool tFits(int8_t pc, int8_t rot, int8_t px, int8_t py) {
  int8_t c[4][2]; tCells(pc, rot, px, py, c);
  for (uint8_t i = 0; i < 4; i++) {
    if (c[i][0] < 0 || c[i][0] >= 10 || c[i][1] >= 16) return false;
    if (c[i][1] >= 0 && ((tB[c[i][1]] >> c[i][0]) & 1)) return false;
  }
  return true;
}
static int8_t tDraw7() {
  if (tBi >= 7) { for (uint8_t i = 0; i < 7; i++) tBag[i] = i; for (int i = 6; i > 0; i--) { int j = random(i + 1); uint8_t t = tBag[i]; tBag[i] = tBag[j]; tBag[j] = t; } tBi = 0; }
  return tBag[tBi++];
}
static void tSpawn() {
  tPc = tNx; tNx = tDraw7(); tRot = 0; tX = 4; tY = 1; tTick = 0;
  if (!tFits(tPc, tRot, tX, tY)) { tDead = true; endRun(TT_ID, tScore > 65535 ? 65535 : (uint16_t)tScore, tHi); }
}
static void tInit() {
  for (uint8_t i = 0; i < 16; i++) tB[i] = 0;
  tScore = 0; tLines = 0; tLevel = 1; tDead = false; tBi = 7; tHi = hiGet(TT_ID);
  tNx = tDraw7(); tSpawn();
}
static void tLock() {
  int8_t c[4][2]; tCells(tPc, tRot, tX, tY, c);
  for (uint8_t i = 0; i < 4; i++) if (c[i][1] >= 0) tB[c[i][1]] |= (uint16_t)(1 << c[i][0]);
  uint8_t cleared = 0;
  for (int y = 15; y >= 0;) {
    if (tB[y] == 0x3FF) { for (int k = y; k > 0; k--) tB[k] = tB[k - 1]; tB[0] = 0; cleared++; } else y--;
  }
  if (cleared) {
    static const uint16_t pts[5] = {0, 40, 100, 300, 1200};
    tScore += pts[cleared] * tLevel; tLines += cleared;
    uint8_t nl = 1 + tLines / 8;
    if (nl > tLevel) { tLevel = nl; bannerLevel(nl); fx(SFX_LEVEL, C_CYAN, 250); } else fx(SFX_LINE, C_GOLD, 150);
  } else sfx(SFX_MOVE);
  tSpawn();
}
static void tUpdate() {
  if (tDead) { if (overKeys()) tInit(); return; }
  if (in.rx && tFits(tPc, tRot, tX + in.rx, tY)) tX += in.rx;
  if (in.aP || in.py < 0) {
    static const int8_t k[5] = {0, -1, 1, -2, 2};
    for (uint8_t i = 0; i < 5; i++) if (tFits(tPc, (tRot + 1) & 3, tX + k[i], tY)) { tRot = (tRot + 1) & 3; tX += k[i]; sfx(SFX_MOVE); break; }
  }
  if (in.bP) { int r = 0; while (tFits(tPc, tRot, tX, tY + 1)) { tY++; r++; } tScore += 2 * r; tLock(); return; }
  bool soft = in.dy > 0; tTick++;
  int iv = 28 - tLevel * 2; if (iv < 4) iv = 4; if (soft) iv = 2;
  if (tTick >= iv) { tTick = 0; if (tFits(tPc, tRot, tX, tY + 1)) { tY++; if (soft) tScore++; } else tLock(); }
}
static void tDraw() {
  u8g2.drawVLine(19, 0, 64); u8g2.drawVLine(60, 0, 64);
  for (uint8_t y = 0; y < 16; y++) for (uint8_t x = 0; x < 10; x++) if ((tB[y] >> x) & 1) u8g2.drawBox(20 + x * 4, y * 4, 3, 3);
  if (!tDead) {
    int8_t c[4][2]; int gy = tY; while (tFits(tPc, tRot, tX, gy + 1)) gy++;
    if (gy != tY) { tCells(tPc, tRot, tX, gy, c); for (uint8_t i = 0; i < 4; i++) if (c[i][1] >= 0) u8g2.drawPixel(21 + c[i][0] * 4, 1 + c[i][1] * 4); }
    tCells(tPc, tRot, tX, tY, c);
    for (uint8_t i = 0; i < 4; i++) if (c[i][1] >= 0) u8g2.drawBox(20 + c[i][0] * 4, c[i][1] * 4, 3, 3);
  }
  char b[16]; u8g2.setFont(FONT_S);
  u8g2.drawStr(68, 7, "SCORE"); snprintf(b, sizeof b, "%lu", (unsigned long)tScore); u8g2.drawStr(68, 15, b);
  u8g2.drawStr(68, 26, "LEVEL"); snprintf(b, sizeof b, "%u", tLevel); u8g2.drawStr(68, 34, b);
  u8g2.drawStr(68, 45, "LINES"); snprintf(b, sizeof b, "%u", tLines); u8g2.drawStr(68, 53, b);
  u8g2.drawStr(100, 7, "NEXT"); u8g2.drawFrame(98, 10, 26, 18);
  for (uint8_t i = 0; i < 4; i++) { int nx = TP[tNx][i][0], ny = TP[tNx][i][1]; u8g2.drawBox(103 + (nx + 1) * 4, 14 + (ny + 1) * 4, 3, 3); }
  snprintf(b, sizeof b, "HI %u", tHi); u8g2.drawStr(100, 40, b);
  if (tDead) overBox(tScore > 65535 ? 65535 : (uint16_t)tScore, tHi);
}
static void tIcon(int x, int y) {
  static const uint8_t m[6][6] = {{0,0,0,0,0,0},{0,1,0,0,0,0},{1,1,1,0,1,0},{0,0,1,1,1,1},{1,1,1,1,0,1},{1,0,1,1,1,1}};
  for (int r = 0; r < 6; r++) for (int c = 0; c < 6; c++) if (m[r][c]) u8g2.drawBox(x + c * 5 + 1, y + r * 5 + 1, 4, 4);
}
