#include <Arduboy2.h>
#include <ArduboyTones.h>
#include <stdbool.h>
#include <EEPROM.h>
#include "assets.h"
#include "music.h"

#define ACTION_TIME 1100
#define splash_delay 1000
#define final_screen_delay 2000
#define level_delay 3000
#define GO_DELAY 1000
#define JUMP_Y 12
#define PITCH 20
#define SPAWN_X 130
#define OBSTACLE_COUNT 10
#define TOP_JUMP_DELAY 50

#define OBS(cell, obstacle) \
    { cell, ROUTE_OBSTACLE, obstacle }
#define CUP_MIDDLE(cell) \
    { cell, ROUTE_CUP_MIDDLE, 0 }
#define CUP_TOP(cell) \
    { cell, ROUTE_CUP_TOP, 0 }
#define BUSH(cell) \
    { cell, ROUTE_BUSH, 0 }
#define BUSH_WITH_TOP_HAZARD(cell, upperObstacle) \
    { cell, ROUTE_BUSH_WITH_TOP_HAZARD, upperObstacle }
#define TABLE_WITH_COMPUTER(cell) \
    { cell, ROUTE_TABLE_WITH_COMPUTER, 0 }
#define TALL_CABINET(cell) \
    { cell, ROUTE_TALL_CABINET, 0 }

Arduboy2 arduboy;
ArduboyTones sound(arduboy.audio.enabled);

struct RouteEvent
{
    uint8_t cell;
    uint8_t type;
    uint8_t arg;
};

enum Lane : uint8_t
{
    LANE_FLOOR = 0,
    LANE_MIDDLE = 1,
    LANE_TOP = 2
};

enum ObstacleType : uint8_t
{
    OBS_ELECTRO = 0,
    OBS_WIRES,
    OBS_BUSH_1,
    OBS_CHAIR,
    OBS_TABLE,
    OBS_COMPUTER,
    OBS_CABINET,
    OBS_BUSH_2
};

enum RouteType : uint8_t
{
    ROUTE_OBSTACLE = 0,
    ROUTE_CUP_MIDDLE,
    ROUTE_CUP_TOP,
    ROUTE_BUSH,
    ROUTE_BUSH_WITH_TOP_HAZARD,
    ROUTE_TABLE_WITH_COMPUTER,
    ROUTE_TALL_CABINET
};

uint8_t controls_page = 0;
const uint8_t* const PROGMEM controls_sprites[] =
{
    controls1,
    controls2
};

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
bool jump_waiting = false;
uint32_t jump_wait_timestamp = 0;
uint8_t level = 0;
uint8_t total_cups[3] = {5, 8, 8};
uint8_t cups = 0;
uint8_t cup_animation = 0;
uint8_t score = 0;
uint8_t run_score = 0;
uint8_t best_score = 0;
uint8_t level_start_score = 0;
const uint8_t EEPROM_MAGIC = 0xA7;
const uint8_t EEPROM_MAGIC_ADDR = 0;
const uint8_t EEPROM_BEST_SCORE_ADDR = 1;

