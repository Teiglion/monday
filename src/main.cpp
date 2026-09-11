#include <Arduboy2.h>
#include <ArduboyTones.h>
#include <stdbool.h>
#include "assets.h"
#include "music.h"

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
    LONG_JUMP,
    ATTACK,
    NONE
};

GameState state;
GameState prev_state;
ActionState a_state;

const uint32_t splash_delay = 2000;
const uint32_t final_screen_delay = 2000;

uint8_t player_x = 8;
uint32_t action_timestap;
uint32_t timer_started;
bool player_state = true;
bool action_music = false;

//===========================Hammergo==============================
// MOVING
const uint8_t LINE_TOP[3] = {35, 42, 49};
uint8_t playerLine = 0;

//препятсвия
struct Obstacle {
    int16_t obsX;
    int8_t obsLane;
    int8_t type;
};
const uint8_t* const obstacleSprites[] PROGMEM = { //Rename
    box_img,
    bush1_img,
    car_img,
    bush2_img
};

const int16_t MIN_SPACE = 24; //Мин раст между препятсвиями
const int8_t obstacleWidths[] = {5, 5, 13, 5};

const int8_t OBSTACLE_COUNT = 6;
Obstacle obstacles[OBSTACLE_COUNT];
int16_t spawnTimer = 0;
uint8_t obsSpeed = 1;
int16_t spawnDistance = 120;
uint8_t score = 0;
uint8_t difficultyLevel = 1;
bool obstaclesInit = false;




//движение задника
int16_t bgScrollx = 0;
const int8_t BG_SCROLL_SPEED = 1;

//========================================================================
void drawControls();
void update_music();
void movePlayer();
void scrollBG();
void initObstacles();
void updateDifficulty();
void spawnSingle();
void spawnLine();
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
        Sprites::drawOverwrite(0, 0, start_img, 0);
        if(now - timer_started >= splash_delay) state = MENU;
    }
    else if (state == MENU)
    {
        Sprites::drawOverwrite(0, 0, title_img, 0);
        if (arduboy.justPressed(A_BUTTON))state = PLAY;
        if (arduboy.justPressed(B_BUTTON))state = CONTROLS;
    }
    else if (state == CONTROLS)
    {
        drawControls();
        if(arduboy.justPressed(B_BUTTON))state =  MENU;
    }
    else if (state == PAUSE)
    {
        arduboy.setCursor(50, 10);
        arduboy.print(F("PAUSE"));

        arduboy.setCursor(20, 30);
        arduboy.print(F("PRESS A TO RESUME"));

        if (arduboy.justPressed(A_BUTTON)) state = PLAY;
    }
    else if (state == LEVEL_COMPLETE){}
    else if (state == WIN)
    {
        uint32_t now = millis();
        if(now - timer_started < final_screen_delay) Sprites::drawOverwrite(0, 0, win_img, 0);
        else 
        {
            arduboy.setCursor(35, 20);
            arduboy.print(F("YOU DID IT!"));

            arduboy.setCursor(40, 40);
            arduboy.print(F("PRESS A"));
            if (arduboy.justPressed(A_BUTTON)) 
            {
                setup();
                state = PLAY;
            }
        }
    }
    else if (state == FAIL)
    {
        uint32_t now = millis();

        if (now - timer_started < final_screen_delay) Sprites::drawOverwrite(0, 0, fail_img, 0);
        else 
        {
            arduboy.setCursor(30, 10);
            arduboy.print(F("YOU'RE LATE!"));

            arduboy.setCursor(5, 30);
            arduboy.print(F("PRESS A TO TRY AGAIN"));
            arduboy.setCursor(5, 45);
            arduboy.print(F("PRESS B TO MAIN MENU"));

            if (arduboy.justPressed(A_BUTTON)) 
            {
                timer_started = millis();
                a_state = NONE;
                obstaclesInit = false;
                state = PLAY;
            }

            if (arduboy.justPressed(B_BUTTON))state = MENU;
        }
    }
    arduboy.display();
}

