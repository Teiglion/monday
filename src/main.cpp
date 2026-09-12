#include <Arduboy2.h>
#include <ArduboyTones.h>
#include <stdbool.h>
#include <EEPROM.h>
#include "assets.h"
#include "music.h"

#define jump_time 1100
#define attack_time 330
#define splash_delay 1000
#define final_screen_delay 2000
#define level_delay 3000
#define GO_DELAY 1000
#define JUMP_Y 12
#define PITCH 20
#define SPAWN_X 130
#define OBSTACLE_COUNT 10

// кодирование маршрута: байт = (spriteIdx<<3) | (lane&7); бит 6 - ворота (куст+препятствие)
#define EMPTY 0
#define O(sp, ln) ((uint8_t)(((sp) << 3) | ((ln) & 7)))
#define GGE 0x40
#define G(sp) ((uint8_t)(GGE | ((sp) << 3)))

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
bool is_attacking = false;
uint8_t level = 0;
uint8_t total_cups[3] = {5, 8, 8};
uint8_t cups = 0;
uint8_t best_score[3] = {0, 0, 0};

//препятствия
struct Obstacle {
    int16_t obsX;
    int8_t obsLane;
    int8_t type;
    int8_t oY;
    bool isBush;
};

struct Cup
{
    int16_t x;
    int8_t y;
    bool active;
};

const uint8_t CUP_COUNT = 8;
Cup cupsOnMap[CUP_COUNT];

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

Obstacle obstacles[OBSTACLE_COUNT];
uint16_t spawnIdx = 0;
uint8_t obsSpeed = 1;
uint8_t score = 0;

int16_t bgScrollx = 0;
int8_t SCROLL_SPEED = 1;
const int8_t LEVEL_SPEEDS[3] = {1, 1, 2};

// карта спавна препятствий для каждого уровня
const uint8_t PROGMEM route1[] = {
    EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
    EMPTY,  EMPTY,  EMPTY,
    O(4,0), EMPTY,  EMPTY,
    O(1,1), EMPTY,  EMPTY,
    EMPTY,  EMPTY,
    G(0),   EMPTY,  EMPTY,
    O(4,0), EMPTY,  EMPTY,
    O(2,1), EMPTY,  EMPTY,
    O(6,1), EMPTY,  EMPTY,
    G(1),   EMPTY,  EMPTY,
    O(5,0), EMPTY,  EMPTY,
    O(7,0), EMPTY,  EMPTY,
    EMPTY,  EMPTY
};

const uint8_t PROGMEM route2[] = {
    EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
    EMPTY,  EMPTY,  EMPTY,  EMPTY,
    O(4,0), EMPTY,  EMPTY,  EMPTY,
    O(1,1), EMPTY,  EMPTY,
    G(0),   EMPTY,  EMPTY,  EMPTY,
    O(7,0), EMPTY,  EMPTY,
    O(3,0), EMPTY,  EMPTY,  EMPTY,
    O(2,1), EMPTY,  EMPTY,
    G(1),   EMPTY,  EMPTY,  EMPTY,
    O(5,0), EMPTY,  EMPTY,
    O(6,0), EMPTY,  EMPTY,
    O(0,1), EMPTY,  EMPTY,
    EMPTY,  EMPTY
};

const uint8_t PROGMEM route3[] = {
    EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
    EMPTY,  EMPTY,  EMPTY,  EMPTY,
    O(1,1), EMPTY,  EMPTY,  EMPTY,
    O(3,0), EMPTY,  EMPTY,
    G(0),   EMPTY,  EMPTY,
    O(2,1), EMPTY,  EMPTY,  EMPTY,
    O(5,0), EMPTY,  EMPTY,
    G(1),   EMPTY,  EMPTY,
    O(4,0), EMPTY,  EMPTY,  EMPTY,
    O(7,0), EMPTY,  EMPTY,
    O(6,1), EMPTY,  EMPTY,
    O(0,1), EMPTY,  EMPTY,  EMPTY,
    G(0),   EMPTY,  EMPTY,
    O(3,0), EMPTY,  EMPTY,  EMPTY,
    O(4,0), EMPTY,  EMPTY,
    EMPTY,  EMPTY
};

const uint8_t* const PROGMEM routes[] = {route1, route2, route3};
const uint8_t routeLens[3] = {44, 50, 60};


