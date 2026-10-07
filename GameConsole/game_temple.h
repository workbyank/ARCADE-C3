#pragma once
// Temple Dash - endless runner, 3 lanes seen from behind. Stick left/right = change lane, A = jump (low barriers),
// B = slide (overhead beams), walls must be dodged by changing lane. Collect coins; speed rises with distance.
#define TR_ID 12
#define TR_N  10
struct TrObj { float d; int8_t lane; uint8_t t; };       // t: 0 barrier, 1 beam, 2 wall, 3 coin, 255 = unused
static TrObj trO[TR_N];
static float trX, trSp, trAcc; static int8_t trLane; static uint8_t trJ, trS; static uint16_t trCoins, trHi, trScore; static float trDist;
static bool trOver; static uint8_t trPhase;
static int trY(float d) { return 12 + (int)(46 * d * d); }
static int trLx(int lane, float d) { return 64 + (int)((lane - 1) * 36 * d); }
static void trSpawn(int8_t lane, uint8_t t) { for (uint8_t i = 0; i < TR_N; i++) if (trO[i].t == 255) { trO[i].d = 0.1f; trO[i].lane = lane; trO[i].t = t; return; } }
static void trInit() {
  for (uint8_t i = 0; i < TR_N; i++) trO[i].t = 255;
  trX = 1; trLane = 1; trJ = trS = 0; trSp = 0.011f; trAcc = 0; trCoins = 0; trScore = 0; trDist = 0; trOver = false; trHi = hiGet(TR_ID); trPhase = 0;
}
static void trRow() {                                       // one procedural row: obstacle(s) always leave a free lane
  uint8_t k = random(10); int8_t l = random(3);
  if (k < 3) trSpawn(l, 0); else if (k < 5) trSpawn(l, 1);
  else if (k < 7) { trSpawn(l, 2); if (random(2)) trSpawn((l + 1) % 3, 2); }
  else trSpawn(random(3), 3);   // coin
}
static void trEnd() { trOver = true; trScore = (uint16_t)(trDist / 4) + trCoins * 5; endRun(TR_ID, trScore, trHi); }
static void trUpdate() {
  if (trOver) { if (overKeys()) trInit(); return; }
  trPhase++; if (in.px < 0 && trLane > 0) { trLane--; sfx(SFX_MOVE); } if (in.px > 0 && trLane < 2) { trLane++; sfx(SFX_MOVE); }
  trX += (trLane - trX) * 0.35f;
  if (!trJ && !trS) { if (in.aP) { trJ = 1; sfx(SFX_JUMP); } else if (in.bP) { trS = 1; sfx(SFX_DUCK); } }
  if (trJ) { if (++trJ > 22) trJ = 0; }
  if (trS) { if (++trS > 18) trS = 0; }
  trSp = 0.011f + trDist * 0.0000045f; if (trSp > 0.03f) trSp = 0.03f;
  trDist += trSp * 100; trAcc += trSp;
  float gap = 0.30f - trSp * 4; if (gap < 0.16f) gap = 0.16f;
  if (trAcc >= gap) { trAcc = 0; trRow(); }
  int jh = trJ ? (int)(16 * 4 * trJ * (22 - trJ) / (22 * 22)) : 0;
  for (uint8_t i = 0; i < TR_N; i++) {
    TrObj &o = trO[i]; if (o.t == 255) continue;
    o.d += trSp; if (o.d > 1.12f) { o.t = 255; continue; }
    if (o.d < 0.9f || o.d > 1.02f || o.lane != trLane) continue;
    if (o.t == 3) { o.t = 255; trCoins++; fx(SFX_COIN, C_GOLD, 80); }
    else if ((o.t == 0 && jh < 7) || (o.t == 1 && !trS) || o.t == 2) { fx(SFX_HIT, C_BAD, 300); trEnd(); return; }
  }
}
static void trDraw() {
  char b[28]; u8g2.setFont(FONT_S); snprintf(b, sizeof b, "%um  C%u  HI %u", (unsigned)(trDist / 4), trCoins, trHi); textC(b, 7);
  for (int k = 0; k < 4; k++) { int xb = 10 + k * 36; u8g2.drawLine(64 + (xb - 64) / 6, 12, xb, 63); }   // lane lines to the vanishing point
  for (int k = 0; k < 5; k++) { float d = (k + (trPhase % 20) / 20.0f) / 5.0f; int y = trY(d), h = (int)(54 * d); if (d > 0.05f) u8g2.drawHLine(64 - h, y, 2 * h + 1); }
  for (uint8_t i = 0; i < TR_N; i++) {                      // far objects first (array order is spawn order)
    TrObj &o = trO[i]; if (o.t == 255) continue;
    int x = trLx(o.lane, o.d), y = trY(o.d), w = 3 + (int)(24 * o.d), h = 2 + (int)(18 * o.d);
    switch (o.t) {
      case 0: u8g2.drawBox(x - w / 2, y - h / 3, w, h / 3 + 1); break;                                   // low barrier
      case 1: u8g2.drawVLine(x - w / 2, y - h * 2, h * 2); u8g2.drawVLine(x + w / 2, y - h * 2, h * 2); u8g2.drawBox(x - w / 2, y - h * 2, w, 2 + h / 4); break;  // beam on posts
      case 2: u8g2.drawFrame(x - w / 2, y - h, w, h); u8g2.drawLine(x - w / 2, y - h, x + w / 2, y - 1); break;  // wall
      default: u8g2.drawDisc(x, y - h / 3, 1 + (int)(3 * o.d));                                          // coin
    }
  }
  int px = trLx(0, 1) + (int)(trX * 36), jh = trJ ? (int)(16 * 4 * trJ * (22 - trJ) / (22 * 22)) : 0, py = 60 - jh;
  if (trS) { u8g2.drawBox(px - 4, py - 4, 9, 4); u8g2.drawDisc(px + 4, py - 3, 2); }                      // sliding
  else { u8g2.drawDisc(px, py - 11, 2); u8g2.drawBox(px - 2, py - 8, 5, 6); int l = (trPhase >> 1) & 1; u8g2.drawVLine(px - 1 + l * 2, py - 2, 3); u8g2.drawVLine(px + 1 - l * 2, py - 2, 3); }
  if (jh) u8g2.drawHLine(px - 3, 61, 7);                                                                   // shadow
  if (trOver) overBox(trScore, trHi);
}
static void trIcon(int x, int y) {
  u8g2.drawLine(x + 16, y + 4, x + 2, y + 30); u8g2.drawLine(x + 16, y + 4, x + 30, y + 30);
  u8g2.drawDisc(x + 16, y + 14, 2); u8g2.drawBox(x + 14, y + 17, 5, 6); u8g2.drawBox(x + 4, y + 26, 10, 3); u8g2.drawDisc(x + 24, y + 22, 2);
}
