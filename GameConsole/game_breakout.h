#pragma once
// Breakout - 5 brick layouts that repeat harder. Stick L/R = paddle, A (or up) = launch. W = wide paddle, L = extra life.
#define BO_ID 6
#define BO_C  8
#define BO_R  5
static uint8_t boBr[BO_R][BO_C];
static float boBx, boBy, boVx, boVy, boPx;
static uint8_t boLives, boLvl, boLeft; static uint16_t boScore, boHi, boWide; static bool boLaunched, boOver;
struct BoC { float x, y; uint8_t t; bool on; };
static BoC boC[3];
static int boPw() { return boWide ? 32 : 20; }

static void boLoad(uint8_t lv) {
  boLeft = 0; uint8_t p = (lv - 1) % 5;
  for (uint8_t r = 0; r < BO_R; r++) for (uint8_t c = 0; c < BO_C; c++) {
    uint8_t h = 0;
    switch (p) {
      case 0: h = (r < 4) ? 1 : 0; break;
      case 1: h = ((r + c) & 1) ? 1 : 0; break;
      case 2: h = (r < 4 && c >= r && c < BO_C - r) ? 1 : 0; break;
      case 3: h = (c & 1) ? 0 : ((r < 2) ? 2 : 1); break;
      default: h = (r < 2) ? 2 : 1;
    }
    if (lv > 5 && h == 1 && ((r + c) % 3 == 0)) h = 2;
    boBr[r][c] = h; if (h) boLeft++;
  }
  for (uint8_t i = 0; i < 3; i++) boC[i].on = false;
  boLaunched = false;
}
static void boInit() {
  boLvl = 1; boLives = 3; boScore = 0; boWide = 0; boOver = false; boPx = 54; boHi = hiGet(BO_ID); boLoad(1);
}
static bool boBrick() {
  int cx = (int)(boBx + 1), cy = (int)(boBy + 1);
  if (cy < 10) return false;
  int row = (cy - 10) / 6, col = cx / 16;
  if (row >= BO_R || col < 0 || col >= BO_C) return false;
  uint8_t &h = boBr[row][col]; if (!h) return false;
  h--;
  if (h == 0) {
    boLeft--; boScore += 10; fx(SFX_COIN, C_ORANGE, 60);
    if (random(100) < 12) for (uint8_t i = 0; i < 3; i++) if (!boC[i].on) { boC[i] = {(float)(col * 16 + 5), (float)(10 + row * 6), (uint8_t)random(2), true}; break; }
  } else { boScore += 2; sfx(SFX_MOVE); }
  if (boLeft == 0) { boLvl++; bannerLevel(boLvl); fx(SFX_LEVEL, C_GOOD, 300); boLoad(boLvl); }
  return true;
}
static void boUpdate() {
  if (boOver) { if (overKeys()) boInit(); return; }
  if (boWide) boWide--;
  boPx += in.ax * 0.045f; int pw = boPw(); if (boPx < 0) boPx = 0; if (boPx > 128 - pw) boPx = 128 - pw;
  for (uint8_t i = 0; i < 3; i++) {
    if (!boC[i].on) continue;
    boC[i].y += 1.0f; if (boC[i].y > 64) { boC[i].on = false; continue; }
    if (hitBox(boC[i].x, boC[i].y, 6, 5, boPx, 60, pw, 3)) {
      boC[i].on = false;
      if (boC[i].t == 0) boWide = 600; else if (boLives < 5) boLives++;
      fx(SFX_POWER, C_GOOD, 150);
    }
  }
  if (!boLaunched) {
    boBx = boPx + pw / 2 - 1; boBy = 57;
    if (in.aP || in.py < 0) {
      boLaunched = true; boVx = random(2) ? 1.2f : -1.2f;
      float s = 1.8f + 0.15f * boLvl; if (s > 3.2f) s = 3.2f; boVy = -s; sfx(SFX_JUMP);
    }
    return;
  }
  boBx += boVx;
  if (boBx < 0) { boBx = 0; boVx = -boVx; } if (boBx > 126) { boBx = 126; boVx = -boVx; }
  if (boBrick()) boVx = -boVx;
  boBy += boVy; if (boBy < 8) { boBy = 8; boVy = -boVy; }
  if (boBrick()) boVy = -boVy;
  if (boLaunched && boVy > 0 && boBy + 2 >= 60 && boBy <= 62 && boBx + 2 >= boPx && boBx <= boPx + pw) {
    float sp = fabsf(boVy) * 1.03f; if (sp > 3.4f) sp = 3.4f;
    boVy = -sp; boBy = 57.5f; boVx = ((boBx + 1) - (boPx + pw / 2.0f)) / (pw / 2.0f) * 2.4f; sfx(SFX_SELECT);
  }
  if (boBy > 64) {
    boLives--; fx(SFX_HIT, C_BAD, 250); boLaunched = false;
    if (boLives == 0) { boOver = true; endRun(BO_ID, boScore, boHi); }
  }
}
static void boDraw() {
  char b[28]; u8g2.setFont(FONT_S);
  snprintf(b, sizeof b, "L%u LV%u %u HI %u", boLives, boLvl, boScore, boHi); u8g2.drawStr(1, 7, b);
  for (uint8_t r = 0; r < BO_R; r++) for (uint8_t c = 0; c < BO_C; c++) if (boBr[r][c]) {
    u8g2.drawBox(c * 16 + 1, 10 + r * 6, 14, 4);
    if (boBr[r][c] == 1 && boLvl >= 1) { u8g2.setDrawColor(0); u8g2.drawHLine(c * 16 + 3, 11 + r * 6, 10); u8g2.setDrawColor(1); }
  }
  for (uint8_t i = 0; i < 3; i++) if (boC[i].on) { u8g2.drawFrame((int)boC[i].x, (int)boC[i].y, 7, 6); u8g2.drawPixel((int)boC[i].x + 3, (int)boC[i].y + 2); u8g2.drawPixel((int)boC[i].x + (boC[i].t ? 2 : 4), (int)boC[i].y + 3); }
  u8g2.drawBox((int)boPx, 60, boPw(), 3); u8g2.drawBox((int)boBx, (int)boBy, 2, 2);
  if (!boLaunched && !boOver) { u8g2.setFont(FONT_S); textC("A: launch", 52); }
  if (boOver) overBox(boScore, boHi);
}
static void boIcon(int x, int y) {
  for (int r = 0; r < 3; r++) for (int c = 0; c < 4; c++) u8g2.drawBox(x + c * 8, y + 3 + r * 5, 7, 3);
  u8g2.drawBox(x + 8, y + 27, 14, 3); u8g2.drawBox(x + 20, y + 19, 2, 2);
}

