#pragma once
// Space Defender - side-scrolling shooter. Waves -> levels, boss every 4th level, power-ups.
#define SD_ID 1
struct SdE { float x, y, ph; int16_t hp; uint8_t t, cd; bool on; };   // t: 0 drone, 1 zigzag, 2 gunner, 3 boss
struct SdB { float x, y, vx, vy; bool on; };
struct SdP { float x, y; uint8_t t; bool on; };                        // t: P(ower) S(hield) B(omb) H(eart)
static SdE sE[8]; static SdB sB[10], sEB[10]; static SdP sP[3];
static float sX, sY, sSx[14], sSy[14];
static int8_t sLives; static uint8_t sPow, sBombs, sLevel, sCool, sFlash;
static uint16_t sScore, sHi, sInv; static int16_t sToSpawn, sCd, sBan, sBossMax;
static bool sDead, sBossLv, sShield;

static uint8_t sdW(uint8_t t) { static const uint8_t w[4] = {8, 9, 10, 16}; return w[t]; }
static uint8_t sdH(uint8_t t) { static const uint8_t h[4] = {7, 6, 10, 16}; return h[t]; }

static void sdLevel() {
  sBossLv = (sLevel % 4 == 0);
  sToSpawn = sBossLv ? 1 : 6 + sLevel * 2; sCd = 50; sBan = 40;
  if (sBossLv) banner("BOSS!"); else bannerLevel(sLevel);
  if (sLevel > 1) { fx(SFX_LEVEL, C_CYAN, 200); if (sBombs < 3) sBombs++; }
}
static void sdInit() {
  sX = 10; sY = 32; sLives = 3; sPow = 1; sBombs = 1; sLevel = 1; sCool = 0; sFlash = 0; sInv = 0; sShield = false;
  sScore = 0; sDead = false; sHi = hiGet(SD_ID);
  for (uint8_t i = 0; i < 8; i++) sE[i].on = false;
  for (uint8_t i = 0; i < 10; i++) { sB[i].on = false; sEB[i].on = false; }
  for (uint8_t i = 0; i < 3; i++) sP[i].on = false;
  for (uint8_t i = 0; i < 14; i++) { sSx[i] = random(128); sSy[i] = random(9, 64); }
  sdLevel();
}
static void sdAdd(SdB* a, uint8_t n, float x, float y, float vx, float vy) {
  for (uint8_t i = 0; i < n; i++) if (!a[i].on) { a[i] = {x, y, vx, vy, true}; return; }
}
static void sdSpawn() {
  for (uint8_t i = 0; i < 8; i++) {
    if (sE[i].on) continue;
    SdE &e = sE[i]; e.on = true; e.x = 128; e.ph = random(628) / 100.0f; e.cd = random(40, 80);
    if (sBossLv) { e.t = 3; e.y = 24; e.hp = 25 + sLevel * 5; sBossMax = e.hp; }
    else {
      uint8_t r = random(100);
      e.t = (sLevel >= 3 && r < 25) ? 2 : ((sLevel >= 2 && r < 55) ? 1 : 0);
      e.y = random(14, 50); e.hp = (e.t == 2) ? 3 : 1;
    }
    return;
  }
}
static void sdDrop(float x, float y) {
  if (random(100) >= 14) return;
  for (uint8_t i = 0; i < 3; i++) if (!sP[i].on) { sP[i] = {x, y, (uint8_t)random(4), true}; return; }
}
static void sdDamage(SdE &e, int16_t d) {
  e.hp -= d;
  if (e.hp > 0) { ledFlash(C_WHITE, 30); return; }
  e.on = false;
  static const uint16_t pts[4] = {10, 15, 30, 500};
  sScore += pts[e.t]; fx(SFX_BOOM, C_ORANGE, 100); sdDrop(e.x, e.y);
  if (e.t == 3) { banner("BOSS DOWN!"); sdDrop(e.x, e.y + 4); }
}
static void sdHurt() {
  if (sInv) return;
  if (sShield) { sShield = false; sInv = 40; fx(SFX_HIT, C_CYAN, 150); return; }
  sLives--; sInv = 70; if (sPow > 1) sPow--; fx(SFX_HIT, C_BAD, 200);
  if (sLives <= 0) { sDead = true; endRun(SD_ID, sScore, sHi); }
}

