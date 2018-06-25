#pragma once
#include "SDL.h"
#include "SDL_ttf.h"
#include <stdio.h>
#include <Windows.h>

#define GRAV 0.00001
#define PI 3.1415926535
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 760
#define SAVE_SLOT_COUNT 6
// Global eco
#define FUEL_MAX_PRICE 100.0
#define FUEL_MIN_PRICE 50.0
#define STARGATE_MAX_PRICE 2000.0
#define STARGATE_MIN_PRICE 1000.0
//
#define FOOD_MAX_PRICE 10.0
#define FOOD_MIN_PRICE 6.0
#define MED_MAX_PRICE 22.0
#define MED_MIN_PRICE 12.0
#define MIN_MAX_PRICE 15.0
#define MIN_MIN_PRICE 6.0
#define WEAP_MAX_PRICE 33.0
#define WEAP_MIN_PRICE 18.0
#define TECH_MAX_PRICE 60.0
#define TECH_MIN_PRICE 39.0
//
#define GOODS_NORMAL_QUANTITY 150

enum PLANET_TYPE {
  STAR,
  GAS_GIANT,
  UNINHABITABLE,
  UNEXPLORABLE,
  HABITABLE,
  MOON,
  MOON_UNEXPLORABLE,
  MOON_HABITABLE,
  STAR_GATE,
  COMET
};

enum SHIP_EVENT {
  NO_EVENT,
  GRAV_WELL_CHANGED,
  SHIP_CRASH,
  DEST_ARRIVED,
  FUEL_DEPLETE
};

struct quest_t;
struct worldEvent_t;

struct coord_t {
  double x, y;
};

struct body_t {
  coord_t vel, accel, coord;
  double mass, rad;
};

struct date_t {
  double time;
  int day, year;
};

struct planet_t {
  PLANET_TYPE type;
  body_t bodyProperties;
  int pop, clim, age, eco, satelliteCount,
    foodQ, medQ, minQ, weapQ, techQ, scansAvail;
  Uint8 rcol, gcol, bcol;
  planet_t *parent, *satellites, *next, *prev;
  double gravRadius, 
    foodPS, foodPB, medPS, medPB, minPS, minPB, weapPS, weapPB, techPS, techPB, fuelPrice;
  date_t lastScan;
  char *name;
  bool trader;
  quest_t *currQuest;
};

struct playerShip_t {
  coord_t moveStart, moveDest;
  body_t bodyProperties;
  double angle, turnAccel, turnVel, fuel, credits, globalSpeed, maxMass;
  int enginePowerLevel, turnRate, orbit,
    foodQ, medQ, minQ, weapQ, techQ;
  planet_t *well;
  bool engineOn;
};

struct timeMultipliers_t {
  double timeControl, scale, speed;
  double universal;
};

struct loadpointers_t {
  void *oldPtr, *newPtr;
};

struct worldEvent_t {
  int type, subtype, textLines, scrollLine, stage;
  char *text;
  planet_t *location;
};

struct flags_t {
  bool pause, quit, menu, start, info, globalMove, shiftModifier, questChange, disableQuests,
    inGameMenu, destSet, shipCrashed, infoChange, passInfChange, dateChange, modsChange, fuelDeplete;
  int menuMode, mapMode, systemCount, eventTrigger, scroll, scrollPos, textLinesGlobal;
};

struct quest_t {
  int type, subtype, stage, duration, textLines, textLinesWide;
  planet_t *starter, *goal;
  date_t start, end;
  char *text, *textWide;
  double reward;
  quest_t *next, *prev;
};

struct timers_t {
  double PCFreq, oldTime, newTime;
  LONGLONG starter;
  int date;
};

struct globals {
  planet_t *system, *infoTarget, *passiveInfoTarg, *currInterface, *trackPlanet;
  playerShip_t ship;
  timeMultipliers_t timeMods;
  quest_t *quests;
  flags_t flags;
  timers_t timers;
  worldEvent_t currEvent;
  date_t currDate;
  bool *saveSlots;
  double x;

  FILE *textSource;
  SDL_Texture *info, *passiveInfo, *dateTexture, *modsTexture, *questsTextureGlobal,
    *questTexturePlanet, *eventTexture;
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Point mousePos;
  SDL_Event event;
  TTF_Font *genFont;
};

struct savedata {
  int systemCount, mapMode;
  date_t currDate;
  quest_t *questMain;
  timeMultipliers_t globalStuff;
  playerShip_t ship;
  planet_t *system;
};