struct Obstacle
{
    int16_t obsX;
    int8_t obsLane;
    int8_t type;
    int8_t oY;
    bool isBush;
    bool harmful;
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

int16_t bgScrollx = 0;
int8_t SCROLL_SPEED = 1;
const int8_t LEVEL_SPEEDS[3] = {1, 2, 3};

const uint8_t* const PROGMEM bgSprites[] =
{
    lamp,
    crack,
    vent
};
const uint8_t BG_SPRITE_COUNT = 3;
uint8_t bgSprite1 = 0;
uint8_t bgSprite2 = 1;

const RouteEvent PROGMEM route1[] =
{
    CUP_MIDDLE(0),
    OBS(1, OBS_ELECTRO),
    TALL_CABINET(5),
    BUSH(8),
    OBS(11, OBS_CHAIR),
    CUP_TOP(13),
    TABLE_WITH_COMPUTER(16),
    CUP_MIDDLE(18),
    BUSH_WITH_TOP_HAZARD(19, OBS_WIRES),
    CUP_TOP(22),
    TALL_CABINET(22),
    CUP_MIDDLE(24),
    OBS(27, OBS_WIRES),
    TABLE_WITH_COMPUTER(30)
  
};

const RouteEvent PROGMEM route2[] =
{
    CUP_MIDDLE(3),
    OBS(7, OBS_WIRES),
    CUP_TOP(11),
    OBS(15, OBS_CHAIR),
    BUSH(19),
    CUP_MIDDLE(18),
    TABLE_WITH_COMPUTER(23),
    CUP_TOP(25),
    BUSH_WITH_TOP_HAZARD(29, OBS_ELECTRO),
    OBS(34, OBS_ELECTRO),
    CUP_MIDDLE(38),
    TALL_CABINET(38),
    CUP_TOP(39),
    BUSH(42),
    CUP_TOP(44),
    OBS(46, OBS_TABLE),
    CUP_MIDDLE(49),
    OBS(52, OBS_WIRES),
    
};

const RouteEvent PROGMEM route3[] =
{
    BUSH(2),
    CUP_MIDDLE(5),
    OBS(9, OBS_WIRES),
    CUP_MIDDLE(15),
    OBS(16, OBS_CHAIR),
    CUP_TOP(19),
    OBS(23, OBS_CABINET),
    CUP_MIDDLE(26),
    BUSH_WITH_TOP_HAZARD(30, OBS_WIRES),
    CUP_MIDDLE(33),
    OBS(37, OBS_ELECTRO),
    CUP_TOP(44),
    TALL_CABINET(44),
    CUP_TOP(49),
    TABLE_WITH_COMPUTER(51),
    CUP_MIDDLE(54),
    BUSH(58)
};

const RouteEvent* const PROGMEM routes[] =
{
    route1,
    route2,
    route3
};

const uint8_t routeLens[3] =
{
    sizeof(route1) / sizeof(route1[0]),
    sizeof(route2) / sizeof(route2[0]),
    sizeof(route3) / sizeof(route3[0])
};

void update_music();
void movePlayer();
void scrollBG();
void scrollFloor();
void initBG();
uint8_t randomBGSprite();
void initCups();
void loadBestScores();
void saveBestScore();
void drawCups();
void updateCups();
void checkCupCollection();
void startLevel();
void updateObstacles();
void drawObstacles();
bool checkCollision();

void spawnRoute();

Obstacle* findFreeObstacle();
Obstacle* spawnObstacle(uint8_t type, Lane lane, int16_t x);
bool spawnCupAt(Lane lane, int16_t x);
bool spawnRouteEvent(const RouteEvent& ev, int16_t x);
int8_t obstacleY(uint8_t type, Lane lane);
int8_t cupY(Lane lane);
void readRouteEvent(const RouteEvent* route, uint8_t index, RouteEvent* out);

void setup() {
    arduboy.begin();
    randomSeed(micros());
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
                run_score = 0;
                startLevel();
                state = MENU;
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
        arduboy.setCursor(44, 52);
        arduboy.print(F("BS: "));
        arduboy.print(best_score);
        if (arduboy.justPressed(A_BUTTON)){ level = 0; cups = 0; score = 0; run_score = 0; startLevel(); state = PLAY; }
        if (arduboy.justPressed(B_BUTTON)){controls_page = 0;state = CONTROLS;}
    }
    else if (state == CONTROLS)
    {
        const uint8_t* controls = (const uint8_t*)pgm_read_ptr(&controls_sprites[controls_page]);

        Sprites::drawOverwrite(0, 0, controls, 0);

        if (arduboy.justPressed(B_BUTTON))
        {
            controls_page++;

            if (controls_page >= 2)
            {
                controls_page = 0;
                state = MENU;
            }
        }
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
        arduboy.print(F("RUN SCORE "));
        arduboy.print(run_score);
        if(now - timer_started >= level_delay)
        {
            level++;

            if (level >= 3) state = WIN;
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
            run_score = 0;
            startLevel();
            state = MENU;
        }
    }
    else if (state == FAIL)
    {
        Sprites::drawOverwrite(0, 0, fail_img, 0);

        if (arduboy.justPressed(A_BUTTON))
        {
            run_score = level_start_score;
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
            scrollFloor();
            arduboy.drawLine(0, 8, 128, 8, WHITE);
            arduboy.drawLine(0, 16, 128, 16, WHITE);
            const uint8_t* sprite1 = (const uint8_t*)pgm_read_ptr(&bgSprites[bgSprite1]);
            const uint8_t* sprite2 = (const uint8_t*)pgm_read_ptr(&bgSprites[bgSprite2]);
            Sprites::drawSelfMasked(0, 9, sprite1, 0);
            Sprites::drawSelfMasked(128, 9, sprite2, 0);
            Sprites::drawSelfMasked(player_x, player_y, player1, 0);
            arduboy.setCursor(56, 28);
            arduboy.print(F("GO!"));
        }
        else
        {
            scrollBG();
            spawnRoute();
            updateObstacles(); 
            scrollFloor();
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
                is_attacking = false;
                jump_waiting = false;
                player_y = 30;
                state = FAIL;
            }

            if (spawnIdx >= routeLens[level])
            {
                bool obstaclesStillActive = false;

                for (uint8_t i = 0; i < OBSTACLE_COUNT; i++)
                {
                    if (obstacles[i].obsX >= -20)
                    {
                        obstaclesStillActive = true;
                        break;
                    }
                }

                if (!obstaclesStillActive)
                {
                    saveBestScore();
                    timer_started = millis();
                    a_state = NONE;
                    is_attacking = false;
                    jump_waiting = false;
                    player_y = 30;
                    state = LEVEL_COMPLETE;
                }
            }
        }
    }
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

