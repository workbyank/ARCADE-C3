#pragma once
// ---------------------------------------------------------------- Settings
static uint8_t stSel = 0, stTest = 0, stHold = 0;
static const char* const LED_NAMES[4] = {"OFF", "LOW", "MED", "HIGH"};
static void stInit() { stSel = 0; stHold = 0; }
static void stUpdate() {
  const uint8_t N = 6;
  if (in.py < 0) { stSel = (stSel + N - 1) % N; sfx(SFX_MOVE); }
  if (in.py > 0) { stSel = (stSel + 1) % N; sfx(SFX_MOVE); }
  if (in.bP) { gotoMenu = true; return; }
  bool act = in.aP; int8_t ch = in.px;
  switch (stSel) {
    case 0: if (act || ch) { cfg.sound = !cfg.sound; cfgSave(); if (!cfg.sound) audioStop(); else sfx(SFX_SELECT); } break;
    case 1: if (act || ch) { cfg.led = (cfg.led + (ch < 0 ? 3 : 1)) % 4; cfgSave(); ledReset(); ledFlash(C_WHITE, 400); sfx(SFX_MOVE); } break;
    case 2: if (act || ch) {
      cfg.buzzActive = !cfg.buzzActive; cfgSave();
      u8g2.clearBuffer(); u8g2.setFont(FONT_M); textC("RESTARTING...", 34); u8g2.sendBuffer(); delay(700); ESP.restart();
    } break;
    case 3: if (act || ch) {
      if (!cfg.sound) toast("SOUND IS OFF");
      else { static const uint16_t fr[4] = {1200, 2000, 2700, 3500}; stTest = (stTest + 1) & 3; sfxTone(fr[stTest], 500); }
    } break;
    case 4: if (act) { inputCalibrate(); toast("STICK CALIBRATED"); sfx(SFX_SELECT); } break;
    case 5: if (in.a) { if (++stHold >= 30) { for (uint8_t i = 0; i < 16; i++) hiSet(i, 0); stHold = 0; toast("SCORES CLEARED"); sfx(SFX_OVER); } } else stHold = 0; break;
  }
}
static void stDraw() {
  u8g2.setFont(FONT_S); u8g2.setDrawColor(1); u8g2.drawBox(0, 0, 128, 9); u8g2.setDrawColor(0);
  u8g2.drawStr(2, 7, "SETTINGS"); u8g2.drawStr(128 - 6 * 5 - 2, 7, "B:back"); u8g2.setDrawColor(1);
  static const char* const lab[6] = {"Sound", "LED brightness", "Buzzer type", "Sound test", "Calibrate stick", "Reset scores"};
  char v[12];
  for (uint8_t i = 0; i < 6; i++) {
    int y = 17 + i * 9; bool on = (i == stSel);
    if (on) u8g2.drawBox(0, y - 7, 128, 9);
    u8g2.setDrawColor(on ? 0 : 1); u8g2.drawStr(4, y, lab[i]);
    v[0] = 0;
    switch (i) {
      case 0: strcpy(v, cfg.sound ? "ON" : "OFF"); break;
      case 1: strcpy(v, LED_NAMES[cfg.led]); break;
      case 2: strcpy(v, cfg.buzzActive ? "ACTIVE" : "PASSIVE"); break;
      case 3: strcpy(v, "A:play"); break;
      case 4: strcpy(v, "A:do"); break;
      default: strcpy(v, stHold ? "..." : "hold A");
    }
    u8g2.drawStr(124 - u8g2.getStrWidth(v), y, v);
    if (i == 5 && stHold) u8g2.drawBox(4, y + 1, stHold * 4, 1);
  }
  u8g2.setDrawColor(1);
}
static void stIcon(int x, int y) {
  u8g2.drawCircle(x + 16, y + 16, 8); u8g2.drawDisc(x + 16, y + 16, 3);
  for (int i = 0; i < 8; i++) { float a = i * 0.7854f; u8g2.drawBox(x + 15 + (int)(cosf(a) * 12), y + 15 + (int)(sinf(a) * 12), 3, 3); }
}

// ---------------------------------------------------------------- Hardware test (exit with stick click)
static void tsInit() {}
static void tsUpdate() {
  if (in.aP) sfxTone(2700, 300);
  if (in.bP) sfxTone(1200, 300);
  ledFlash((uint8_t)constrain(128 + joyRx / 8, 0, 255), (uint8_t)constrain(128 + joyRy / 8, 0, 255), in.touch ? 255 : 0, 60);
}
static void tsDraw() {
  char b[28]; u8g2.setFont(FONT_S); textC("HW TEST    (SW = exit)", 7);
  snprintf(b, sizeof b, "X%4d Y%4d", joyRawX, joyRawY); u8g2.drawStr(2, 20, b);
  snprintf(b, sizeof b, "ax%4d ay%4d", in.ax, in.ay); u8g2.drawStr(2, 29, b);
  snprintf(b, sizeof b, "A:%d  B:%d", in.a, in.b); u8g2.drawStr(2, 41, b);
  snprintf(b, sizeof b, "SW:%d  TOUCH:%d", in.sw, in.touch); u8g2.drawStr(2, 50, b);
  u8g2.drawStr(2, 61, "A=2.7kHz  B=1.2kHz");
  u8g2.drawFrame(92, 14, 32, 32);
  int px = 108 + in.ax * 14 / 100, py = 30 + in.ay * 14 / 100; u8g2.drawBox(px - 1, py - 1, 3, 3);
}
static void tsIcon(int x, int y) {
  u8g2.drawBox(x + 4, y + 22, 24, 6); u8g2.drawBox(x + 14, y + 10, 4, 12); u8g2.drawDisc(x + 16, y + 8, 5);
  u8g2.drawFrame(x + 2, y + 2, 5, 5); u8g2.drawFrame(x + 25, y + 2, 5, 5);
}
