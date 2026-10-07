#pragma once
// ===== Pin map (ESP32-C3 Super Mini) =====
#define PIN_SDA     6
#define PIN_SCL     7
#define PIN_JOY_X   0
#define PIN_JOY_Y   1
#define PIN_JOY_SW  3
#define PIN_BTN_A   5    // A and B swapped vs v1 (A was 4, B was 5)
#define PIN_BTN_B   4
#define PIN_TOUCH   10
#define PIN_LED     20   // WS2812 DIN (through 330R)
#define PIN_BUZZER  21

// ===== Behaviour =====
#define FRAME_MS    33        // ~30 fps
#define I2C_HZ      400000    // if the OLED is stable, try 800000 for smoother games
#define JOY_THRESH  900       // digital press threshold, ADC counts from centre (12-bit)
#define JOY_HYST    250       // digital release = JOY_THRESH - JOY_HYST (no flicker at the edge)
#define JOY_DEAD    260       // analog dead zone (counts): nothing happens inside this
#define JOY_RANGE   1700      // counts from centre that count as full deflection
#define JOY_FILT    3         // smoothing, 1 (heavy) .. 8 (none); new = old + (raw-old)*JOY_FILT/8
#define BTN_SWAP_AB  1        // 1 = swap the A/B switches in software (GPIO map above is untouched)
#define JOY_SWAP_XY  0
#define JOY_INVERT_X 0
#define JOY_INVERT_Y 0
// LED brightness, sound and buzzer type are now in the in-game Settings screen.
