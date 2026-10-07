#pragma once
// Cricket - batting chase. Bowler runs in and bowls; press A (drive) or B (loft) as the ball reaches the bat.
// Stick aims the shot: match the ball's line (off / straight / leg). Timing + line + loft decide 0,1,2,3,4,6 or a wicket.
// 2 overs, 3 wickets, beat the target to win. Score = runs.
#define CR_ID    10
#define CR_BALLS 12
#define CR_WKTS  3
#define CR_HITY  47      // ball y where the bat meets it
#define CR_STMY  55      // ball y where it reaches the stumps
enum : uint8_t { CRS_RUN, CRS_BALL, CRS_FLY, CRS_RES, CRS_OVER };
static const int8_t crFld[6][2] = {{46,20},{82,20},{40,36},{88,36},{47,54},{81,54}};
static uint8_t crSt, crBalls, crWk, crKind, crOutK, crSN; static uint16_t crRuns, crHi, crT, crTgt;
static int8_t crLine, crSx, crSwDir, crSwF; static bool crSwung, crLoft, crWin; static char crStrip[7];
static float crBy, crSp, crFx, crFy, crVx, crVy; static uint8_t crFl, crFmax; static int8_t crPend;   // crPend: runs from the shot (-1 = wicket)

static void crDeliver() {
  crSt = CRS_RUN; crT = 0; crSwF = -1; crSwung = false; crLine = random(-1, 2); crSx = random(-3, 4);
  crKind = random(3); crSp = crKind == 0 ? 1.0f : (crKind == 1 ? 1.5f : 2.2f); crBy = 17; crOutK = 0;
}
static void crInit() {
  crRuns = 0; crBalls = 0; crWk = 0; crSN = 0; crWin = false; crHi = hiGet(CR_ID); crTgt = 20 + random(0, 13);
  memset(crStrip, 0, sizeof crStrip); crDeliver();
}
static void crRecord(char c) { if (crSN >= 6) { crSN = 0; memset(crStrip, 0, sizeof crStrip); } crStrip[crSN++] = c; }
static void crResolve(int8_t runs, uint8_t outKind, const char* msg) {   // runs<0 = no scoring ball; outKind 1 caught 2 bowled
  crBalls++; char c = '.';
  if (outKind) { crWk++; c = 'W'; fx(SFX_HIT, C_BAD, 300); }
  else if (runs >= 6) { c = '6'; fx(SFX_LEVEL, C_GOLD, 300); }
  else if (runs == 4) { c = '4'; fx(SFX_LINE, C_CYAN, 200); }
  else if (runs > 0) { c = '0' + runs; fx(SFX_COIN, C_GOOD, 100); }
  else sfx(SFX_MOVE);
  if (runs > 0) crRuns += runs;
  crRecord(c); banner(msg); crSt = CRS_RES; crT = 0; crOutK = outKind;
  if (crRuns >= crTgt) crWin = true;
}
static void crHitBall(uint8_t s, bool loft) {   // s: 0 perfect .. 3 edge. Outcome table, a bit of luck for variety
  uint8_t r = random(10); int8_t runs = 0; bool out = false;
  if (!loft) switch (s) {
    case 0: runs = r < 6 ? 4 : (r < 8 ? 3 : 2); break;
    case 1: runs = r < 5 ? 2 : (r < 8 ? 1 : 3); break;
    case 2: runs = r < 3 ? 0 : 1; break;
    default: if (r < 3) out = true; else runs = 1;
  } else switch (s) {
    case 0: runs = r < 7 ? 6 : 4; break;
    case 1: if (r < 4) runs = 6; else if (r < 8) runs = 4; else out = true; break;
    case 2: if (r < 5) out = true; else runs = 2; break;
    default: if (r < 7) out = true; else runs = 4;
  }
  crPend = out ? -1 : runs; crSt = CRS_FLY; crFx = 64; crFy = CR_HITY; crFl = 0; crOutK = out ? 1 : 0;
  if (out) {   // fly to a fielder on the chosen side (straight = back to the bowler)
    int tx = crSwDir < 0 ? 40 : (crSwDir > 0 ? 88 : 64), ty = crSwDir ? 36 : 16; crFmax = 14;
    crVx = (tx - 64) / 14.0f; crVy = (ty - CR_HITY) / 14.0f;
  } else {
    crVx = crSwDir * 2.0f; crVy = crSwDir ? -1.2f : -2.4f; crFmax = runs >= 4 ? 40 : 6 + 3 * runs;
    if (runs == 0) crFmax = 4;
  }
  fx(SFX_JUMP, C_WHITE, 60);
}
static void crSwing() {   // A = drive, B = loft; evaluated at the moment of the press
  crSwung = true; crSwF = 0; crLoft = in.bP; crSwDir = in.dx;
  float t = (crBy - CR_HITY) / crSp; if (t < 0) t = -t;     // timing error in frames
  if (t > 5.0f) { sfx(SFX_DUCK); return; }                   // whiff: ball carries on
  uint8_t s = t <= 1.6f ? 0 : (t <= 3.2f ? 1 : 2);
  int d = crSwDir - crLine; if (d < 0) d = -d; s += d; if (s > 3) s = 3;   // playing across the line costs quality
  crHitBall(s, crLoft);
}
static void crUpdate() {
  if (crSt == CRS_OVER) { if (overKeys()) crInit(); return; }
  crT++;
  if (crSwF >= 0 && crSwF < 8) { if (crT & 1) crSwF++; }
  switch (crSt) {
    case CRS_RUN: if (crT >= 24) { crSt = CRS_BALL; crT = 0; sfx(SFX_DUCK); } break;
    case CRS_BALL:
      crBy += crSp;
      if (!crSwung && (in.aP || in.bP)) crSwing();
      if (crSt == CRS_BALL && crBy >= CR_STMY) {
        if (crLine == 0) crResolve(0, 2, "BOWLED!"); else crResolve(0, 0, "DOT BALL");
      }
      break;
    case CRS_FLY: {
      crFx += crVx; crFy += crVy; crFl++;
      int dx = (int)crFx - 64, dy = (int)crFy - 36; bool rope = dx * dx + dy * dy >= 28 * 28;
      bool done = crFl >= crFmax || (crPend >= 4 && rope);
      if (done) {
        if (crPend < 0) crResolve(0, 1, "CAUGHT!");
        else if (crPend == 0) crResolve(0, 0, "DOT BALL");
        else if (crPend == 4) crResolve(4, 0, "FOUR!");
        else if (crPend == 6) crResolve(6, 0, "SIX!");
        else { char b[12]; snprintf(b, sizeof b, crPend == 1 ? "1 RUN" : "%d RUNS", crPend); crResolve(crPend, 0, b); }
      }
    } break;
    case CRS_RES:
      if (crT >= 30) {
        if (crWin || crWk >= CR_WKTS || crBalls >= CR_BALLS) {
          crSt = CRS_OVER; endRun(CR_ID, crRuns, crHi); if (crWin && !newBest) fx(SFX_LEVEL, C_GOLD, 600);
        } else { if (crBalls % 6 == 0) toast("END OF OVER"); crDeliver(); }
      }
      break;
  }
}
static void crFigure(int x, int y, bool arm) {   // tiny person: head, body, optional raised arm
  u8g2.drawDisc(x, y - 3, 1); u8g2.drawBox(x - 1, y - 1, 3, 4); if (arm) u8g2.drawLine(x + 1, y - 1, x + 3, y - 5);
}
static void crDraw() {
  char b[16]; u8g2.setFont(FONT_S);
  // HUD: left = runs / wickets / overs, right = target / need
  u8g2.drawStr(2, 7, "RUNS"); u8g2.drawStr(2, 29, "WKT"); u8g2.drawStr(2, 49, "OVER");
  snprintf(b, sizeof b, "%u", crRuns); u8g2.setFont(FONT_M); u8g2.drawStr(2, 19, b);
  snprintf(b, sizeof b, "%u/%d", crWk, CR_WKTS); u8g2.setFont(FONT_S); u8g2.drawStr(2, 38, b);
  snprintf(b, sizeof b, "%u.%u", crBalls / 6, crBalls % 6); u8g2.setFont(FONT_M); u8g2.drawStr(2, 58, b);
  u8g2.setFont(FONT_S); u8g2.drawStr(94, 7, "TARGET"); u8g2.drawStr(94, 29, "NEED");
  snprintf(b, sizeof b, "%u", crTgt); u8g2.setFont(FONT_M); u8g2.drawStr(94, 19, b);
  snprintf(b, sizeof b, "%d", crTgt > crRuns ? crTgt - crRuns : 0); u8g2.drawStr(94, 40, b);
  u8g2.setFont(FONT_S);
  if (crSt <= CRS_BALL) u8g2.drawStr(94, 56, crKind == 0 ? "SLOW" : (crKind == 1 ? "MED" : "FAST"));
  for (uint8_t i = 0; i < crSN; i++) { char c[2] = {crStrip[i], 0}; u8g2.drawStr(94 + i * 6, 63, c); }
  // ground, pitch, stumps, fielders
  u8g2.drawCircle(64, 36, 27); u8g2.drawFrame(58, 13, 13, 46); u8g2.drawHLine(58, 18, 13); u8g2.drawHLine(58, 50, 13);
  for (uint8_t i = 0; i < 6; i++) u8g2.drawBox(crFld[i][0], crFld[i][1], 2, 2);
  u8g2.drawPixel(56, CR_HITY); u8g2.drawPixel(72, CR_HITY);               // timing marks
  for (int8_t i = -1; i <= 1; i++) {
    u8g2.drawVLine(64 + i * 2, 14, 3);
    if (crOutK == 2 && crSt == CRS_RES) u8g2.drawVLine(64 + i * 2 + i * (crT / 3), 54 - crT / 6, 3);   // stumps scatter
    else u8g2.drawVLine(64 + i * 2, 54, 4);
  }
  u8g2.drawBox(63, 61, 3, 2);                                               // keeper
  // bowler: run-up then arm swing
  int by = 8 + (crSt == CRS_RUN ? crT / 4 : 6); crFigure(64, by + 4, crSt == CRS_RUN ? (crT & 4) : crSt == CRS_BALL && crBy < 24);
  // batter + bat (idle tip -> shot tip)
  crFigure(54, 50, false);
  int tx = 59, ty = 57;
  if (crSwF >= 0) {
    int fx1 = crSwDir < 0 ? 51 : (crSwDir > 0 ? 71 : 64), fy1 = crLoft ? 40 : (crSwDir ? 49 : 43); int f = crSwF > 4 ? 4 : crSwF;
    tx = 59 + (fx1 - 59) * f / 4; ty = 57 + (fy1 - 57) * f / 4;
  }
  u8g2.drawLine(57, 50, tx, ty); u8g2.drawLine(58, 51, tx + 1, ty);
  // ball
  if (crSt == CRS_BALL) {
    int bx = 64 + crSx + (crLine * 3 - crSx) * (int)(crBy - 17) / (CR_STMY - 17); int bs = crBy > 40 ? 3 : 2;
    u8g2.drawBox(bx, (int)crBy, bs, bs);
    if (crBy > 30) u8g2.drawPixel(bx, (int)crBy - 3);
  } else if (crSt == CRS_FLY) {
    int bs = (crPend == 6 && crFl > 3 && crFl < 24) ? 3 : 2; u8g2.drawBox((int)crFx, (int)crFy, bs, bs);
  }
  if (crSt == CRS_OVER) {
    u8g2.setDrawColor(0); u8g2.drawBox(10, 8, 108, 48); u8g2.setDrawColor(1); u8g2.drawFrame(10, 8, 108, 48);
    u8g2.setFont(FONT_M); textC(crWin ? "YOU WIN!" : "INNINGS OVER", 20); u8g2.setFont(FONT_S);
    snprintf(b, sizeof b, "%u/%u", crRuns, crWk); char l[40]; snprintf(l, sizeof l, "SCORE %s  TARGET %u", b, crTgt); textC(l, 30);
    snprintf(l, sizeof l, "BEST %u", crHi); if (newBest && ((millis() / 300) & 1)) textC("* NEW BEST! *", 39); else textC(l, 39);
    textC("A:again   B:menu", 51);
  }
}
static void crIcon(int x, int y) {
  for (int i = -1; i <= 1; i++) u8g2.drawLine(x + 6 + i, y + 29, x + 19 + i, y + 11);   // bat blade
  u8g2.drawLine(x + 19, y + 11, x + 24, y + 3); u8g2.drawLine(x + 20, y + 11, x + 25, y + 4);   // handle
  u8g2.drawDisc(x + 24, y + 24, 4); u8g2.setDrawColor(0); u8g2.drawPixel(x + 23, y + 22); u8g2.drawPixel(x + 24, y + 24); u8g2.drawPixel(x + 25, y + 26); u8g2.setDrawColor(1);
}