static void sdUpdate() {
  if (sDead) { if (overKeys()) sdInit(); return; }
  if (sInv) sInv--;
  if (sFlash) sFlash--;
  sX += in.ax * 0.024f; sY += in.ay * 0.024f;
  if (sX < 2) sX = 2; if (sX > 60) sX = 60; if (sY < 10) sY = 10; if (sY > 56) sY = 56;
  if (in.a && sCool == 0) {
    sCool = 7;
    if (sPow == 1) sdAdd(sB, 10, sX + 10, sY + 3, 4, 0);
    else { sdAdd(sB, 10, sX + 10, sY + 1, 4, 0); sdAdd(sB, 10, sX + 10, sY + 5, 4, 0); }
    if (sPow >= 3) { sdAdd(sB, 10, sX + 8, sY, 3.8f, -0.7f); sdAdd(sB, 10, sX + 8, sY + 6, 3.8f, 0.7f); }
    sfx(SFX_SHOOT);
  }
  if (sCool) sCool--;
  if (in.bP && sBombs) {
    sBombs--; sFlash = 5; fx(SFX_BOOM, C_GOLD, 300);
    for (uint8_t i = 0; i < 10; i++) sEB[i].on = false;
    for (uint8_t i = 0; i < 8; i++) if (sE[i].on) sdDamage(sE[i], sE[i].t == 3 ? 10 : 4);
  }
  for (uint8_t i = 0; i < 14; i++) { sSx[i] -= 0.4f + (i % 3) * 0.4f; if (sSx[i] < 0) { sSx[i] = 128; sSy[i] = random(9, 64); } }
  for (uint8_t i = 0; i < 10; i++) {
    if (sB[i].on) { sB[i].x += sB[i].vx; sB[i].y += sB[i].vy; if (sB[i].x > 128 || sB[i].y < 8 || sB[i].y > 64) sB[i].on = false; }
    if (sEB[i].on) {
      sEB[i].x += sEB[i].vx; sEB[i].y += sEB[i].vy;
      if (sEB[i].x < -4 || sEB[i].y < 8 || sEB[i].y > 64) sEB[i].on = false;
      else if (hitBox(sEB[i].x, sEB[i].y, 3, 2, sX, sY, 10, 7)) { sEB[i].on = false; sdHurt(); }
    }
  }
  if (sBan > 0) sBan--;
  else if (sToSpawn > 0 && --sCd <= 0) { sdSpawn(); sToSpawn--; sCd = imax(14, 48 - sLevel * 3); }

  float spd = 0.8f + sLevel * 0.07f; if (spd > 1.8f) spd = 1.8f;
  for (uint8_t i = 0; i < 8; i++) {
    SdE &e = sE[i]; if (!e.on) continue;
    switch (e.t) {
      case 0: e.x -= spd; break;
      case 1: e.x -= spd; e.ph += 0.14f; e.y += sinf(e.ph) * 1.6f; break;
      case 2:
        e.x -= spd * 0.6f;
        if (e.cd == 0) {
          e.cd = 70;
          if (e.x < 120 && e.x > sX + 20) { float fr = (e.x - sX) / 1.6f, vy = (sY + 3 - (e.y + 5)) / (fr < 1 ? 1 : fr); if (vy > 1) vy = 1; if (vy < -1) vy = -1; sdAdd(sEB, 10, e.x, e.y + 4, -1.6f, vy); }
        } else e.cd--;
        break;
      default:
        if (e.x > 100) e.x -= 0.8f; else { e.ph += 0.05f; e.y = 24 + sinf(e.ph) * 18; }
        if (e.cd == 0) { e.cd = 45; for (int k = -1; k <= 1; k++) sdAdd(sEB, 10, e.x, e.y + 7, -1.5f, k * 0.8f); } else e.cd--;
    }
    if (e.y < 9) e.y = 9; if (e.y > 64 - sdH(e.t)) e.y = 64 - sdH(e.t);
    if (e.x < -18) { e.on = false; continue; }
    for (uint8_t k = 0; k < 10; k++) if (sB[k].on && hitBox(sB[k].x, sB[k].y, 3, 2, e.x, e.y, sdW(e.t), sdH(e.t))) { sB[k].on = false; sdDamage(e, 1); if (!e.on) break; }
    if (!e.on) continue;
    if (hitBox(sX, sY, 10, 7, e.x, e.y, sdW(e.t), sdH(e.t))) { sdHurt(); if (e.t < 3) sdDamage(e, 9); }
  }
  for (uint8_t i = 0; i < 3; i++) {
    if (!sP[i].on) continue;
    sP[i].x -= 0.8f; if (sP[i].x < -9) { sP[i].on = false; continue; }
    if (hitBox(sX, sY, 10, 7, sP[i].x, sP[i].y, 9, 9)) {
      sP[i].on = false; fx(SFX_POWER, C_GOOD, 150);
      switch (sP[i].t) { case 0: if (sPow < 3) sPow++; break; case 1: sShield = true; break;
                         case 2: if (sBombs < 3) sBombs++; break; default: if (sLives < 5) sLives++; }
    }
  }
  if (sToSpawn == 0 && sBan == 0) {
    bool any = false; for (uint8_t i = 0; i < 8; i++) any |= sE[i].on;
    if (!any) { sScore += 100 * sLevel; sLevel++; sdLevel(); }
  }
}

