// ARCADE - multi-game console for ESP32-C3 Super Mini + 128x64 SSD1306 OLED
// Library needed: "U8g2" by olikraus (Library Manager). Board: ESP32C3 Dev Module, USB CDC On Boot: Enabled.
// Pins / options: config.h.  Add games: games.h.
//
// CONTROLS
//   Stick       move / navigate           A  = main action (select, jump, fire...)
//   B           secondary (duck, bomb...)  Stick click (SW) = PAUSE MENU (resume / restart / sound / quit)
//   Touch pad   mute / unmute (hold ~0.2 s)
#include "hal.h"
#include "games.h"

enum Mode : uint8_t { MODE_MENU, MODE_INTRO, MODE_PLAY, MODE_PAUSE };
static Mode     mode = MODE_MENU;
static uint8_t  sel = 0, cur = 0, pauseSel = 0;
static uint32_t menuLock = 0;       // ignore input briefly after entering the menu (no accidental launch)
static int16_t  menuTarget = 0;      // unbounded card index so wrap-around animates smoothly
static float    menuPos = 0;         // animated position (card units)
static uint16_t hiCache[16];

static void enterMenu() {
  mode = MODE_MENU; audioStop(); bannerUntil = 0; toastUntil = 0; gotoMenu = false; ledFlashEnd = 0; overAt = 0;
  sel = cur; menuTarget = sel; menuPos = sel; menuLock = millis() + 200;
  for (uint8_t i = 0; i < NGAMES && i < 16; i++) hiCache[i] = GAMES[i].kind ? 0 : hiGet(i);
  ledSetBase(GAMES[sel].r, GAMES[sel].g, GAMES[sel].b);
}
static void startGame(uint8_t i) {
  cur = i; mode = MODE_PLAY; bannerUntil = 0; toastUntil = 0; gotoMenu = false;
  ledSetBase(GAMES[i].r, GAMES[i].g, GAMES[i].b);
  GAMES[i].init(); fx(SFX_START, C_WHITE, 150);
}

// ------------------------------------------------------------------ launcher
static void menuInput() {
  if ((int32_t)(millis() - menuLock) < 0) return;
  int8_t d = in.rx ? in.rx : in.ry;
  if (d) {
    menuTarget += d; sel = ((menuTarget % NGAMES) + NGAMES) % NGAMES; sfx(SFX_MOVE);
    ledSetBase(GAMES[sel].r, GAMES[sel].g, GAMES[sel].b);
  }
  if (in.aP || in.swP) {
    sfx(SFX_SELECT);
    if (GAMES[sel].kind) startGame(sel); else { cur = sel; mode = MODE_INTRO; }
  }
}
static void menuCard(int cx, int y, uint8_t g, float near) {   // near: 1 = selected .. 0 = side card
  int x = cx - 18;
  if (near > 0.6f) {
    u8g2.drawRFrame(x, y, 36, 36, 3);
    if ((millis() / 400) & 1) u8g2.drawRFrame(x - 1, y - 1, 38, 38, 4);    // blinking selection halo
    GAMES[g].icon(x + 2, y + 2);
  } else {
    u8g2.drawRFrame(x + 2, y + 2, 32, 32, 3); GAMES[g].icon(x + 2, y + 2);
    u8g2.setDrawColor(0);   // dim side cards with a checker mask
    for (int j = 0; j < 36; j += 2) for (int i = (j >> 1) & 1; i < 36; i += 2) u8g2.drawPixel(x + i, y + j);
    u8g2.setDrawColor(1);
  }
}
static void menuDraw() {
  if (!ledFlashEnd) {
    float k = 0.45f + 0.55f * (0.5f + 0.5f * sinf(millis() / 350.0f));
    ledApply((uint8_t)(GAMES[sel].r * k), (uint8_t)(GAMES[sel].g * k), (uint8_t)(GAMES[sel].b * k));
  }
  menuPos += (menuTarget - menuPos) * 0.38f; if (fabsf(menuTarget - menuPos) < 0.02f) menuPos = menuTarget;
  char b[16];
  u8g2.setFont(FONT_S); u8g2.setDrawColor(1); u8g2.drawBox(0, 0, 128, 9); u8g2.setDrawColor(0);
  u8g2.drawStr(2, 7, "ARCADE"); snprintf(b, sizeof b, "%u/%u", sel + 1, NGAMES); u8g2.drawStr(64 - u8g2.getStrWidth(b) / 2, 7, b);
  if (hiCache[sel]) { snprintf(b, sizeof b, "HI %u", hiCache[sel]); u8g2.drawStr(126 - 14 - u8g2.getStrWidth(b) - 4, 7, b); }
  drawSpeaker(117, 1, cfg.sound); u8g2.setDrawColor(1);
  int base = (int)floorf(menuPos);
  for (int i = base - 1; i <= base + 2; i++) {
    float d = i - menuPos, ad = fabsf(d);
    if (ad > 1.7f) continue;
    uint8_t g = ((i % NGAMES) + NGAMES) % NGAMES;
    menuCard(64 + (int)(d * 46), 11 + (ad > 1 ? 3 : (int)(ad * 3)), g, ad < 0.5f ? 1.0f : 0.0f);
  }
  u8g2.setFont(FONT_M); textC(GAMES[sel].name, 55);
  u8g2.setFont(FONT_S); textC(GAME_DESC[sel], 63);
}

