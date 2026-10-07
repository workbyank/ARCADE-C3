#pragma once
// Pong vs AI - every 5 points the level goes up: faster ball, smarter AI, shorter paddle. 3 lives.
#define PG_ID 7
static float pgBx, pgBy, pgVx, pgVy, pgPy, pgAy;
static uint8_t pgLives, pgLvl; static uint16_t pgScore, pgHi; static bool pgOver; static uint32_t pgServeAt;
static int pgPh() { int h = 16 - pgLvl; return h < 8 ? 8 : h; }
static void pgServe(int dir) {
  float sp = 1.8f + 0.12f * pgLvl; if (sp > 3.6f) sp = 3.6f;
  pgBx = 64; pgBy = 32; pgVx = dir * sp; pgVy = random(-10, 11) / 10.0f; pgServeAt = millis() + 700;
}
static void pgInit() { pgPy = pgAy = 25; pgLives = 3; pgLvl = 1; pgScore = 0; pgOver = false; pgHi = hiGet(PG_ID); pgServe(-1); }
static void pgHit(float py, int ph) {
  float off = ((pgBy + 1) - (py + ph / 2.0f)) / (ph / 2.0f);
  pgVx = -pgVx * 1.05f; if (pgVx > 4.8f) pgVx = 4.8f; if (pgVx < -4.8f) pgVx = -4.8f; pgVy = off * 2.2f;
}
static void pgUpdate() {
  if (pgOver) { if (overKeys()) pgInit(); return; }
  int ph = pgPh();
  pgPy += in.ay * 0.032f; if (pgPy < 0) pgPy = 0; if (pgPy > 64 - ph) pgPy = 64 - ph;
  float spd = 1.3f + 0.22f * pgLvl; if (spd > 4.0f) spd = 4.0f;
  float target = (pgVx > 0) ? pgBy - ph / 2.0f : 32 - ph / 2.0f, d = target - pgAy;
  if (d > spd) d = spd; if (d < -spd) d = -spd;
  pgAy += d; if (pgAy < 0) pgAy = 0; if (pgAy > 64 - ph) pgAy = 64 - ph;
  if (millis() < pgServeAt) return;
  pgBx += pgVx; pgBy += pgVy;
  if (pgBy < 9) { pgBy = 9; pgVy = -pgVy; } if (pgBy > 62) { pgBy = 62; pgVy = -pgVy; }
  if (pgVx < 0 && pgBx <= 6 && pgBx >= 2 && pgBy + 2 >= pgPy && pgBy <= pgPy + ph) { pgHit(pgPy, ph); pgBx = 7; sfx(SFX_SELECT); }
  if (pgVx > 0 && pgBx + 2 >= 122 && pgBx <= 125 && pgBy + 2 >= pgAy && pgBy <= pgAy + ph) { pgHit(pgAy, ph); pgBx = 119.5f; sfx(SFX_BACK); }
  if (pgBx < -2) {
    pgLives--; fx(SFX_HIT, C_BAD, 300);
    if (pgLives == 0) { pgOver = true; endRun(PG_ID, pgScore, pgHi); } else pgServe(-1);
  } else if (pgBx > 130) {
    pgScore++; uint8_t nl = 1 + pgScore / 5;
    if (nl > pgLvl) { pgLvl = nl; bannerLevel(nl); fx(SFX_LEVEL, C_CYAN, 200); } else fx(SFX_COIN, C_GOOD, 120);
    pgServe(-1);
  }
}
static void pgDraw() {
  char b[28]; u8g2.setFont(FONT_S); snprintf(b, sizeof b, "L%u  LV%u  %u  HI %u", pgLives, pgLvl, pgScore, pgHi); textC(b, 7);
  for (int y = 10; y < 64; y += 6) u8g2.drawVLine(64, y, 3);
  u8g2.drawBox(3, (int)pgPy, 3, pgPh()); u8g2.drawBox(122, (int)pgAy, 3, pgPh()); u8g2.drawBox((int)pgBx, (int)pgBy, 2, 2);
  if (pgOver) overBox(pgScore, pgHi);
}
static void pgIcon(int x, int y) {
  u8g2.drawBox(x + 2, y + 8, 3, 14); u8g2.drawBox(x + 27, y + 12, 3, 14); u8g2.drawBox(x + 16, y + 16, 3, 3);
  for (int i = 0; i < 32; i += 6) u8g2.drawVLine(x + 15, y + i, 3);
}
