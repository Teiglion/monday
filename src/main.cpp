#include <Arduboy2.h>
#include <ArduboyTones.h>
#include <stdbool.h>
#include "assets.h"
#include "music.h"

#define jump_time 500
#define attack_time 330
#define splash_delay 1000
#define final_screen_delay 2000
#define level_delay 3000

Arduboy2 arduboy;
ArduboyTones sound(arduboy.audio.enabled);

enum GameState 
{
  SPLASH,
  MENU,
  CONTROLS,
  PLAY,
  PAUSE,
  LEVEL_COMPLETE,
  FAIL,
  WIN
};

enum ActionState
{
    JUMP,
    TOP_JUMP,
    ATTACK,
    SIT,
    NONE
};

GameState state;
GameState prev_state;
ActionState a_state;

uint8_t splash_stack = 0;
const uint8_t* const PROGMEM splash_sprites [] = {
    splash1,
    splash2,
    splash3,
    splash4,
    splash5
};
uint8_t player_x = 8;
uint8_t player_y = 30;
uint32_t action_timestamp;
uint32_t timer_started;
bool player_state = true;
bool action_music = false;
uint8_t level;
uint8_t total_cups[3] = {5,8,8};
uint8_t cups;

//препятсвия
struct Obstacle {
    int16_t obsX;
    int8_t obsLane;
    int8_t type;
};
const uint8_t* const PROGMEM obstacleSprites[]  = { 
    electro,
    wires,
    bush1,
    chair,
    table,
    computer,
    cabinet,
    bush2
};

const int16_t MIN_SPACE = 24; //Мин раст между препятсвиями
int8_t obstacleWidths = 16;

const int8_t OBSTACLE_COUNT = 10;
Obstacle obstacles[OBSTACLE_COUNT];
int16_t spawnTimer = 0;
uint8_t obsSpeed = 1;
int16_t spawnDistance = 120;
uint8_t score = 0;
uint8_t difficultyLevel = 1;
bool obstaclesInit = false;

int16_t bgScrollx = 0;
const int8_t BG_SCROLL_SPEED = 1;


void drawControls();
void update_music();
void movePlayer();
void scrollBG();
//void initObstacles();
//void updateDifficulty();
//void spawnSingle();
//void spawnLine();
//void updateObstacles();
//void drawObstacles();
//bool checkCollision();

void resetGame()
{
    player_x = 8;
    player_y = 41;
    player_state = true;
    a_state = NONE;
    obstaclesInit = false;
    timer_started = millis();
}

void setup() {
    arduboy.begin();
    arduboy.clear();
    arduboy.setFrameRate(30);
    arduboy.audio.on();
    prev_state = state = SPLASH;
    a_state = NONE;
    cups = 0;
    obstaclesInit = false;
    sound.tones(menu_theme);
    timer_started = millis();
}