static void sdDraw() {
  for (uint8_t i = 0; i < 14; i++) u8g2.drawPixel((int)sSx[i], (int)sSy[i]);
  for (uint8_t i = 0; i < 10; i++) { if (sB[i].on) u8g2.drawBox((int)sB[i].x, (int)sB[i].y, 3, 1); if (sEB[i].on) u8g2.drawBox((int)sEB[i].x, (int)sEB[i].y, 3, 2); }
  for (uint8_t i = 0; i < 8; i++) {
    if (!sE[i].on) continue;
    int ex = (int)sE[i].x, ey = (int)sE[i].y;
    switch (sE[i].t) {
      case 0: u8g2.drawBox(ex, ey + 2, 8, 3); u8g2.drawBox(ex + 2, ey, 4, 7); break;
      case 1: u8g2.drawBox(ex, ey + 2, 9, 2); u8g2.drawBox(ex + 2, ey, 5, 2); u8g2.drawBox(ex + 2, ey + 4, 5, 2); break;
      case 2: u8g2.drawFrame(ex, ey, 10, 10); u8g2.drawBox(ex + 3, ey + 3, 4, 4); break;
      default:
        u8g2.drawBox(ex, ey, 16, 16); u8g2.setDrawColor(0); u8g2.drawBox(ex + 3, ey + 4, 4, 4); u8g2.drawBox(ex + 9, ey + 4, 4, 4); u8g2.drawHLine(ex + 3, ey + 12, 10); u8g2.setDrawColor(1);
        u8g2.drawFrame(86, 9, 40, 4); u8g2.drawBox(88, 10, imax(0, sE[i].hp * 36 / sBossMax), 2);
    }
  }
  for (uint8_t i = 0; i < 3; i++) {
    if (!sP[i].on) continue;
    char s[2] = {"PSBH"[sP[i].t], 0}; u8g2.setFont(FONT_S);
    u8g2.drawFrame((int)sP[i].x, (int)sP[i].y, 9, 9); u8g2.drawStr((int)sP[i].x + 2, (int)sP[i].y + 8, s);
  }
  if (!(sInv && ((sInv / 4) & 1))) {
    int x = (int)sX, y = (int)sY;
    u8g2.drawBox(x, y + 2, 9, 3); u8g2.drawBox(x + 1, y, 3, 7); u8g2.drawBox(x + 9, y + 3, 1, 1);
  }
  if (sShield) u8g2.drawCircle((int)sX + 5, (int)sY + 3, 8);
  if (sFlash) { u8g2.setDrawColor(2); u8g2.drawBox(0, 0, 128, 64); u8g2.setDrawColor(1); }
  char b[28]; u8g2.setFont(FONT_S);
  snprintf(b, sizeof b, "%05u L%u H%d B%u P%u", sScore, sLevel, sLives, sBombs, sPow); u8g2.drawStr(1, 7, b);
  if (sDead) overBox(sScore, sHi);
}
static void sdIcon(int x, int y) {
  u8g2.drawBox(x + 2, y + 14, 9, 3); u8g2.drawBox(x + 3, y + 12, 3, 7); u8g2.drawBox(x + 11, y + 15, 1, 1);
  u8g2.drawBox(x + 15, y + 15, 3, 1); u8g2.drawBox(x + 21, y + 15, 3, 1);
  u8g2.drawBox(x + 25, y + 11, 6, 3); u8g2.drawBox(x + 26, y + 9, 4, 7);
  u8g2.drawPixel(x + 6, y + 4); u8g2.drawPixel(x + 20, y + 6); u8g2.drawPixel(x + 14, y + 26); u8g2.drawPixel(x + 28, y + 24);
}
