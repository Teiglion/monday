#include "music.h"
#include <ArduboyTones.h>

const uint16_t PROGMEM menu_theme[]  = {
  NOTE_D4,      320,
  NOTE_A4,      160,
  NOTE_D5,      224,
  NOTE_REST,     64,

  NOTE_F4,      256,
  NOTE_A4,      160,
  NOTE_C5,      224,
  NOTE_REST,     64,

  NOTE_B3,      320,
  NOTE_F4,      160,
  NOTE_B4,      224,
  NOTE_REST,     64,

  NOTE_D4,      192,
  NOTE_F4,      160,
  NOTE_A4,      192,
  NOTE_REST,     96,

  NOTE_C4,      256,
  NOTE_G4,      160,
  NOTE_C5,      224,
  NOTE_REST,     64,

  NOTE_E4,      192,
  NOTE_G4,      160,
  NOTE_B4,      192,
  NOTE_REST,     96,

  NOTE_D4,      320,
  NOTE_A4,      160,
  NOTE_D5,      320,
  NOTE_REST,    192,

  TONES_REPEAT
};

const uint16_t PROGMEM level_complete[] = {
  NOTE_C5,       110,
  NOTE_E5,       110,
  NOTE_G5,       110,
  NOTE_C6H,      220,

  NOTE_REST,      70,

  NOTE_G5,       100,
  NOTE_C6,       110,
  NOTE_E6H,      280,

  NOTE_REST,      80,

  TONES_END
};

const uint16_t PROGMEM win_theme[] = {
  NOTE_C5,       120,
  NOTE_E5,       120,
  NOTE_G5,       140,
  NOTE_C6H,      260,

  NOTE_REST,     180,

  NOTE_E5,       120,
  NOTE_G5,       120,
  NOTE_C6,       140,
  NOTE_E6H,      320,

  NOTE_REST,     500,

  TONES_REPEAT
};

const uint16_t PROGMEM fail_theme[] = {
  NOTE_E5,       150,
  NOTE_D5,       150,
  NOTE_C5,       210,

  NOTE_REST,     120,

  NOTE_G4,       180,
  NOTE_E4,       180,
  NOTE_C4H,      330,

  NOTE_REST,     750,

  TONES_REPEAT
};

const uint16_t PROGMEM jump_theme[] = {
  NOTE_C5,  35,
  NOTE_E5,  35,
  NOTE_A5H, 65,
  TONES_END
};

const uint16_t PROGMEM attack_theme[] = {
  NOTE_E4H, 75,
  NOTE_C4,  110,
  TONES_END
};