void loop() {
    if(!arduboy.nextFrame()) return;
    arduboy.clear();
    arduboy.pollButtons();
    update_music();
    if(state == SPLASH){
        uint32_t now = millis();
        const uint8_t *splash = (const uint8_t *)pgm_read_ptr(&splash_sprites[splash_stack]);
        Sprites::drawOverwrite(0, 0, splash, 0);

        if(now - timer_started >= splash_delay)
        {
            splash_stack++;
            timer_started = millis();
        }

        if(now - timer_started >= splash_delay && splash_stack == 5) 
        {
            state = MENU;
            splash_stack = 0;
        }
    }
    else if (state == MENU)
    {
        Sprites::drawOverwrite(0, 0, menu, 0);
        if (arduboy.justPressed(A_BUTTON))state = PLAY;
        if (arduboy.justPressed(B_BUTTON))state = CONTROLS;
    }
    else if (state == CONTROLS)
    {
        drawControls();
        if(arduboy.justPressed(B_BUTTON))state = MENU;
    }
    else if (state == PAUSE)
    {
        arduboy.setCursor(50, 10);
        arduboy.print(F("PAUSE"));

        arduboy.setCursor(15, 30);
        arduboy.print(F("PRESS A TO RESUME"));

        if (arduboy.justPressed(A_BUTTON)) state = PLAY;
    }
    else if (state == LEVEL_COMPLETE)
    {
        uint32_t now = millis();
        arduboy.setCursor(10, 10);
        arduboy.print(F("LEVEL COMPLETE"));
        arduboy.setCursor(10, 30);
        arduboy.print(F("Coffee "));
        arduboy.print(cups);
        arduboy.print('/');
        arduboy.println(total_cups[level]);
        arduboy.setCursor(10, 50);
        arduboy.print(F("Score "));
        arduboy.print(score);
        if(now - timer_started >= level_delay)
        {
            level++;
            state = PLAY;
        }  
    }
    else if (state == WIN)
    {
        Sprites::drawOverwrite(0, 0, win_img, 0);

        if (arduboy.justPressed(A_BUTTON))
        {
            timer_started = millis();
            a_state = NONE;
            obstaclesInit = false;
            state = PLAY;
        }
    }
    else if (state == FAIL)
    {
        Sprites::drawOverwrite(0, 0, fail_img, 0);

        if (arduboy.justPressed(A_BUTTON))
        {
            timer_started = millis();
            a_state = NONE;
            obstaclesInit = false;
            state = PLAY;
        }

        if (arduboy.justPressed(B_BUTTON))
        {
            state = MENU;
        }
    }
    else if (state == PLAY)
    {
        scrollBG();

       /*if (!obstaclesInit)
        {
            initObstacles();
            obstaclesInit = true;
        }
        updateObstacles();*/ 

        uint8_t x=0;
        for(uint8_t i=0; i<16;i++)
        {
           Sprites::drawOverwrite(x, 56, floor_img, 0); 
           x+=8;
        }
        arduboy.drawLine(0, 8, 128, 8, WHITE);
        arduboy.drawLine(0, 16, 128, 16, WHITE);

        movePlayer();
        /*drawObstacles();

        if (checkCollision())
        {
            timer_started = millis();
            a_state = NONE;
            obstaclesInit = false;
            state = FAIL;
        }*/
    }

   //Sprites::drawSelfMasked(50, 9, crack, 0);
   //Sprites::drawSelfMasked(20, 9, lamp, 0);
   //Sprites::drawSelfMasked(80, 9, vent, 0);

   //Sprites::drawOverwrite(5, 25, wires, 0);
   //Sprites::drawOverwrite(25, 31, cabinet, 0);
   //Sprites::drawOverwrite(25, 46, cabinet, 0);
   //Sprites::drawSelfMasked(25, 30, player1, 0);
   //Sprites::drawOverwrite(50, 25, electro, 0);
   //Sprites::drawSelfMasked(50, 46, bush1, 0);
   //Sprites::drawSelfMasked(50, 30, player2, 0);
   //Sprites::drawSelfMasked(75, 30, attack, 0);
   //Sprites::drawSelfMasked(5, 46, sit, 0);
   //Sprites::drawSelfMasked(65, 46, bush2, 0);
   //Sprites::drawSelfMasked(80, 44, chair, 0);
   //Sprites::drawSelfMasked(110, 38, computer, 0);
   //Sprites::drawSelfMasked(110, 44, table, 0);

   //Sprites::drawSelfMasked(80, 30, cup1, 0);
   //Sprites::drawSelfMasked(100, 30, cup2, 0);
   //Sprites::drawSelfMasked(100, 16, sit, 0);
   //Sprites::drawOverwrite(100, 31, cabinet, 0);
   //Sprites::drawOverwrite(100, 46, cabinet, 0);
    arduboy.display();
}

void update_music()
{
    if(action_music && (a_state == JUMP || a_state == TOP_JUMP || a_state == SIT))
    {
        sound.tones(jump_theme);
        action_music = false;
    }
    if(action_music && a_state == ATTACK)
    {
        sound.tones(attack_theme);
        action_music = false;
    }
    if(state == prev_state) return;
    if((prev_state == SPLASH && state == MENU) || (prev_state == CONTROLS && state == MENU)) return;
    switch (state)
    {
        case GameState::MENU : sound.tones(menu_theme); break;
        case GameState::PLAY : sound.noTone(); break;
        case GameState::LEVEL_COMPLETE : sound.tones(level_complete); break;
        case GameState::FAIL : sound.tones(fail_theme); break;
        case GameState::WIN : sound.tones(win_theme); break;
        default : break;
    }
    prev_state = state;
}