// ------------------------------------------------------------------ intro card (controls)
static void introInput() {
  if (in.bP) { sfx(SFX_BACK); enterMenu(); }
  else if (in.aP || in.swP) startGame(cur);
}
static void introDraw() {
  const Game& g = GAMES[cur];
  u8g2.setDrawColor(1); u8g2.drawBox(0, 0, 128, 11); u8g2.setDrawColor(0); u8g2.setFont(FONT_M); textC(g.name, 9); u8g2.setDrawColor(1);
  u8g2.drawFrame(2, 14, 36, 36); g.icon(4, 16);
  u8g2.setFont(FONT_S);
  u8g2.drawStr(42, 22, g.help1); u8g2.drawStr(42, 31, g.help2); u8g2.drawStr(42, 40, "SW: pause menu");
  char b[16]; snprintf(b, sizeof b, "BEST %u", hiCache[cur]); u8g2.drawStr(42, 49, b);
  u8g2.drawHLine(0, 53, 128); u8g2.drawStr(2, 62, "A:START"); u8g2.drawStr(128 - 7 * 5 - 2, 62, "B:BACK");
}

// ------------------------------------------------------------------ play + pause
static void playInput() {
  if (GAMES[cur].kind) { if (in.swP || gotoMenu) { gotoMenu = false; sfx(SFX_BACK); enterMenu(); } else GAMES[cur].update(); return; }
  if (in.swP) { mode = MODE_PAUSE; pauseSel = 0; audioStop(); sfx(SFX_SELECT); return; }
  if (gotoMenu) { gotoMenu = false; sfx(SFX_BACK); enterMenu(); return; }
  GAMES[cur].update();
}
static void pauseInput() {
  if (in.py < 0) { pauseSel = (pauseSel + 3) % 4; sfx(SFX_MOVE); }
  if (in.py > 0) { pauseSel = (pauseSel + 1) % 4; sfx(SFX_MOVE); }
  if (in.bP) { mode = MODE_PLAY; sfx(SFX_BACK); return; }
  if (!in.aP && !in.swP) return;                // A or stick-click confirms the highlighted item
  switch (pauseSel) {
    case 0: mode = MODE_PLAY; sfx(SFX_BACK); break;
    case 1: GAMES[cur].init(); mode = MODE_PLAY; fx(SFX_START, C_WHITE, 150); break;
    case 2: cfg.sound = !cfg.sound; cfgSave(); if (!cfg.sound) audioStop(); else sfx(SFX_SELECT); break;
    default: sfx(SFX_BACK); enterMenu();
  }
}
static void pauseDraw() {
  u8g2.setDrawColor(0); u8g2.drawBox(22, 4, 84, 57); u8g2.setDrawColor(1); u8g2.drawFrame(22, 4, 84, 57);
  u8g2.setFont(FONT_M); textC("PAUSED", 15); u8g2.setFont(FONT_S);
  const char* it[4] = {"Resume", "Restart", cfg.sound ? "Sound: ON" : "Sound: OFF", "Quit to menu"};
  for (uint8_t i = 0; i < 4; i++) {
    int y = 27 + i * 9; bool on = (i == pauseSel);
    if (on) u8g2.drawBox(26, y - 7, 76, 9);
    u8g2.setDrawColor(on ? 0 : 1); u8g2.drawStr(30, y, it[i]); u8g2.setDrawColor(1);
  }
}