void movePlayer()
{
    if (arduboy.everyXFrames(15)) player_state = !player_state;

    if(arduboy.justPressed(LEFT_BUTTON)) state = PAUSE;

    if (a_state == NONE && !is_attacking)
    {
            if (!jump_waiting && arduboy.justPressed(UP_BUTTON))
        {
            jump_waiting = true;
            jump_wait_timestamp = millis();
        }

        if (jump_waiting && arduboy.pressed(A_BUTTON))
        {
            jump_waiting = false;

            a_state = TOP_JUMP;
            player_y = JUMP_Y;
            action_timestamp = millis();
            action_music = true;
        }
        else if (jump_waiting && millis() - jump_wait_timestamp >= TOP_JUMP_DELAY)
        {
            jump_waiting = false;

            a_state = JUMP;
            player_y = 16;
            action_timestamp = millis();
            action_music = true;
        }
        else if(!jump_waiting && arduboy.justPressed(DOWN_BUTTON))
        {
            a_state = SIT;
            player_y = 46;
            action_timestamp = millis();
            action_music = true;
        } 
        else if (!jump_waiting && arduboy.justPressed(RIGHT_BUTTON))
        {
            a_state = ATTACK;
            is_attacking = true;
            player_y = 30;
            action_timestamp = millis();
            action_music = true;
        }
    }

    if (a_state != NONE && millis() - action_timestamp >= ACTION_TIME)
    {
        a_state = NONE;
        is_attacking = false;
        player_y = 30;
    }

    if (is_attacking)
    {
        Sprites::drawSelfMasked(player_x, player_y, attack, 0);
        return;
    }
    
    switch (a_state)
    {
        case ActionState::JUMP : Sprites::drawSelfMasked(player_x, player_y, player2, 0); break;
        case ActionState::TOP_JUMP : Sprites::drawSelfMasked(player_x, player_y, sit, 0); break;
        case ActionState::SIT : Sprites::drawSelfMasked(player_x, player_y, sit, 0); break;
        case ActionState::NONE : Sprites::drawSelfMasked(player_x, player_y, player_state ? player1 : player2, 0); break;
        default : break;
    }
    
}


