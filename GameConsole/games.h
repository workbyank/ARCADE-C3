#pragma once
// ---- To add a game: write game_xxx.h (init/update/draw/icon, unique *_ID = its index below),
//      include it here and add ONE line to GAMES[]. The launcher, intro card, pause menu,
//      sound/LED effects and high scores all come for free.
#include "game_dino.h"
#include "game_shooter.h"
#include "game_tetris.h"
#include "game_racer.h"
#include "game_copter.h"
#include "game_snake.h"
#include "game_breakout.h"
#include "game_pong.h"
#include "game_flappy.h"
#include "game_simon.h"
#include "game_cricket.h"
#include "game_bounce.h"
#include "game_temple.h"
#include "sys_misc.h"

struct Game {
  const char* name;
  uint8_t r, g, b;                 // LED accent colour
  void (*init)(); void (*update)(); void (*draw)(); void (*icon)(int, int);
  const char* help1; const char* help2;   // shown on the intro card (max 17 chars each)
  uint8_t kind;                    // 0 = game (intro + pause menu), 1 = system screen
};
static const Game GAMES[] = {
  {"Dino Run",      0, 200, 255, dnInit, dnUpdate, dnDraw, dnIcon, "A:jump  B:duck",  "Stick up/dn too", 0},   // id 0
  {"Space Defender",0, 255, 120, sdInit, sdUpdate, sdDraw, sdIcon, "A:fire  B:bomb",  "Stick: fly",      0},   // id 1
  {"Tetris",      160,   0, 255, tInit,  tUpdate,  tDraw,  tIcon,  "A:rotate B:drop", "Stick:move/down", 0},   // id 2
  {"Road Racer",  255,  60,   0, rcInit, rcUpdate, rcDraw, rcIcon, "A:nitro B:brake", "Stick up/dn:lane",0},   // id 3
  {"Cave Copter", 255, 200,   0, cpInit, cpUpdate, cpDraw, cpIcon, "Hold A: climb",   "Dodge the walls", 0},   // id 4
  {"Snake",         0, 255,   0, snInit, snUpdate, snDraw, snIcon, "Stick: steer",    "Eat, dodge rocks",0},   // id 5
  {"Breakout",    255, 120,   0, boInit, boUpdate, boDraw, boIcon, "Stick: paddle",   "A: launch ball",  0},   // id 6
  {"Pong",        255, 255, 255, pgInit, pgUpdate, pgDraw, pgIcon, "Stick up/down",   "Beat the AI",     0},   // id 7
  {"Flappy",      255, 255,   0, fbInit, fbUpdate, fbDraw, fbIcon, "A: flap",         "Fly through gaps",0},   // id 8
  {"Simon",       255,   0, 150, smInit, smUpdate, smDraw, smIcon, "Stick: repeat",   "Watch, then copy",0},   // id 9
  {"Cricket",     60, 255,  60, crInit, crUpdate, crDraw, crIcon, "A:drive B:loft",  "Stick: aim shot", 0},   // id 10
  {"Bounce Ball", 0, 160, 255, bbInit, bbUpdate, bbDraw, bbIcon, "Stick: paddle",   "Keep it bouncing",0},   // id 11
  {"Temple Dash",255, 170,  0, trInit, trUpdate, trDraw, trIcon, "A:jump  B:slide",  "Stick: change lane",0}, // id 12
  {"Settings",    150, 150, 150, stInit, stUpdate, stDraw, stIcon, "", "", 1},
  {"HW Test",     160,   0, 255, tsInit, tsUpdate, tsDraw, tsIcon, "", "", 1},
};
static const uint8_t NGAMES = sizeof(GAMES) / sizeof(GAMES[0]);
// short one-line descriptions for the launcher (max 24 chars), same order as GAMES[]
static const char* const GAME_DESC[] = {
  "Jump, duck, survive", "Shoot the alien wave", "Stack and clear lines", "Weave through traffic",
  "Fly the tight cave", "Eat, grow, dodge rocks", "Smash every brick", "Beat the AI paddle",
  "Tap through the pipes", "Watch, then repeat", "Bat the chase down", "Gravity paddle bounce", "Run, jump, slide, coins", "Sound, LED, stick", "Test every input"
};
static_assert(sizeof(GAME_DESC) / sizeof(GAME_DESC[0]) == sizeof(GAMES) / sizeof(GAMES[0]), "GAME_DESC must match GAMES[]");
