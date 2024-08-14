// =============================================================================
//
// Arduino - Flappy Bird clone
// ---------------------------
// by Themistokle "mrt-prodz" Benetatos and refined by Ashraf Hany for the Nano
//
// This is a Flappy Bird clone made for the ATMEGA328 and a Sainsmart 1.8" TFT
// screen (ST7735). It features an intro screen, a game over screen containing
// the player score and a similar gameplay from the original game.
//
// Developed and tested with an Arduino UNO and a Sainsmart 1.8" TFT screen.
//
// Dependencies:
// - https://github.com/adafruit/Adafruit-GFX-Library
// - https://github.com/adafruit/Adafruit-ST7735-Library
//
// =============================================================================

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// Initialize Sainsmart 1.8" TFT screen (connect pins accordingly or change these values)
const int TFT_DC = 9;     // Sainsmart RS/DC
const int TFT_RST = 8;    // Sainsmart RES
const int TFT_CS = 10;    // Sainsmart CS

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Screen constants
const int TFTW = 128;    // Screen width
const int TFTH = 160;    // Screen height
const int TFTW2 = TFTW / 2;    // Half screen width
const int TFTH2 = TFTH / 2;    // Half screen height

// Game constants
const float GRAVITY = 9.8;
const float JUMP_FORCE = 2.15;
const double SKIP_TICKS = 20.0;    // 1000 / 50fps
const int MAX_FRAMESKIP = 5;
const int SPEED = 1;

// Bird size
const int BIRDW = 8;     // Bird width
const int BIRDH = 8;     // Bird height
const int BIRDW2 = BIRDW / 2;    // Half width
const int BIRDH2 = BIRDH / 2;    // Half height

// Pipe size
const int PIPEW = 12;    // Pipe width
const int GAPHEIGHT = 36;    // Pipe gap height

// Floor size
const int FLOORH = 20;    // Floor height (from bottom of the screen)

// Grass size
const int GRASSH = 4;    // Grass height (inside floor, starts at floor y)

// Colors
const uint16_t BCKGRDCOL = tft.color565(138, 235, 244);
const uint16_t BIRDCOL = tft.color565(255, 254, 174);
const uint16_t PIPECOL = tft.color565(99, 255, 78);
const uint16_t PIPEHIGHCOL = tft.color565(250, 255, 250);
const uint16_t PIPESEAMCOL = tft.color565(0, 0, 0);
const uint16_t FLOORCOL = tft.color565(246, 240, 163);
const uint16_t GRASSCOL = tft.color565(141, 225, 87);
const uint16_t GRASSCOL2 = tft.color565(156, 239, 88);
const uint16_t C1 = tft.color565(195, 165, 75);
const uint16_t C5 = tft.color565(251, 216, 114);

// Bird sprite colors (Cx name for values to keep the array readable)
const uint16_t birdcol[] = {
  BCKGRDCOL, BCKGRDCOL, C1, C1, C1, C1, C1, BCKGRDCOL,
  BCKGRDCOL, C1, BIRDCOL, BIRDCOL, BIRDCOL, C1, ST7735_WHITE, C1,
  BCKGRDCOL, BIRDCOL, BIRDCOL, BIRDCOL, BIRDCOL, C1, ST7735_WHITE, C1,
  C1, C1, C1, BIRDCOL, BIRDCOL, ST7735_WHITE, C1, C1,
  C1, BIRDCOL, BIRDCOL, BIRDCOL, BIRDCOL, BIRDCOL, ST7735_RED, ST7735_RED,
  C1, BIRDCOL, BIRDCOL, BIRDCOL, C1, C5, ST7735_RED, BCKGRDCOL,
  BCKGRDCOL, C1, BIRDCOL, C1, C5, C5, C5, BCKGRDCOL,
  BCKGRDCOL, BCKGRDCOL, C1, C5, C5, C5, BCKGRDCOL, BCKGRDCOL
};

// Bird structure
struct Bird {
  uint8_t x, y, old_y;
  float vel_y;
} bird;

// Pipe structure
struct Pipe {
  int8_t x, gap_y;
} pipe;

// Score
int16_t score;
// Temporary x and y variables
int16_t tmpx, tmpy;

// Faster drawPixel method by inlining calls and using setAddrWindow and pushColor
#define drawPixel(a, b, c) { tft.setAddrWindow(a, b, a, b); tft.pushColor(c); }

void setup() {
  // Initialize the push button on pin 2 as an input
  pinMode(2, INPUT);
  // Initialize the ST7735S chip, black tab
  tft.initR(INITR_BLACKTAB);
  tft.fillScreen(BCKGRDCOL);
}

void loop() {
  game_start();
  game_loop();
  game_over();
}