void scrollBG()
{
    int16_t offset = -(bgScrollx % 128);
    const uint8_t* sprite1 = (const uint8_t*)pgm_read_ptr(&bgSprites[bgSprite1]);
    const uint8_t* sprite2 = (const uint8_t*)pgm_read_ptr(&bgSprites[bgSprite2]);

    Sprites::drawSelfMasked(offset, 9, sprite1, 0);
    Sprites::drawSelfMasked(offset + 128, 9, sprite2, 0);

    bgScrollx += SCROLL_SPEED;

    if ((bgScrollx % 128) == 0)
    {
        bgSprite1 = bgSprite2;
        bgSprite2 = randomBGSprite();
    }
}

uint8_t randomBGSprite()
{
    return random(BG_SPRITE_COUNT);
}

void scrollFloor()
{
    int16_t offset = -(bgScrollx % 8);
    for (int16_t x = offset; x < 128; x += 8)
    {
        Sprites::drawOverwrite(x, 56, floor_img, 0);
    }
}

void initBG()
{
    bgSprite1 = randomBGSprite();
    bgSprite2 = randomBGSprite();
}

void initCups()
{
    for (uint8_t i = 0; i < CUP_COUNT; i++)
    {
        cupsOnMap[i].x = 0;
        cupsOnMap[i].y = 30;
        cupsOnMap[i].active = false;
    }
}

void loadBestScores()
{
    if (EEPROM.read(EEPROM_MAGIC_ADDR) != EEPROM_MAGIC)
    {
        best_score = 0;
        EEPROM.update(EEPROM_BEST_SCORE_ADDR, best_score);
        EEPROM.update(EEPROM_MAGIC_ADDR, EEPROM_MAGIC);
        return;
    }

    best_score = EEPROM.read(EEPROM_BEST_SCORE_ADDR);
}

void saveBestScore()
{
    if (run_score > best_score)
    {
        best_score = run_score;
        EEPROM.update(EEPROM_BEST_SCORE_ADDR, best_score);
    }
}

void drawCups()
{
    for (uint8_t i = 0; i < CUP_COUNT; i++)
    {
        if (!cupsOnMap[i].active) continue;

        switch(cup_animation)
        {
            case 0 : Sprites::drawOverwrite(cupsOnMap[i].x, cupsOnMap[i].y, cup1, 0); break;
            case 1 : Sprites::drawOverwrite(cupsOnMap[i].x, cupsOnMap[i].y, cup2, 0); break;
            case 2 : Sprites::drawOverwrite(cupsOnMap[i].x, cupsOnMap[i].y, cup3, 0); break;
        }   
    }
    if (arduboy.everyXFrames(8))
    {
        cup_animation++;
        if (cup_animation == 3) cup_animation = 0;
    }
}