void drawControls(){
    arduboy.setCursor(40, 0);
    arduboy.print(F("CONTROLS"));

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

void movePlayer(){

}


//сменить имя файла задника
void scrollBG(){
    int16_t offset = -(bgScrollx % 128);
    Sprites::drawOverwrite(offset, 0, five_img, 0);
    Sprites::drawOverwrite(offset + 128, 0, five_img, 0);
    bgScrollx += BG_SCROLL_SPEED;
}
void initObstacles()
{
    for (int8_t i = 0; i < OBSTACLE_COUNT; i++)
    {
        obstacles[i].obsX = 128 + i * 50;
        obstacles[i].obsLane = random(0, 3);
        obstacles[i].type = random(0, 4);
    }
    spawnTimer = 0;
    score = 0;
    difficultyLevel = 1;
    obsSpeed = 1;
    spawnDistance = 120;
}

void updateDifficulty()
{
    if(score >= 50){
        state = WIN;
    }
    else if (score >= 20)
    {
        difficultyLevel = 3;
        obsSpeed = 3;
        spawnDistance = 30;
    }
    else if (score >= 10)
    {
        difficultyLevel = 2;
        obsSpeed = 2;
        spawnDistance = 40;
    }
    else
    {
        difficultyLevel = 1;
        obsSpeed = 1;
        spawnDistance = 50;
    }
}

void spawnSingle()
{
    for (int8_t i = 0; i < OBSTACLE_COUNT; i++)
    {
        if (obstacles[i].obsX <= -20)
        {
            obstacles[i].obsX = max((int16_t)(128 + random(0, 20)), (int16_t)(rightmostActiveX() + MIN_SPACE));
            obstacles[i].obsLane = random(0, 3);
            obstacles[i].type = random(0, 3);
            return;
        }
    }
}
void spawnLine(){
        int8_t size = (random(0, 100) < 50) ? 2 : 3;

    int8_t freeSlots = 0;
    for (int8_t i = 0; i < OBSTACLE_COUNT; i++)
    {
        if (obstacles[i].obsX <= -20) freeSlots++;
    }

    if (freeSlots < size)
    {
        spawnSingle();
        return;
    }

    bool usedLane[3] = { false, false, false };
    int8_t lanes[3];
    int8_t picked = 0;

    while (picked < size)
    {
        int8_t c = random(0, 3);
        if (!usedLane[c])
        {
            usedLane[c] = true;
            lanes[picked] = c;
            picked++;
        }
    }

    int16_t commonX = max((int16_t)(128 + random(0, 20)), (int16_t)(rightmostActiveX() + MIN_SPACE));
    int8_t slotIdx = 0;

    for (int8_t p = 0; p < size; p++)
    {
        while (slotIdx < OBSTACLE_COUNT && obstacles[slotIdx].obsX > -20) slotIdx++;
        if (slotIdx >= OBSTACLE_COUNT) break;

        obstacles[slotIdx].obsX = commonX;
        obstacles[slotIdx].obsLane = lanes[p];
        obstacles[slotIdx].type = random(0, 4);
        slotIdx++;
    }
}

void updateObstacles()
{
    for (int8_t i = 0; i < OBSTACLE_COUNT; i++)
    {
        if (obstacles[i].obsX > -20)
        {
            obstacles[i].obsX -= obsSpeed;
        }
    }

    for (int8_t i = 0; i < OBSTACLE_COUNT; i++)
    {
        if (obstacles[i].obsX > -20 && obstacles[i].obsX < -8)
        {
            score++;
            obstacles[i].obsX = -25;
        }
    }

    updateDifficulty();

    spawnTimer += obsSpeed;

    if (spawnTimer >= spawnDistance)
    {
        if (random(0, 100) < 65)
            spawnSingle();
        else
            spawnLine();

        spawnTimer = 0;
    }
}

void drawObstacles()
{
    for (int8_t i = 0; i < OBSTACLE_COUNT; i++)
    {
        if (obstacles[i].obsX < -20) continue;

        const uint8_t *spr = (const uint8_t*)pgm_read_ptr(&obstacleSprites[obstacles[i].type]);
        int8_t oy = LINE_TOP[obstacles[i].obsLane];
        Sprites::drawOverwrite(obstacles[i].obsX, oy, spr, 0);
    }
}

bool checkCollision()
{
    bool flag = false;
    for (int8_t i = 0; i < OBSTACLE_COUNT; i++)
    {
        if (obstacles[i].obsX < -20 || obstacles[i].obsLane != playerLine || a_state == JUMP || a_state == LONG_JUMP) continue;
        if(obstacles[i].type == 1 && a_state == ATTACK && player_x + 8 == obstacles[i].obsX && flag == false)
        {
            obstacles[i].type = 3;
            flag = true;
        }
        int8_t obsW = obstacleWidths[obstacles[i].type];
        if (player_x < obstacles[i].obsX + obsW && player_x + 8 > obstacles[i].obsX && obstacles[i].type != 3)return true;
    }
    return false;
}