void drawControls();
void update_music();
void movePlayer();
void scrollBG();
void initCups();
void loadBestScores();
void saveBestScore();
void drawCups();
void updateCups();
void checkCupCollection();
void startLevel();
void spawnRoute();
void spawnObject(uint8_t ev, int16_t x);
void updateObstacles();
void drawObstacles();
bool checkCollision();

void setup() {
    arduboy.begin();
    arduboy.clear();
    arduboy.setFrameRate(30);
    arduboy.audio.on();

    prev_state = state = SPLASH;
    a_state = NONE;

    level = 0;
    cups = 0;
    score = 0;

    initCups(); 

    loadBestScores();

    sound.tones(menu_theme);
    timer_started = millis();
}

void loop() {
    if(!arduboy.nextFrame()) return;
    arduboy.clear();
    arduboy.pollButtons();
    update_music();
    if(state == SPLASH)
    {
        uint32_t now = millis();

        const uint8_t *splash =
            (const uint8_t *)pgm_read_ptr(&splash_sprites[splash_stack]);

        Sprites::drawOverwrite(0, 0, splash, 0);

        if(now - timer_started >= splash_delay)
        {
            splash_stack++;

            if(splash_stack >= 5)
            {
                splash_stack = 0;
                level = 0;
                cups = 0;
                score = 0;
                startLevel();
                state = PLAY;
            }
            else
            {
                timer_started = millis();
            }
        }
    }
    else if (state == MENU)
    {
        Sprites::drawOverwrite(0, 0, menu, 0);
        if (arduboy.justPressed(A_BUTTON)){ level = 0; cups = 0; score = 0; startLevel(); state = PLAY; }
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
        arduboy.print(F("CUPS "));
        arduboy.print(cups);
        arduboy.print('/');
        arduboy.print(total_cups[level]);

        arduboy.setCursor(10, 45);
        arduboy.print(F("CUPS "));
        arduboy.print(score);

        arduboy.setCursor(10, 55);
        arduboy.print(F("BEST "));
        arduboy.print(best_score[level]);
        if(now - timer_started >= level_delay)
        {
            level++;

            if (level >= 3)
            {
                state = WIN;
            }
            else
            {
                cups = 0;
                startLevel();
                state = PLAY;
            }
        }  
    }
    else if (state == WIN)
    {
        Sprites::drawOverwrite(0, 0, win_img, 0);

        if (arduboy.justPressed(A_BUTTON))
        {
            level = 0;
            cups = 0;
            score = 0;
            startLevel();
            state = PLAY;
        }
    }
    else if (state == FAIL)
    {
        Sprites::drawOverwrite(0, 0, fail_img, 0);

        if (arduboy.justPressed(A_BUTTON))
        {
            startLevel();
            state = PLAY;
        }

        if (arduboy.justPressed(B_BUTTON))
        {
            state = MENU;
        }
    }
    else if (state == PLAY)
    {
        if (millis() - timer_started < GO_DELAY)
        {
            uint8_t x = 0;
            for(uint8_t i = 0; i < 16; i++)
            {
                Sprites::drawOverwrite(x, 56, floor_img, 0);
                x += 8;
            }
            arduboy.drawLine(0, 8, 128, 8, WHITE);
            arduboy.drawLine(0, 16, 128, 16, WHITE);
            Sprites::drawOverwrite(0, 9, lamp, 0);
            Sprites::drawOverwrite(128, 9, lamp, 0);
            Sprites::drawSelfMasked(player_x, player_y, player1, 0);
            arduboy.setCursor(56, 28);
            arduboy.print(F("GO!"));
        }
        else
        {
            scrollBG();
            spawnRoute();
            updateObstacles(); 

            uint8_t x=0;
            for(uint8_t i=0; i<16;i++)
            {
                Sprites::drawOverwrite(x, 56, floor_img, 0); 
                x+=8;
            }
            arduboy.setCursor(2, 0);
            arduboy.print(F("CUPS: "));
            arduboy.print(score);

            arduboy.setCursor(82, 0);
            arduboy.print(cups);
            arduboy.print('/');
            arduboy.print(total_cups[level]);

            arduboy.drawLine(0, 8, 128, 8, WHITE);
            arduboy.drawLine(0, 16, 128, 16, WHITE);

            updateCups();
            drawCups();
            drawObstacles();
            movePlayer();
            checkCupCollection();

            if (checkCollision())
            {
                timer_started = millis();
                a_state = NONE;
                state = FAIL;
            }

            if (cups >= total_cups[level])
            {
                saveBestScore();

                timer_started = millis();
                state = LEVEL_COMPLETE;
            }
        }
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

    if(arduboy.justPressed(LEFT_BUTTON)) state = PAUSE;
    
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
        player_y = JUMP_Y;
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


    if (a_state != NONE && a_state != ATTACK) {
        uint32_t now = millis();
        if (now - action_timestamp >= jump_time) {
            a_state = NONE;
            player_y = 30;
        }
    }

    if(arduboy.justPressed(RIGHT_BUTTON) && !is_attacking)
    {
        is_attacking = true;
        if (a_state == NONE) {
            action_timestamp = millis();
        }
        action_music = true;
        sound.tones(attack_theme);
    }

    static uint32_t attack_start = 0;
    if (arduboy.justPressed(RIGHT_BUTTON)) attack_start = millis();
    if (is_attacking && (millis() - attack_start >= attack_time)) {
        is_attacking = false;
    }

    if (is_attacking) {
       Sprites::drawSelfMasked(player_x, player_y, attack, 0);
    } else {
        switch (a_state)
        {
            case ActionState::JUMP:     Sprites::drawSelfMasked(player_x, player_y, player2, 0); break;
            case ActionState::TOP_JUMP: Sprites::drawSelfMasked(player_x, player_y, sit, 0); break;
            case ActionState::SIT:      Sprites::drawSelfMasked(player_x, player_y, sit, 0); break;
            case ActionState::NONE:     Sprites::drawSelfMasked(player_x, player_y, player_state ? player1 : player2, 0); break;
            default: break;
        }
    }
}


void scrollBG(){
    int16_t offset = -(bgScrollx % 128);
    Sprites::drawOverwrite(offset, 9, lamp, 0);
    Sprites::drawOverwrite(offset + 128, 9, lamp, 0);
    bgScrollx += SCROLL_SPEED;
}

bool cupFrame = false;

void initCups()
{
    cupsOnMap[0].x = 130;
    cupsOnMap[0].y = 30;
    cupsOnMap[0].active = true;

    cupsOnMap[1].x = 170;
    cupsOnMap[1].y = 16;
    cupsOnMap[1].active = true;

    cupsOnMap[2].x = 210;
    cupsOnMap[2].y = 46;
    cupsOnMap[2].active = true;

    cupsOnMap[3].x = 250;
    cupsOnMap[3].y = 30;
    cupsOnMap[3].active = true;

    cupsOnMap[4].x = 290;
    cupsOnMap[4].y = 16;
    cupsOnMap[4].active = true;

    cupsOnMap[5].x = 330;
    cupsOnMap[5].y = 46;
    cupsOnMap[5].active = true;

    cupsOnMap[6].x = 370;
    cupsOnMap[6].y = 30;
    cupsOnMap[6].active = true;

    cupsOnMap[7].x = 410;
    cupsOnMap[7].y = 16;
    cupsOnMap[7].active = true;
}

void loadBestScores()
{
    for (uint8_t i = 0; i < 3; i++)
    {
        best_score[i] = EEPROM.read(i);
    }
}

void saveBestScore()
{
    if (score > best_score[level])
    {
        best_score[level] = score;
        EEPROM.update(level, score);
    }
}

void drawCups()
{
    if (arduboy.everyXFrames(8))
    {
        cupFrame = !cupFrame;
    }

    for (uint8_t i = 0; i < CUP_COUNT; i++)
    {
        if (!cupsOnMap[i].active)
            continue;

        if (cupFrame)
            Sprites::drawOverwrite(cupsOnMap[i].x, cupsOnMap[i].y, cup1, 0);
        else
            Sprites::drawOverwrite(cupsOnMap[i].x, cupsOnMap[i].y, cup2, 0);
    }
}

void updateCups()
{
    for (uint8_t i = 0; i < CUP_COUNT; i++)
    {
        if (!cupsOnMap[i].active)
            continue;

        cupsOnMap[i].x -= obsSpeed;

        if (cupsOnMap[i].x < -8)
        {
            cupsOnMap[i].x = 130;
        }
    }
}

void checkCupCollection()
{
    if (!arduboy.justPressed(A_BUTTON))
        return;

    for (uint8_t i = 0; i < CUP_COUNT; i++)
    {
        if (!cupsOnMap[i].active)
            continue;

        if (player_x < cupsOnMap[i].x + 8 &&
            player_x + 16 > cupsOnMap[i].x &&
            player_y < cupsOnMap[i].y + 8 &&
            player_y + 16 > cupsOnMap[i].y)
        {
            cupsOnMap[i].active = false;

            cups++;
            score++;

            if (cups >= total_cups[level])
            {
                saveBestScore();

                timer_started = millis();
                state = LEVEL_COMPLETE;
            }
        }
    }
}

void startLevel(){
    SCROLL_SPEED = LEVEL_SPEEDS[level];
    obsSpeed = SCROLL_SPEED;
    bgScrollx = 0;
    spawnIdx = 0;
    for(uint8_t i = 0; i < OBSTACLE_COUNT; i++) obstacles[i].obsX = -40;
    cups = 0;
    score = 0;
    player_x = 8;
    player_y = 30;
    a_state = NONE;
    player_state = true;
    action_music = false;
    initCups();
    timer_started = millis();
}

void spawnRoute(){
    while(spawnIdx < routeLens[level]){
        int16_t targetScroll = (int16_t)spawnIdx * PITCH;
        
        // Если до этого момента игрок еще не добежал — прекращаем спавн на этом кадре
        if (targetScroll > bgScrollx) {
            break;
        }
        
        uint8_t ev = pgm_read_byte((const uint8_t*)pgm_read_ptr(&routes[level]) + spawnIdx);
        if(ev) {
            int16_t exactX = SPAWN_X + (bgScrollx - targetScroll);
            spawnObject(ev, exactX);
        }
        spawnIdx++;
    }
}

void spawnObject(uint8_t ev, int16_t x){
    bool gate = ev & GGE;
    uint8_t idx = (ev >> 3) & 0x07;
    uint8_t lane = ev & 0x07;

    Obstacle *o = NULL;
    for(uint8_t i = 0; i < OBSTACLE_COUNT; i++){
        if(obstacles[i].obsX < -20){ o = &obstacles[i]; break; }
    }
    if(!o) return;

    if(!gate){
        o->obsX = x;
        o->obsLane = lane;
        o->type = idx;
        o->oY = (lane == 1) ? 25 : 46;
        o->isBush = (idx == 2 || idx == 7);
    }
    else{
        o->obsX = x;
        o->obsLane = 2;
        o->type = idx;
        o->oY = 25;
        o->isBush = false;

        for(uint8_t i = 0; i < OBSTACLE_COUNT; i++){
            if(obstacles[i].obsX < -20){
                obstacles[i].obsX = x;
                obstacles[i].obsLane = 0;
                obstacles[i].type = 2; // bush1
                obstacles[i].oY = 46;
                obstacles[i].isBush = true;
                break;
            }
        }
    }
}

void updateObstacles(){
    for(uint8_t i = 0; i < OBSTACLE_COUNT; i++){
        if(obstacles[i].obsX < -20) continue;
        obstacles[i].obsX -= SCROLL_SPEED;
    }
}

void drawObstacles(){
    for(uint8_t i = 0; i < OBSTACLE_COUNT; i++){
        Obstacle *o = &obstacles[i];
        if(o->obsX < -16 || o->obsX > 140) continue; 
        Sprites::drawOverwrite(o->obsX, o->oY, (const uint8_t*)pgm_read_ptr(&obstacleSprites[o->type]), 0);
    }
}


bool checkCollision(){
    int16_t px = (int16_t)player_x + 4, pw = 8;
    int16_t py, ph;

    switch (a_state)
    {
        case ActionState::JUMP:     py = 16 + 6; ph = 20; break;
        case ActionState::TOP_JUMP: py = 16 + 2; ph = 16; break;
        case ActionState::SIT:      py = 46 + 4; ph = 8; break;
        default:                    py = 30 + 4; ph = 24; break; // NONE и старый ATTACK
    }

    for(uint8_t i = 0; i < OBSTACLE_COUNT; i++){
        Obstacle *o = &obstacles[i];
        if(o->obsX < -20) continue;

        int16_t ox = o->obsX + 4, ow = 8;
        int16_t oy = o->oY + 4, oh = 8;


        if(px < ox + ow && ox < px + pw && py < oy + oh && oy < py + ph)
        {

            if(o->isBush && is_attacking)
            {
                o->obsX = -40;
                continue;  
            }    
            return true; 
        }
    }
    return false;
}