void updateCups()
{
    for (uint8_t i = 0; i < CUP_COUNT; i++)
    {
        if (!cupsOnMap[i].active) continue;

        cupsOnMap[i].x -= obsSpeed;

        if (cupsOnMap[i].x < -8)
        {
            cupsOnMap[i].active = false;
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

        if (player_x < cupsOnMap[i].x + 8 && player_x + 16 > cupsOnMap[i].x &&
            player_y < cupsOnMap[i].y + 8 &&
            player_y + 16 > cupsOnMap[i].y)
        {
            cupsOnMap[i].active = false;

            cups++;
            score++;
            run_score++;

        }
    }
}

void startLevel()
{
    SCROLL_SPEED = LEVEL_SPEEDS[level];
    obsSpeed = SCROLL_SPEED;
    bgScrollx = 0;
    initBG();
    spawnIdx = 0;
    level_start_score = run_score;
    for (uint8_t i = 0; i < OBSTACLE_COUNT; i++)
    {
        obstacles[i].obsX = -40;
        obstacles[i].obsLane = LANE_FLOOR;
        obstacles[i].type = OBS_ELECTRO;
        obstacles[i].oY = 46;
        obstacles[i].isBush = false;
        obstacles[i].harmful = false;
    }
    cups = 0;
    score = 0;
    cup_animation = 0;
    player_x = 8;
    player_y = 30;
    a_state = NONE;
    jump_waiting = false;
    player_state = true;
    is_attacking = false;
    action_music = false;
    initCups();
    timer_started = millis();
}

int8_t obstacleY(uint8_t type, Lane lane)
{
    switch (type)
    {
        case OBS_ELECTRO :
        case OBS_WIRES : return (lane == LANE_TOP) ? 16 : 25;

        case OBS_CHAIR :
        case OBS_TABLE :
        case OBS_CABINET : return 44;

        case OBS_COMPUTER : return 38;

        case OBS_BUSH_1 :
        case OBS_BUSH_2 : return 26;
        default : return 44;
    }
}

int8_t cupY(Lane lane)
{
    switch (lane)
    {
        case LANE_MIDDLE : return 30;
        case LANE_TOP : return 18;
        default : return 30;
    }
}

Obstacle* findFreeObstacle()
{
    for (uint8_t i = 0; i < OBSTACLE_COUNT; i++)
    {
        if (obstacles[i].obsX < -20) return &obstacles[i];
    }
    return nullptr;
}

Obstacle* spawnObstacle(uint8_t type, Lane lane, int16_t x)
{
    Obstacle* o = findFreeObstacle();

    if (o == nullptr) return nullptr;

    o->obsX = x;
    o->obsLane = lane;
    o->type = type;
    o->oY = obstacleY(type, lane);

    o->isBush = (type == OBS_BUSH_1);
    o->harmful = true;

    return o;
}

bool spawnCupAt(Lane lane, int16_t x)
{
    for (uint8_t i = 0; i < CUP_COUNT; i++)
    {
        if (cupsOnMap[i].active) continue;

        cupsOnMap[i].x = x;
        cupsOnMap[i].y = cupY(lane);
        cupsOnMap[i].active = true;

        return true;
    }

    return false;
}

void readRouteEvent(const RouteEvent* route, uint8_t index, RouteEvent* out)
{
    memcpy_P(out, &route[index], sizeof(RouteEvent));
}

bool spawnRouteEvent(const RouteEvent& ev, int16_t x)
{
    switch (ev.type)
    {
        case ROUTE_OBSTACLE : return spawnObstacle(ev.arg, LANE_MIDDLE, x) != nullptr;

        case ROUTE_CUP_MIDDLE : return spawnCupAt(LANE_MIDDLE, x);

        case ROUTE_CUP_TOP : return spawnCupAt(LANE_TOP, x);

        case ROUTE_BUSH : return spawnObstacle(OBS_BUSH_1, LANE_FLOOR, x) != nullptr;

        case ROUTE_BUSH_WITH_TOP_HAZARD :
        {
            Obstacle* bush = spawnObstacle(OBS_BUSH_1, LANE_FLOOR, x);

            if (bush == nullptr) return false;

            Obstacle* topHazard = spawnObstacle(ev.arg, LANE_TOP, x);

            if (topHazard == nullptr)
            {
                bush->obsX = -40;
                return false;
            }

            return true;
        }

        case ROUTE_TABLE_WITH_COMPUTER :
        {
            Obstacle* table = spawnObstacle(OBS_TABLE, LANE_FLOOR, x);

            if (table == nullptr) return false;

            Obstacle* computer = spawnObstacle(OBS_COMPUTER, LANE_MIDDLE, x);

            if (computer == nullptr)
            {
                table->obsX = -40;
                return false;
            }

            return true;
        }

        case ROUTE_TALL_CABINET :
        {
            Obstacle* lowerCabinet =
                spawnObstacle(OBS_CABINET, LANE_FLOOR, x);

            if (lowerCabinet == nullptr) return false;

            Obstacle* upperCabinet = spawnObstacle(OBS_CABINET, LANE_MIDDLE, x);

            if (upperCabinet == nullptr)
            {
                lowerCabinet->obsX = -40;
                return false;
            }

            upperCabinet->oY = 28;

            return true;
        }

        default : return false;
    }
}

void spawnRoute()
{
    const RouteEvent* route = (const RouteEvent*)pgm_read_ptr(&routes[level]);

    while (spawnIdx < routeLens[level])
    {
        RouteEvent ev;
        readRouteEvent(route, spawnIdx, &ev);

        int16_t targetScroll = (int16_t)ev.cell * PITCH;

        if (targetScroll > bgScrollx) break;

        int16_t spawnX = SPAWN_X + (bgScrollx - targetScroll);

        if (!spawnRouteEvent(ev, spawnX)) break;

        spawnIdx++;
    }
}



void updateObstacles(){
    for(uint8_t i = 0; i < OBSTACLE_COUNT; i++){
        if(obstacles[i].obsX < -20) continue;
        obstacles[i].obsX -= SCROLL_SPEED;
    }
}

void drawObstacles(){
    for(uint8_t i = 0; i < OBSTACLE_COUNT; i++)
    {
        Obstacle *o = &obstacles[i];
        if(o->obsX < -16 || o->obsX > 140) continue; 
        const uint8_t* sprite = (const uint8_t*)pgm_read_ptr(&obstacleSprites[o->type]);
        Sprites::drawSelfMasked(o->obsX, o->oY, sprite, 0);
    }
}


bool checkCollision()
{
    int16_t px = (int16_t)player_x + 4;
    int16_t pw = 8;

    int16_t py = 30;
    int16_t ph = 24;

    switch (a_state)
    {
        case ActionState::JUMP : py = player_y; ph = 20; break;
        case ActionState::TOP_JUMP : py = player_y; ph = 16; break;
        case ActionState::SIT : py = 46; ph = 10; break;
        case ActionState::ATTACK : py = 30; ph = 24; break;
        default : py = 30; ph = 24; break;
    }

    if (is_attacking)
    {
        int16_t attackX = (int16_t)player_x + 4;
        int16_t attackW = 24;

        for (uint8_t i = 0; i < OBSTACLE_COUNT; i++)
        {
            Obstacle* o = &obstacles[i];

            if (o->obsX < -20 || !o->harmful || !o->isBush) continue;

            int16_t bushX = o->obsX;
            int16_t bushW = 16;
            int16_t bushY = o->oY;
            int16_t bushH = 32;

            bool hitByAttack = attackX < bushX + bushW && bushX < attackX + attackW && player_y < bushY + bushH && bushY < player_y + 32;

            if (hitByAttack)
            {
                o->type = OBS_BUSH_2;
                o->isBush = false;
                o->harmful = false;
                break;
            }
        }
    }

    for (uint8_t i = 0; i < OBSTACLE_COUNT; i++)
    {
        Obstacle* o = &obstacles[i];

        if (o->obsX < -20 || !o->harmful) continue;

        int16_t ox = o->obsX + 3;
        int16_t ow = 10;

        int16_t oy = o->oY;
        int16_t oh = 16;

        if (o->type == OBS_BUSH_1)
        {
            oy = o->oY + 5;
            oh = 27;
        }

        if (o->type == OBS_ELECTRO || o->type == OBS_WIRES)
        {
            if (o->obsLane == LANE_TOP)
            {
                oy = o->oY;
                oh = 12;
            }
            else
            {
                oy = o->oY + 2;
                oh = 12;
            }
        }

        if (o->type == OBS_COMPUTER)
        {
            oy = o->oY - 4;
            oh = 8;
        }

        bool overlap = px < ox + ow && ox < px + pw && py < oy + oh &&oy < py + ph;

        if (!overlap)continue;

        return true;
    }
    return false;
}