void game_loop() {
  uint8_t GAMEH = TFTH - FLOORH;
  tft.drawFastHLine(0, GAMEH, TFTW, ST7735_BLACK);
  tft.fillRect(0, GAMEH + 1, TFTW2, GRASSH, GRASSCOL);
  tft.fillRect(TFTW2, GAMEH + 1, TFTW2, GRASSH, GRASSCOL2);
  tft.drawFastHLine(0, GAMEH + GRASSH, TFTW, ST7735_BLACK);
  tft.fillRect(0, GAMEH + GRASSH + 1, TFTW, FLOORH - GRASSH, FLOORCOL);
  int8_t grassx = TFTW;
  
  double delta, old_time, next_game_tick = millis(), current_time = next_game_tick;
  int loops;
  bool passed_pipe = false;

  while (true) {
    loops = 0;
    while (millis() > next_game_tick && loops < MAX_FRAMESKIP) {
      if (!digitalRead(2)) {
        bird.vel_y = bird.y > BIRDH2 * 0.5 ? -JUMP_FORCE : 0;
      }

      old_time = current_time;
      current_time = millis();
      delta = (current_time - old_time) / 1000.0;

      bird.vel_y += GRAVITY * delta;
      bird.y += bird.vel_y;

      pipe.x -= SPEED;
      if (pipe.x < -PIPEW) {
        pipe.x = TFTW;
        pipe.gap_y = random(10, GAMEH - (10 + GAPHEIGHT));
      }

      next_game_tick += SKIP_TICKS;
      loops++;
    }

    if (pipe.x >= 0 && pipe.x < TFTW) {
      tft.drawFastVLine(pipe.x + 3, 0, pipe.gap_y, PIPECOL);
      tft.drawFastVLine(pipe.x + 3, pipe.gap_y + GAPHEIGHT + 1, GAMEH - (pipe.gap_y + GAPHEIGHT + 1), PIPECOL);
      tft.drawFastVLine(pipe.x, 0, pipe.gap_y, PIPEHIGHCOL);
      tft.drawFastVLine(pipe.x, pipe.gap_y + GAPHEIGHT + 1, GAMEH - (pipe.gap_y + GAPHEIGHT + 1), PIPEHIGHCOL);
      drawPixel(pipe.x, pipe.gap_y, PIPESEAMCOL);
      drawPixel(pipe.x, pipe.gap_y + GAPHEIGHT, PIPESEAMCOL);
      drawPixel(pipe.x, pipe.gap_y - 6, PIPESEAMCOL);
      drawPixel(pipe.x, pipe.gap_y + GAPHEIGHT + 6, PIPESEAMCOL);
      drawPixel(pipe.x + 3, pipe.gap_y - 6, PIPESEAMCOL);
      drawPixel(pipe.x + 3, pipe.gap_y + GAPHEIGHT + 6, PIPESEAMCOL);
    }
    if (pipe.x <= TFTW) tft.drawFastVLine(pipe.x + PIPEW, 0, GAMEH, BCKGRDCOL);

    tmpx = BIRDW - 1;
    do {
      int8_t px = bird.x + tmpx + BIRDW;
      tmpy = BIRDH - 1;
      do {
        drawPixel(px, bird.old_y + tmpy, BCKGRDCOL);
      } while (tmpy--);
    } while (tmpx--);

    if (pipe.x >= -BIRDW2 && pipe.x <= BIRDW2) {
      if (bird.y - BIRDH2 < pipe.gap_y || bird.y + BIRDH2 > pipe.gap_y + GAPHEIGHT) {
        bird.vel_y = -JUMP_FORCE;
      } else if (!passed_pipe) {
        passed_pipe = true;
        score++;
      }
    } else if (pipe.x < -BIRDW2) {
      passed_pipe = false;
    }

    if (bird.y > GAMEH - BIRDH2) {
      bird.y = GAMEH - BIRDH2;
    }
    if (bird.y - BIRDH2 < 0) {
      bird.y = BIRDH2;
    }

    tmpx = BIRDW - 1;
    do {
      int8_t px = bird.x + tmpx;
      tmpy = BIRDH - 1;
      do {
        drawPixel(px, bird.y + tmpy, birdcol[tmpy * BIRDW + tmpx]);
      } while (tmpy--);
    } while (tmpx--);

    bird.old_y = bird.y;

    if (++grassx >= TFTW) grassx = 0;
    tft.drawFastHLine(grassx, GAMEH + 1, 2, GRASSCOL);
    tft.drawFastHLine(grassx, GAMEH + 3, 2, GRASSCOL2);
    if (grassx == 0) continue;
    tft.drawFastHLine(grassx - 2, GAMEH + 1, 2, FLOORCOL);
    tft.drawFastHLine(grassx - 2, GAMEH + 3, 2, FLOORCOL);
  }
}

void game_start() {
  tft.fillScreen(BCKGRDCOL);
  tft.setTextColor(ST7735_BLACK);
  tft.setTextWrap(true);
  tft.setCursor(28, 64);
  tft.setTextSize(2);
  tft.print("Flappy");
  tft.setCursor(36, 96);
  tft.print("Bird");
  tft.setTextSize(1);
  tft.setCursor(24, 130);
  tft.print("press button");
  tft.setCursor(32, 140);
  tft.print("to start");

  while (digitalRead(2));
  delay(20);
  while (!digitalRead(2));

  bird.y = bird.old_y = TFTH2;
  bird.vel_y = 0.0;
  bird.x = 18;

  pipe.x = TFTW;
  pipe.gap_y = random(10, TFTH - (10 + GAPHEIGHT + FLOORH));

  score = 0;
}

void game_over() {
  tft.fillScreen(BCKGRDCOL);
  tft.setTextColor(ST7735_BLACK);
  tft.setTextWrap(true);
  tft.setCursor(24, 64);
  tft.setTextSize(2);
  tft.print("Game Over");
  tft.setCursor(36, 96);
  tft.setTextSize(1);
  tft.print("score:");
  tft.setCursor(68, 96);
  tft.print(score);
  tft.setCursor(20, 130);
  tft.print("press button");
  tft.setCursor(28, 140);
  tft.print("to restart");

  while (digitalRead(2));
  delay(20);
  while (!digitalRead(2));
}
