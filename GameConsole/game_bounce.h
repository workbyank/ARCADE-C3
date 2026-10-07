#pragma once
// Bouncing Ball - gravity ball, return it with the paddle (stick left/right). Every 5 returns: higher gravity = faster ball,
// narrower paddle. Hit with the paddle edge to steer the ball. Miss = game over.
#define BB_ID 11
static float bbX, bbY, bbVx, bbVy, bbG, bbPx; static uint8_t bbLvl, bbTr[5][2]; static uint16_t bbScore, bbHi;
static bool bbOver; static uint32_t bbServe; static uint8_t bbFlash;
static int bbPw() { int w = 24 - 2 * (bbLvl - 1); return w < 12 ? 12 : w; }
static void bbInit() {
  bbPx = 54; bbLvl = 1; bbG = 0.20f; bbScore = 0; bbOver = false; bbHi = hiGet(BB_ID); bbFlash = 0;
  bbX = 20 + random(88); bbY = 12; bbVx = random(2) ? 0.8f : -0.8f; bbVy = 0; bbServe = millis() + 600;
  for (uint8_t i = 0; i < 5; i++) { bbTr[i][0] = bbX; bbTr[i][1] = bbY; }
}
static void bbUpdate() {
  if (bbOver) { if (overKeys()) bbInit(); return; }
  int pw = bbPw(); bbPx += in.ax * 0.07f; if (bbPx < 0) bbPx = 0; if (bbPx > 128 - pw) bbPx = 128 - pw;
  if (bbFlash) bbFlash--;
  if ((int32_t)(millis() - bbServe) < 0) return;
  for (uint8_t i = 4; i > 0; i--) { bbTr[i][0] = bbTr[i - 1][0]; bbTr[i][1] = bbTr[i - 1][1]; }
  bbTr[0][0] = (uint8_t)bbX; bbTr[0][1] = (uint8_t)bbY;
  bbVy += bbG; bbX += bbVx; bbY += bbVy;
  if (bbX < 2) { bbX = 2; bbVx = -bbVx; sfx(SFX_MOVE); } if (bbX > 125) { bbX = 125; bbVx = -bbVx; sfx(SFX_MOVE); }
  if (bbY < 11) { bbY = 11; bbVy = -bbVy * 0.8f; sfx(SFX_MOVE); }
  if (bbVy > 0 && bbY >= 56 && bbY - bbVy <= 60 && bbX + 2 >= bbPx && bbX - 2 <= bbPx + pw) {   // paddle return
    float off = (bbX - (bbPx + pw / 2.0f)) / (pw / 2.0f + 2);                                // -1..1
    bbY = 56; bbVy = -sqrtf(2 * bbG * 46.0f);                                               // peak ~46 px, any gravity
    bbVx = off * (2.2f + 0.12f * bbLvl) + in.ax * 0.012f; if (bbVx > 4) bbVx = 4; if (bbVx < -4) bbVx = -4;
    bbScore++; bbFlash = 4; uint8_t nl = 1 + bbScore / 5;
    if (nl > bbLvl) { bbLvl = nl; bbG += 0.025f; if (bbG > 0.6f) bbG = 0.6f; bannerLevel(nl); fx(SFX_LEVEL, C_CYAN, 200); }
    else fx(SFX_COIN, C_GOOD, 90);
  }
  if (bbY > 66) { bbOver = true; endRun(BB_ID, bbScore, bbHi); }
}
static void bbDraw() {
  char b[32]; u8g2.setFont(FONT_S); snprintf(b, sizeof b, "SCORE %u  LV%u  HI %u", bbScore, bbLvl, bbHi); textC(b, 7);
  u8g2.drawHLine(0, 9, 128);
  for (uint8_t i = 4; i < 5; i++) u8g2.drawPixel(bbTr[i][0], bbTr[i][1]);                  // short trail
  u8g2.drawPixel(bbTr[2][0], bbTr[2][1]); u8g2.drawPixel(bbTr[0][0], bbTr[0][1]);
  u8g2.drawDisc((int)bbX, (int)bbY, 2);
  if (bbFlash) u8g2.drawFrame((int)bbPx - bbFlash, 58 - bbFlash, bbPw() + 2 * bbFlash, 3 + 2 * bbFlash);
  u8g2.drawBox((int)bbPx, 59, bbPw(), 3);
  if (bbOver) overBox(bbScore, bbHi);
}
static void bbIcon(int x, int y) {
  for (int i = 0; i < 6; i++) u8g2.drawPixel(x + 6 + i * 3, y + 24 - (i * (9 - i)) / 2 - 4);   // arc
  u8g2.drawDisc(x + 22, y + 10, 3); u8g2.drawBox(x + 6, y + 28, 20, 3);
}