// ------------------------------------------------------------------ boot
// scaled gamepad logo (s = 0..1.3), centred at cx,cy
static void bootLogo(int cx, int cy, float s) {
  int w = (int)(26 * s), h = (int)(14 * s); if (w < 2) return;
  u8g2.drawRBox(cx - w, cy - h, w * 2, h * 2, imax(1, (int)(5 * s)));
  u8g2.setDrawColor(0);
  int px = cx - w * 5 / 8, dp = imax(1, (int)(2 * s));                  // d-pad
  u8g2.drawBox(px - dp * 2, cy - dp / 2, dp * 4 + 1, dp); u8g2.drawBox(px - dp / 2, cy - dp * 2, dp, dp * 4 + 1);
  u8g2.drawDisc(cx + w / 2, cy - h / 4, imax(1, (int)(2 * s))); u8g2.drawDisc(cx + w * 3 / 4, cy + h / 5, imax(1, (int)(2 * s)));
  u8g2.drawBox(cx - w / 4, cy + h / 3, imax(1, w / 5), imax(1, dp / 2)); u8g2.drawBox(cx, cy + h / 3, imax(1, w / 5), imax(1, dp / 2));
  u8g2.setDrawColor(1);
}
static void bootShutter(uint32_t ms, bool open, bool menu) {   // black bars close (logo) or open (menu) from the middle
  uint32_t t0 = millis();
  for (;;) {
    uint32_t e = millis() - t0; if (e >= ms) break;
    int w = (int)(64L * e / ms); if (open) w = 64 - w;
    u8g2.clearBuffer();
    if (menu) menuDraw(); else { bootLogo(64, 24, 1.0f); u8g2.setFont(FONT_M); textC("ARCADE", 52); }
    u8g2.setDrawColor(0);
    if (menu) { u8g2.drawBox(0, 0, w, 64); u8g2.drawBox(128 - w, 0, w, 64); }          // curtains parting toward the edges
    else { u8g2.drawBox(0, 0, 128, w / 2); u8g2.drawBox(0, 64 - w / 2, 128, w / 2); }   // lids closing on the logo
    u8g2.setDrawColor(1); u8g2.sendBuffer(); audioUpdate();
  }
}
void setup() {
  cfgLoad(); randomSeed(esp_random());
  displayInit(); inputInit(); audioInit();
  ledApply(0, 0, 0); sfx(SFX_BOOT);
  static uint8_t pX[14], pY[14], pS[14];                       // sparkle particles
  for (uint8_t i = 0; i < 14; i++) { pX[i] = random(128); pY[i] = random(64); pS[i] = 1 + random(3); }
  const uint32_t T = 1700; uint32_t t0 = millis(), e;
  while ((e = millis() - t0) < T) {
    u8g2.clearBuffer();
    for (uint8_t i = 0; i < 14; i++) {                         // particles drift up, twinkle
      pY[i] = (pY[i] + 64 - pS[i] / 2) & 63; if (((e / 90) + i) % 3) u8g2.drawPixel(pX[i], pY[i]);
    }
    float p = e / 650.0f; if (p > 1) p = 1;                     // ease-out-back pop-in
    float q = p - 1, s = 1 + 2.70158f * q * q * q + 1.70158f * q * q;
    int cy = 24, r = (int)(16 + 30 * (e / (float)T));           // expanding dotted glow ring
    if (e > 150) for (int a = 0; a < 48; a += 1) if (((a + e / 40) & 3) == 0) {
      float an = a * 0.1309f; int x = 64 + (int)(cosf(an) * r * 1.6f), y = cy + (int)(sinf(an) * r); if (x >= 0 && x < 128 && y >= 0 && y < 64) u8g2.drawPixel(x, y);
    }
    bootLogo(64, cy, s);
    if (e > 600) {                                               // wordmark pops in with stepped font sizes
      u8g2.setFont(e < 700 ? u8g2_font_6x10_tf : (e < 800 ? u8g2_font_helvB10_tf : u8g2_font_helvB14_tf)); textC("ARCADE", e < 700 ? 50 : (e < 800 ? 52 : 56));
    }
    if (e > 1000) { u8g2.setFont(FONT_S); textC("ESP32-C3 GAME CONSOLE", 63); }
    int sy = (int)((e * 3) % 190) - 30;                          // scanline sweep with dither trail
    if (sy >= 0 && sy < 64) { u8g2.setDrawColor(2); u8g2.drawHLine(0, sy, 128); u8g2.setDrawColor(1); for (int x = sy & 1; x < 128; x += 2) if (sy > 2) u8g2.drawPixel(x, sy - 2); }
    u8g2.sendBuffer();
    uint8_t k = (uint8_t)(220 * (e < 650 ? e / 650.0f : 1.0f - (e - 650) / (float)(T - 650) * 0.6f)); ledApply(k / 3, k / 2, k);
    audioUpdate();
  }
  bootShutter(220, false, false);
  ledApply(0, 0, 0); enterMenu();
  bootShutter(300, true, true);
}

void loop() {
  uint32_t t0 = millis();
  inputUpdate(); audioUpdate(); ledUpdate();
  if (in.touchP) {
    cfg.sound = !cfg.sound; cfgSave();
    if (!cfg.sound) audioStop(); else sfx(SFX_SELECT);
    toast(cfg.sound ? "SOUND ON" : "SOUND OFF");
  }
  switch (mode) {
    case MODE_MENU:  menuInput();  break;
    case MODE_INTRO: introInput(); break;
    case MODE_PLAY:  playInput();  break;
    default:         pauseInput();
  }
  u8g2.clearBuffer();
  switch (mode) {
    case MODE_MENU:  menuDraw();  break;
    case MODE_INTRO: introDraw(); break;
    case MODE_PLAY:  GAMES[cur].draw(); break;
    default:         GAMES[cur].draw(); pauseDraw();
  }
  toastDraw(); bannerDraw();
  u8g2.sendBuffer();
  while (millis() - t0 < FRAME_MS) delay(1);
}
