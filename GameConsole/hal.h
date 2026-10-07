#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <Preferences.h>
#include "config.h"

#ifndef ESP_ARDUINO_VERSION_MAJOR
#define ESP_ARDUINO_VERSION_MAJOR 2
#endif

#define FONT_S u8g2_font_5x7_tf
#define FONT_M u8g2_font_6x10_tf
// colour shortcuts: fx(SFX_COIN, C_GOLD, 80)
#define C_GOOD   0,255,0
#define C_BAD    255,0,0
#define C_GOLD   255,190,0
#define C_CYAN   0,200,255
#define C_WHITE  255,255,255
#define C_PURPLE 170,0,255
#define C_ORANGE 255,100,0

static inline int imin(int a, int b) { return a < b ? a : b; }
static inline int imax(int a, int b) { return a > b ? a : b; }
static inline bool hitBox(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {
  return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

static U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
static bool gotoMenu = false;   // a game sets this to go back to the launcher

static void displayInit() {
  Wire.setPins(PIN_SDA, PIN_SCL);
  u8g2.begin();
  u8g2.setBusClock(I2C_HZ);
  u8g2.setFont(FONT_M);
}
static void textC(const char* s, int y) { u8g2.drawStr((128 - u8g2.getStrWidth(s)) / 2, y, s); }

// ================================================================ settings (saved in flash)
struct Cfg { bool sound; uint8_t led; bool buzzActive; };
static Cfg cfg = {true, 2, false};
static const uint8_t LED_LEVELS[4] = {0, 12, 40, 110};
static void cfgLoad() {
  Preferences p; p.begin("cfg", false);
  cfg.sound = p.getUChar("snd", 1); cfg.led = p.getUChar("led", 2); cfg.buzzActive = p.getUChar("bz", 0);
  p.end(); if (cfg.led > 3) cfg.led = 2;
}
static void cfgSave() {
  Preferences p; p.begin("cfg", false);
  p.putUChar("snd", cfg.sound); p.putUChar("led", cfg.led); p.putUChar("bz", cfg.buzzActive);
  p.end();
}

// ================================================================ input
struct Input {
  int8_t dx, dy;        // held: -1/0/+1 (dy -1 = up) - hysteresis + dominant-axis filtered
  int8_t px, py;        // first press of a direction
  int8_t rx, ry;        // press + auto-repeat (menus, tetris)
  int8_t ax, ay;        // analog, dead-zoned + curved + smoothed: -100..+100 (use for cursors / paddles / ships)
  bool a, b, sw, touch;
  bool aP, bP, swP, touchP;
};
static Input in;
// joyC* = centre (calibrated, slowly self-corrects), joyF* = smoothed reading <<4, joyR* = filtered offset from centre
static int joyCx = 2048, joyCy = 2048, joyRx = 0, joyRy = 0, joyRawX = 0, joyRawY = 0;
static int joyFx = 2048 << 4, joyFy = 2048 << 4;

static int joyRead(uint8_t pin) { return (analogRead(pin) + analogRead(pin) + 1) >> 1; }   // 2-sample average
static void inputCalibrate() {
  int cx = 2048, cy = 2048;
  for (uint8_t tryN = 0; tryN < 3; tryN++) {      // retry if the stick was moving while sampling
    long sx = 0, sy = 0; int x0 = 4095, x1 = 0, y0 = 4095, y1 = 0;
    for (int i = 0; i < 40; i++) {
      int x = joyRead(PIN_JOY_X), y = joyRead(PIN_JOY_Y); sx += x; sy += y;
      x0 = imin(x0, x); x1 = imax(x1, x); y0 = imin(y0, y); y1 = imax(y1, y); delay(2);
    }
    cx = sx / 40; cy = sy / 40;
    if (x1 - x0 < 120 && y1 - y0 < 120) break;
  }
  if (cx < 1200 || cx > 2900) cx = 2048;           // stick was held at boot -> fall back to nominal centre
  if (cy < 1200 || cy > 2900) cy = 2048;
  joyCx = cx; joyCy = cy; joyFx = cx << 4; joyFy = cy << 4; joyRx = joyRy = 0;
}
static void inputInit() {
  pinMode(PIN_BTN_A, INPUT_PULLUP); pinMode(PIN_BTN_B, INPUT_PULLUP);
  pinMode(PIN_JOY_SW, INPUT_PULLUP); pinMode(PIN_TOUCH, INPUT);
  inputCalibrate();
}
static void repeatAxis(int8_t d, int8_t &prev, uint32_t &next, int8_t &out) {
  out = 0; uint32_t now = millis();
  if (d == 0) { prev = 0; return; }
  if (d != prev) { prev = d; out = d; next = now + 220; }
  else if ((int32_t)(now - next) >= 0) { out = d; next = now + 90; }
}
// offset from centre (counts) -> -100..100: dead zone, then 35% linear + 65% squared (fine control near centre, full speed at the edge)
static int8_t joyShape(int r) {
  int a = r < 0 ? -r : r;
  if (a <= JOY_DEAD) return 0;
  int span = JOY_RANGE - JOY_DEAD; a -= JOY_DEAD; if (a > span) a = span;
  int n = a * 100 / span, o = (n * 35 + n * n * 65 / 100) / 100;
  if (o < 1) o = 1;
  return r < 0 ? -o : o;
}
static void joyTrack(int raw, int &f, int &c, int &r, uint8_t &idle, bool slow) {   // smooth + self-centre one axis
  f += ((raw << 4) - f) * JOY_FILT / 8;
  int v = (f + 8) >> 4; r = v - c;
  if (r > -(JOY_DEAD * 2 / 3) && r < JOY_DEAD * 2 / 3) {      // resting: creep the centre toward the reading (drift fix)
    if (idle < 60) idle++; else if (slow) c += (v > c) - (v < c);
  } else idle = 0;
  r = v - c;
}
static void inputUpdate() {
  static bool pa = false, pb = false, ps = false, tHeld = false, tFired = false;
  static int8_t pdx = 0, pdy = 0, rpx = 0, rpy = 0;
  static uint32_t nx = 0, ny = 0, tStart = 0; static uint8_t idX = 0, idY = 0, tick = 0;
  joyRawX = joyRead(PIN_JOY_X); joyRawY = joyRead(PIN_JOY_Y); tick++;
  int rx, ry; bool slow = (tick & 3) == 0;
  joyTrack(joyRawX, joyFx, joyCx, rx, idX, slow); joyTrack(joyRawY, joyFy, joyCy, ry, idY, slow);
#if JOY_SWAP_XY
  { int t = rx; rx = ry; ry = t; }
#endif
#if JOY_INVERT_X
  rx = -rx;
#endif
#if JOY_INVERT_Y
  ry = -ry;
#endif
  joyRx = rx; joyRy = ry;
  in.ax = joyShape(rx); in.ay = joyShape(ry);
  int ax = rx < 0 ? -rx : rx, ay = ry < 0 ? -ry : ry;
  int tx = pdx ? JOY_THRESH - JOY_HYST : JOY_THRESH, ty = pdy ? JOY_THRESH - JOY_HYST : JOY_THRESH;
  bool onX = ax > tx, onY = ay > ty;
  if (onX && onY) { if (ax * 2 < ay * 3 && ay * 2 < ax * 3) {} else if (ax > ay) onY = false; else onX = false; }   // diagonal only when near 45 deg
  in.dx = onX ? (rx > 0 ? 1 : -1) : 0;
  in.dy = onY ? (ry > 0 ? 1 : -1) : 0;
  in.px = (in.dx != 0 && in.dx != pdx) ? in.dx : 0; pdx = in.dx;
  in.py = (in.dy != 0 && in.dy != pdy) ? in.dy : 0; pdy = in.dy;
  repeatAxis(in.dx, rpx, nx, in.rx); repeatAxis(in.dy, rpy, ny, in.ry);
#if BTN_SWAP_AB
  in.a  = !digitalRead(PIN_BTN_B);  in.aP  = in.a  && !pa; pa = in.a;
  in.b  = !digitalRead(PIN_BTN_A);  in.bP  = in.b  && !pb; pb = in.b;
#else
  in.a  = !digitalRead(PIN_BTN_A);  in.aP  = in.a  && !pa; pa = in.a;
  in.b  = !digitalRead(PIN_BTN_B);  in.bP  = in.b  && !pb; pb = in.b;
#endif
  in.sw = !digitalRead(PIN_JOY_SW); in.swP = in.sw && !ps; ps = in.sw;
  in.touch = digitalRead(PIN_TOUCH); in.touchP = false;      // touch must be held ~120 ms (ignores accidental brushes)
  if (in.touch) {
    if (!tHeld) { tHeld = true; tStart = millis(); }
    if (!tFired && millis() - tStart >= 120) { tFired = true; in.touchP = true; }
  } else { tHeld = false; tFired = false; }
}

// ================================================================ sound engine
// Tones are kept in the 1.5-4.2 kHz range: small buzzers are LOUD there and very quiet below ~1 kHz.
enum Sfx : uint8_t { SFX_NONE, SFX_MOVE, SFX_SELECT, SFX_BACK, SFX_JUMP, SFX_COIN, SFX_HIT, SFX_SHOOT,
                     SFX_BOOM, SFX_POWER, SFX_LEVEL, SFX_OVER, SFX_BEST, SFX_START, SFX_LINE, SFX_DUCK, SFX_BOOT, SFX_COUNT };
struct Note { uint16_t f, ms; };   // {0,0} ends a sequence
static const Note S_MOVE[]   = {{2600,10},{0,0}};
static const Note S_SELECT[] = {{2200,35},{3200,55},{0,0}};
static const Note S_BACK[]   = {{3000,35},{2000,55},{0,0}};
static const Note S_JUMP[]   = {{1800,22},{2600,32},{0,0}};
static const Note S_COIN[]   = {{3300,30},{4200,80},{0,0}};
static const Note S_HIT[]    = {{2200,40},{1400,60},{900,110},{0,0}};
static const Note S_SHOOT[]  = {{3900,12},{3000,12},{0,0}};
static const Note S_BOOM[]   = {{1500,35},{900,45},{500,70},{300,120},{0,0}};
static const Note S_POWER[]  = {{2000,40},{2600,40},{3200,40},{3900,90},{0,0}};
static const Note S_LEVEL[]  = {{2200,70},{2800,70},{3400,70},{4200,150},{0,0}};
static const Note S_OVER[]   = {{2600,130},{2100,130},{1700,130},{1100,320},{0,0}};
static const Note S_BEST[]   = {{2500,90},{3100,90},{3700,90},{3100,70},{4200,250},{0,0}};
static const Note S_START[]  = {{2200,60},{3000,60},{4000,100},{0,0}};
static const Note S_LINE[]   = {{2800,50},{3400,50},{4000,110},{0,0}};
static const Note S_DUCK[]   = {{1500,20},{1100,25},{0,0}};
static const Note S_BOOT[]   = {{1700,70},{2300,70},{2900,70},{0,40},{3400,60},{4200,220},{0,0}};
static const Note* const SFX_TAB[] = {nullptr, S_MOVE, S_SELECT, S_BACK, S_JUMP, S_COIN, S_HIT, S_SHOOT,
                                      S_BOOM, S_POWER, S_LEVEL, S_OVER, S_BEST, S_START, S_LINE, S_DUCK, S_BOOT};
static const uint8_t SFX_PRI[] = {0, 1, 2, 2, 1, 2, 2, 0, 2, 2, 3, 3, 3, 2, 2, 1, 3};
static_assert(sizeof(SFX_TAB) / sizeof(SFX_TAB[0]) == SFX_COUNT, "sfx table size");

static const Note* seq = nullptr; static uint32_t seqNext = 0; static uint8_t seqPri = 0;
static Note dynNote[2];

static void toneWrite(uint16_t f) {
  if (cfg.buzzActive) { digitalWrite(PIN_BUZZER, f ? HIGH : LOW); return; }
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWriteTone(PIN_BUZZER, f);
#else
  ledcWriteTone(0, f);
#endif
}
static void audioInit() {
  if (cfg.buzzActive) { pinMode(PIN_BUZZER, OUTPUT); digitalWrite(PIN_BUZZER, LOW); }
  else {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(PIN_BUZZER, 2700, 8);
#else
    ledcSetup(0, 2700, 8); ledcAttachPin(PIN_BUZZER, 0);
#endif
  }
  toneWrite(0);
}
static void audioStop() { seq = nullptr; seqPri = 0; toneWrite(0); }
static void sfxPlay(const Note* s, uint8_t pri) {
  if (!cfg.sound || !s) return;
  if (seq && pri < seqPri) return;
  seq = s; seqPri = pri; toneWrite(s->f); seqNext = millis() + s->ms;
}
static void sfx(Sfx id) { sfxPlay(SFX_TAB[id], SFX_PRI[id]); }
static void sfxTone(uint16_t f, uint16_t ms) { dynNote[0] = {f, ms}; dynNote[1] = {0, 0}; sfxPlay(dynNote, 2); }
static void audioUpdate() {
  if (!seq || (int32_t)(millis() - seqNext) < 0) return;
  seq++;
  if (seq->ms == 0) { seq = nullptr; seqPri = 0; toneWrite(0); }
  else { toneWrite(seq->f); seqNext += seq->ms; }
}

// ================================================================ LED (WS2812) - synced with sound via fx()
static uint8_t ledBase[3] = {0, 0, 0};
static int16_t ledCur[3] = {-1, -1, -1};
static uint32_t ledFlashEnd = 0;
static void ledApply(uint8_t r, uint8_t g, uint8_t b) {
  if (ledCur[0] == r && ledCur[1] == g && ledCur[2] == b) return;
  ledCur[0] = r; ledCur[1] = g; ledCur[2] = b;
  uint16_t k = LED_LEVELS[cfg.led];
  neopixelWrite(PIN_LED, (uint8_t)((uint16_t)r * k / 255), (uint8_t)((uint16_t)g * k / 255), (uint8_t)((uint16_t)b * k / 255));
}
static void ledReset() { ledCur[0] = ledCur[1] = ledCur[2] = -1; ledApply(ledBase[0], ledBase[1], ledBase[2]); }
static void ledSetBase(uint8_t r, uint8_t g, uint8_t b) { ledBase[0] = r; ledBase[1] = g; ledBase[2] = b; if (!ledFlashEnd) ledApply(r, g, b); }
static void ledFlash(uint8_t r, uint8_t g, uint8_t b, uint16_t ms) { ledApply(r, g, b); ledFlashEnd = millis() + ms; }
static void ledUpdate() {
  if (ledFlashEnd && (int32_t)(millis() - ledFlashEnd) >= 0) { ledFlashEnd = 0; ledApply(ledBase[0], ledBase[1], ledBase[2]); }
}
// one call = sound + LED flash together
static void fx(Sfx s, uint8_t r, uint8_t g, uint8_t b, uint16_t ms = 120) { sfx(s); ledFlash(r, g, b, ms); }

// ================================================================ high scores
static uint16_t hiGet(uint8_t id) {
  char k[8]; snprintf(k, sizeof k, "g%u", id);
  Preferences p; p.begin("hs", false); uint16_t v = p.getUShort(k, 0); p.end(); return v;
}
static void hiSet(uint8_t id, uint16_t v) {
  char k[8]; snprintf(k, sizeof k, "g%u", id);
  Preferences p; p.begin("hs", false); p.putUShort(k, v); p.end();
}

// ================================================================ UI helpers
static char toastMsg[20]; static uint32_t toastUntil = 0;
static char bannerMsg[16]; static uint32_t bannerUntil = 0;
static void toast(const char* m) { strncpy(toastMsg, m, 19); toastMsg[19] = 0; toastUntil = millis() + 1300; }
static void banner(const char* m) { strncpy(bannerMsg, m, 15); bannerMsg[15] = 0; bannerUntil = millis() + 1300; }
static void bannerLevel(uint16_t lv) { char b[16]; snprintf(b, sizeof b, "LEVEL %u", lv); banner(b); }
static void toastDraw() {
  if (!toastUntil) return;
  if ((int32_t)(millis() - toastUntil) >= 0) { toastUntil = 0; return; }
  u8g2.setFont(FONT_S); int w = u8g2.getStrWidth(toastMsg) + 8, x = (128 - w) / 2;
  u8g2.setDrawColor(0); u8g2.drawBox(x, 52, w, 11); u8g2.setDrawColor(1);
  u8g2.drawFrame(x, 52, w, 11); u8g2.drawStr(x + 4, 60, toastMsg);
}
static void bannerDraw() {
  if (!bannerUntil) return;
  if ((int32_t)(millis() - bannerUntil) >= 0) { bannerUntil = 0; return; }
  u8g2.setFont(FONT_M); int w = u8g2.getStrWidth(bannerMsg) + 14, x = (128 - w) / 2;
  u8g2.setDrawColor(0); u8g2.drawBox(x, 22, w, 18); u8g2.setDrawColor(1);
  u8g2.drawFrame(x, 22, w, 18); textC(bannerMsg, 35);
}
static void drawSpeaker(int x, int y, bool on) {   // 9x7 icon in the current draw colour
  u8g2.drawBox(x, y + 2, 2, 3); u8g2.drawBox(x + 2, y + 1, 1, 5); u8g2.drawBox(x + 3, y, 1, 7);
  if (on) { u8g2.drawPixel(x + 5, y + 2); u8g2.drawPixel(x + 5, y + 4); u8g2.drawPixel(x + 6, y + 1); u8g2.drawPixel(x + 6, y + 5); }
  else { for (int i = 0; i < 4; i++) { u8g2.drawPixel(x + 5 + i, y + 1 + i); u8g2.drawPixel(x + 5 + i, y + 5 - i); } }
}
static void overlay(const char* l1, const char* l2) {
  u8g2.setDrawColor(0); u8g2.drawBox(14, 16, 100, 32);
  u8g2.setDrawColor(1); u8g2.drawFrame(14, 16, 100, 32);
  u8g2.setFont(FONT_M); textC(l1, 31); u8g2.setFont(FONT_S); textC(l2, 42);
}

// ---- common "run finished" handling
static uint32_t overAt = 0; static bool newBest = false;
static void endRun(uint8_t id, uint16_t score, uint16_t &hi) {
  newBest = score > hi; if (newBest) { hi = score; hiSet(id, score); }
  overAt = millis();
  if (newBest) fx(SFX_BEST, C_GOLD, 700); else fx(SFX_OVER, C_BAD, 500);
}
static bool overKeys() {                 // true = restart. B goes back to the menu.
  if (millis() - overAt < 500) return false;
  if (in.bP) { gotoMenu = true; return false; }
  return in.aP;
}
static void overBox(uint16_t score, uint16_t hi) {
  u8g2.setDrawColor(0); u8g2.drawBox(10, 8, 108, 48);
  u8g2.setDrawColor(1); u8g2.drawFrame(10, 8, 108, 48);
  char b[28]; u8g2.setFont(FONT_M); textC("GAME OVER", 20);
  u8g2.setFont(FONT_S); snprintf(b, sizeof b, "SCORE %u   BEST %u", score, hi); textC(b, 30);
  if (newBest && ((millis() / 300) & 1)) textC("* NEW BEST! *", 39);
  textC("A:again   B:menu", 51);
}