void drawControls(){
    arduboy.setCursor(1, 1);
    arduboy.print(F("CONTROLS || B TO BACK"));

    // линия под заголовком
    arduboy.drawLine(3, 9, 125, 9);

    //движение
    const uint8_t down_x = 33;
    const uint8_t top = 13;
    const uint8_t bottom= 21;

     // SIT
    arduboy.drawLine(down_x, top, down_x, bottom);
    arduboy.drawLine(down_x, bottom, down_x -3, bottom - 3);
    arduboy.drawLine(down_x, bottom, down_x +3, bottom - 3);

    arduboy.setCursor(5, 15);
    arduboy.print(F("SIT"));

       // UP
    const uint8_t up_x = 34;
    const uint8_t top_y = 26;
    const uint8_t bottom_y = 34;

    arduboy.drawLine(up_x, bottom_y, up_x, top_y);
    arduboy.drawLine(up_x, top_y, up_x - 3, top_y + 3);
    arduboy.drawLine(up_x, top_y, up_x + 3, top_y + 3);

    // JUMP
    arduboy.setCursor(5, 27);
    arduboy.print(F("JUMP"));

    //PAUSE
    arduboy.setCursor(5, 40);
    arduboy.print(F("PAUSE"));
    // стрелка влево
    const uint8_t x = 42;
    const uint8_t y = 43;
    arduboy.drawLine(x+6, y, x - 4, y);
    arduboy.drawLine(x - 4, y, x - 1, y - 3);
    arduboy.drawLine(x - 4, y, x - 1, y + 3);

    //ATTACK
    arduboy.setCursor(5,51);
    arduboy.print(F("ATTACK"));

    // стрелка вправо
    const uint8_t x1 = 49;
    const uint8_t y1 = 54;
    arduboy.drawLine(x1 - 6, y1, x1+ 4, y1);
    arduboy.drawLine(x1 + 4, y1, x1+ 1, y1 - 3);
    arduboy.drawLine(x1+ 4, y1, x1 + 1, y1 + 3);
    
    //PERK
    arduboy.setCursor(65, 15);
    arduboy.print(F("PERK"));
    arduboy.print(F(" A"));

    //MENU
    arduboy.setCursor(65,27);
    arduboy.print(F("MENU"));
    arduboy.print(F(" B"));

    //LONG JUMP
    arduboy.setCursor(65,40);
    arduboy.print(F("LONG"));
    arduboy.setCursor(65,51);
    arduboy.print(F("JUMP"));

    //стрелка вверх
    uint8_t up1 = 98;
    uint8_t down1 = 43;
    uint8_t down2 = 52;
    arduboy.drawLine(up1, down2, up1, down1);
    arduboy.drawLine(up1, down1, up1 - 3, down1 + 3);
    arduboy.drawLine(up1, down1, up1 + 3, down1 + 3);

    //+A
    arduboy.setCursor(102, 44);
    arduboy.print(F(" + A"));
}

void movePlayer()
{
    if (arduboy.everyXFrames(15)) player_state = !player_state;

    if(arduboy.justPressed(LEFT_BUTTON))state = PAUSE;

    if (arduboy.pressed(UP_BUTTON) && arduboy.pressed(A_BUTTON) && a_state == NONE) 
    {
        a_state = TOP_JUMP;
        player_y = 16;
        action_timestamp = millis();
        action_music = true;
    }
    else if(arduboy.justPressed(UP_BUTTON) && a_state == NONE)
    {
        a_state = JUMP;
        player_y = 16;
        action_timestamp = millis();
        action_music = true;
    } 
    else if(arduboy.justPressed(DOWN_BUTTON) && a_state == NONE)
    {
        a_state = SIT;
        player_y = 46;
        action_timestamp = millis();
        action_music = true;
    } 
    else if(arduboy.justPressed(RIGHT_BUTTON) && a_state == NONE)
    {
        a_state = ATTACK;
        player_y = 30;
        action_timestamp = millis();
        action_music = true;
    } 
    
    switch (a_state)
    {
        case ActionState::ATTACK : { Sprites::drawSelfMasked(player_x, player_y,attack, 0); uint32_t now = millis(); if(now - action_timestamp >= attack_time) a_state = NONE; break;}
        case ActionState::JUMP : {Sprites::drawSelfMasked(player_x, player_y,player2, 0); uint32_t now = millis(); if(now - action_timestamp >= jump_time) {a_state = NONE; player_y = 30;} break;}
        case ActionState::TOP_JUMP : {Sprites::drawSelfMasked(player_x, player_y,sit, 0); uint32_t now = millis(); if(now - action_timestamp >= jump_time) {a_state = NONE; player_y = 30;} break;}
        case ActionState::SIT : {Sprites::drawSelfMasked(player_x, player_y,sit, 0); uint32_t now = millis(); if(now - action_timestamp >= jump_time) {a_state = NONE; player_y = 30;} break;}
        case ActionState::NONE : {Sprites::drawSelfMasked(player_x, player_y,player_state ? player1 : player2, 0); break;}
    }
}

void scrollBG(){
    int16_t offset = -(bgScrollx % 128);
    Sprites::drawOverwrite(offset, 9, lamp, 0);
    Sprites::drawOverwrite(offset + 128, 9, lamp, 0);
    bgScrollx += BG_SCROLL_SPEED;
}
