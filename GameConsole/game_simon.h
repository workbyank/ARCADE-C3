#pragma once
// Simon - watch the LED + tone pattern, repeat it with the stick (up/right/down/left). Pattern grows every round.
#define SM_ID 9
static uint8_t smSeq[40], smLen, smPos, smSt, smLit; static uint32_t smT, smIn, smLitT; static uint16_t smScore, smHi;
static const uint16_t smF[4] = {3200, 2700, 2200, 1800};
static const uint8_t smC[4][3] = {{0,255,0},{255,0,0},{0,80,255},{255,190,0}};

static void smPress(uint8_t d, uint16_t ms) {
  smLit = d; smLitT = millis() + ms; sfxTone(smF[d], ms); ledFlash(smC[d][0], smC[d][1], smC[d][2], ms + 30);
}
static void smInit() {
  smLen = 1; smSeq[0] = random(4); smPos = 0; smSt = 0; smT = millis() + 700; smScore = 0; smLit = 255; smHi = hiGet(SM_ID);
}
static void smOver(uint8_t correct) {
  smSt = 5; smLit = correct; smLitT = millis() + 700; endRun(SM_ID, smScore, smHi);
}
static void smUpdate() {
  if (smSt == 5) { if (smLit != 255 && (int32_t)(millis() - smLitT) >= 0) smLit = 255; if (overKeys()) smInit(); return; }
  uint32_t now = millis();
  if (smLit != 255 && (int32_t)(now - smLitT) >= 0) smLit = 255;
  switch (smSt) {
    case 0:
      if ((int32_t)(now - smT) >= 0) {
        if (smPos < smLen) { uint16_t on = imax(200, 520 - 18 * smLen); smPress(smSeq[smPos++], on); smT = now + on + 130; }
        else { smSt = 3; smPos = 0; smIn = now + 6000; }
      }
      break;
    case 3: {
      int8_t d = -1; if (in.py < 0) d = 0; else if (in.px > 0) d = 1; else if (in.py > 0) d = 2; else if (in.px < 0) d = 3;
      if (d >= 0) {
        smPress(d, 160); smIn = now + 6000;
        if (d == smSeq[smPos]) { smPos++; if (smPos == smLen) { smScore = smLen; smSt = 4; smT = now + 800; } }
        else smOver(smSeq[smPos]);
      } else if ((int32_t)(now - smIn) >= 0) smOver(smSeq[smPos]);
      break;
    }
    case 4:
      if ((int32_t)(now - smT) >= 0) {
        if (smLen >= 40) { smOver(255); break; }
        fx(SFX_LEVEL, C_CYAN, 150); smSeq[smLen++] = random(4); smPos = 0; smSt = 0; smT = now + 500;
      }
      break;
  }
}
static void smPad(uint8_t d, int x, int y, int w, int h) {
  bool lit = (smLit == d);
  if (lit) u8g2.drawBox(x, y, w, h); else u8g2.drawFrame(x, y, w, h);
  u8g2.setDrawColor(lit ? 0 : 1); int cx = x + w / 2, cy = y + h / 2;
  switch (d) {
    case 0: u8g2.drawTriangle(cx, cy - 3, cx - 4, cy + 3, cx + 4, cy + 3); break;
    case 1: u8g2.drawTriangle(cx + 3, cy, cx - 3, cy - 4, cx - 3, cy + 4); break;
    case 2: u8g2.drawTriangle(cx, cy + 3, cx - 4, cy - 3, cx + 4, cy - 3); break;
    default: u8g2.drawTriangle(cx - 3, cy, cx + 3, cy - 4, cx + 3, cy + 4);
  }
  u8g2.setDrawColor(1);
}
static void smDraw() {
  char b[28]; u8g2.setFont(FONT_S); snprintf(b, sizeof b, "ROUND %u   BEST %u", smScore, smHi); textC(b, 7);
  smPad(0, 49, 10, 30, 14); smPad(2, 49, 50, 30, 13); smPad(3, 12, 26, 30, 16); smPad(1, 86, 26, 30, 16);
  u8g2.setFont(FONT_M);
  if (smSt == 0) textC("WATCH", 38); else if (smSt == 3) textC("GO!", 38); else if (smSt == 4) textC("NICE!", 38);
  if (smSt == 5) overBox(smScore, smHi);
}
static void smIcon(int x, int y) {
  u8g2.drawBox(x + 11, y + 1, 10, 9); u8g2.drawFrame(x + 11, y + 22, 10, 9); u8g2.drawFrame(x + 1, y + 11, 9, 10); u8g2.drawFrame(x + 22, y + 11, 9, 10);
}
