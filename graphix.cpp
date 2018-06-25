#include "SDL.h"
#include "SDL_ttf.h"
#include "physix.h"
#include "graphix.h"
#include <math.h>

void DrawShip(SDL_Renderer *renderer, playerShip_t ship)
{
  SDL_Point triangle[4];
  double t = ship.angle;
  triangle[0].x = (int)(SCREEN_WIDTH / 2 + 10 * cos(t));
  triangle[0].y = (int)(SCREEN_HEIGHT / 2 + 10 * sin(t));
  t += 5 * PI / 6;
  triangle[1].x = (int)(SCREEN_WIDTH / 2 + 10 * cos(t));
  triangle[1].y = (int)(SCREEN_HEIGHT / 2 + 10 * sin(t));
  t += PI / 3;
  triangle[2].x = (int)(SCREEN_WIDTH / 2 + 10 * cos(t));
  triangle[2].y = (int)(SCREEN_HEIGHT / 2 + 10 * sin(t));
  triangle[3] = triangle[0];
  SDL_SetRenderDrawColor(renderer, 250, 250, 250, 0);
  SDL_RenderDrawLines(renderer, triangle, 4);
  return;
}

void _DrawBodies(SDL_Renderer *renderer, SDL_Point mapCenter, coord_t nullPoint, double distScale, double radScale, planet_t body, int mode)
{
  if (mode == 0)
  {
    if (abs((int)((body.bodyProperties.coord.x - nullPoint.x) * distScale)) > 100 ||
      abs((int)((body.bodyProperties.coord.y - nullPoint.y) * distScale)) > 100)
      return;
  }
  else
    if (abs((int)((body.bodyProperties.coord.x - nullPoint.x) * distScale)) > SCREEN_HEIGHT / 2 ||
      abs((int)((body.bodyProperties.coord.y - nullPoint.y) * distScale)) > SCREEN_HEIGHT / 2)
      return;
  SDL_Point circle[31];
  double diff = PI / 15, t = 0.0;
  for (int i = 0; i < 30; ++i)
  {
    circle[i].x = mapCenter.x + (int)((body.bodyProperties.coord.x - nullPoint.x) * distScale + cos(t) * (2 + 4 * (mode != 0) + 1 * (radScale * sqrt(body.bodyProperties.rad))));
    circle[i].y = mapCenter.y + (int)((body.bodyProperties.coord.y - nullPoint.y) * distScale + sin(t) * (2 + 4 * (mode != 0) + 1 * (radScale * sqrt(body.bodyProperties.rad))));
    t += diff;
  }
  circle[30] = circle[0];
  SDL_SetRenderDrawColor(renderer, body.rcol, body.gcol, body.bcol, 0);
  SDL_RenderDrawLines(renderer, circle, 31);
  return;
}

void _DrawMapShip(SDL_Renderer *renderer, SDL_Point mapCenter, coord_t nullPoint, double distScale, playerShip_t body, int mode)
{
  if (mode == 0)
  {
    if (abs((int)((body.bodyProperties.coord.x - nullPoint.x) * distScale)) > 100 ||
      abs((int)((body.bodyProperties.coord.y - nullPoint.y) * distScale)) > 100)
      return;
  }
  else
    if (abs((int)((body.bodyProperties.coord.x - nullPoint.x) * distScale)) > SCREEN_HEIGHT / 2 ||
      abs((int)((body.bodyProperties.coord.y - nullPoint.y) * distScale)) > SCREEN_HEIGHT / 2)
      return;
  SDL_Point triangle[4];
  double t = body.angle;
  triangle[0].x = mapCenter.x + (int)((body.bodyProperties.coord.x - nullPoint.x) * distScale + cos(t) * 4);
  triangle[0].y = mapCenter.y + (int)((body.bodyProperties.coord.y - nullPoint.y) * distScale + sin(t) * 4);
  t += 5 * PI / 6;
  triangle[1].x = mapCenter.x + (int)((body.bodyProperties.coord.x - nullPoint.x) * distScale + cos(t) * 4);
  triangle[1].y = mapCenter.y + (int)((body.bodyProperties.coord.y - nullPoint.y) * distScale + sin(t) * 4);
  t += PI / 3;
  triangle[2].x = mapCenter.x + (int)((body.bodyProperties.coord.x - nullPoint.x) * distScale + cos(t) * 4);
  triangle[2].y = mapCenter.y + (int)((body.bodyProperties.coord.y - nullPoint.y) * distScale + sin(t) * 4);
  triangle[3] = triangle[0];
  SDL_SetRenderDrawColor(renderer, 250, 250, 250, 0);
  SDL_RenderDrawLines(renderer, triangle, 4);
  return;
}

void _MapGen(globals *all)
{
  coord_t nullCoord = all->ship.well->bodyProperties.coord;
  SDL_Point mapCenter = {};
  SDL_Rect mapBG = {};
  planet_t *mapOwner = all->ship.well, *satPtr = all->ship.well->satellites;
  double distScale = 0.0, radScale = 0.0;
  switch (all->flags.mapMode)
  {
  case 0:
    mapCenter = { 100, SCREEN_HEIGHT - 100 };
    distScale = 100.0 / mapOwner->gravRadius;
    radScale = 5.0 / sqrt(mapOwner->bodyProperties.rad);
    mapBG = { 0, SCREEN_HEIGHT - 200, 200, 200 };
    break;
  case 1:
    mapCenter = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };
    distScale = SCREEN_HEIGHT / (2 * mapOwner->gravRadius);
    radScale = 15.0 / sqrt(mapOwner->bodyProperties.rad);
    mapBG = { (SCREEN_WIDTH - SCREEN_HEIGHT) / 2, 0, SCREEN_HEIGHT, SCREEN_HEIGHT };
    nullCoord = { 0.0, 0.0 };
    break;
  case 2:
    mapOwner = all->system;
    satPtr = mapOwner->satellites;
    mapCenter = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };
    distScale = SCREEN_HEIGHT / (2 * mapOwner->gravRadius);
    radScale = 15.0 / sqrt(mapOwner->bodyProperties.rad);
    mapBG = { (SCREEN_WIDTH - SCREEN_HEIGHT) / 2, 0, SCREEN_HEIGHT, SCREEN_HEIGHT };
    nullCoord = { 0.0, 0.0 };
    break;
  }
  SDL_SetRenderDrawColor(all->renderer, 0, 0, 0, 0);
  SDL_RenderFillRect(all->renderer, &mapBG);
  SDL_SetRenderDrawColor(all->renderer, 200, 200, 200, 0);
  SDL_RenderDrawRect(all->renderer, &mapBG);
  _DrawBodies(all->renderer, mapCenter, nullCoord, distScale, radScale, *mapOwner, all->flags.mapMode);
  for (int i = 0; i < mapOwner->satelliteCount; ++i)
  {
    _DrawBodies(all->renderer, mapCenter, nullCoord, distScale, radScale, *satPtr, all->flags.mapMode);
    satPtr = satPtr->next;
  }
  _DrawMapShip(all->renderer, mapCenter, nullCoord, distScale, all->ship, all->flags.mapMode);
  return;
}

void _DrawBodies(SDL_Renderer *renderer, coord_t shipPos, double scale, planet_t body)
{
  if (abs((int)(((body.bodyProperties.coord.x - shipPos.x) + body.bodyProperties.rad) * scale)) > SCREEN_WIDTH / 2 &&
    abs((int)(((body.bodyProperties.coord.y - shipPos.y) + body.bodyProperties.rad) * scale)) > SCREEN_HEIGHT / 2)
    return;
  SDL_Point circle[101];
  double diff = PI / 50, t = 0.0;
  for (int i = 0; i < 100; ++i)
  {
    circle[i].x = SCREEN_WIDTH / 2 + (int)(((body.bodyProperties.coord.x - shipPos.x) + cos(t) * body.bodyProperties.rad) * scale);
    circle[i].y = SCREEN_HEIGHT / 2 + (int)(((body.bodyProperties.coord.y - shipPos.y) + sin(t) * body.bodyProperties.rad) * scale);
    t += diff;
  }
  circle[100] = circle[0];
  SDL_SetRenderDrawColor(renderer, body.rcol, body.gcol, body.bcol, 0);
  SDL_RenderDrawLines(renderer, circle, 101);
  return;
}

void _DrawShip(SDL_Renderer *renderer, playerShip_t ship)
{
  SDL_Point triangle[4];
  double t = ship.angle;
  triangle[0].x = (int)(SCREEN_WIDTH / 2 + 10 * cos(t));
  triangle[0].y = (int)(SCREEN_HEIGHT / 2 + 10 * sin(t));
  t += 5 * PI / 6;
  triangle[1].x = (int)(SCREEN_WIDTH / 2 + 10 * cos(t));
  triangle[1].y = (int)(SCREEN_HEIGHT / 2 + 10 * sin(t));
  t += PI / 3;
  triangle[2].x = (int)(SCREEN_WIDTH / 2 + 10 * cos(t));
  triangle[2].y = (int)(SCREEN_HEIGHT / 2 + 10 * sin(t));
  triangle[3] = triangle[0];
  SDL_SetRenderDrawColor(renderer, 250, 250, 250, 0);
  SDL_RenderDrawLines(renderer, triangle, 4);
  return;
}

void _DrawSystem(SDL_Renderer *renderer, planet_t system, playerShip_t ship, timeMultipliers_t mods)
{
  planet_t *plaPtr = system.satellites, *satPtr = NULL;
  _DrawBodies(renderer, ship.bodyProperties.coord, mods.scale, system);
  for (int i = 0; i < system.satelliteCount; ++i)
  {
    _DrawBodies(renderer, ship.bodyProperties.coord, mods.scale, *plaPtr);
    satPtr = plaPtr->satellites;
    for (int j = 0; j < plaPtr->satelliteCount; ++j)
    {
      _DrawBodies(renderer, ship.bodyProperties.coord, mods.scale, *satPtr);
      satPtr = satPtr->next;
    }
    plaPtr = plaPtr->next;
  }
  _DrawShip(renderer, ship);
  return;
}

void _ShipInterface(globals *all)
{
  SDL_Surface *textSurf = NULL;
  SDL_Texture *textTexture = NULL;
  char text[9] = "";
  SDL_Rect speedRect = { SCREEN_WIDTH - 150, SCREEN_HEIGHT - 150, 150, 150 }, orbitRect = { SCREEN_WIDTH - 150, SCREEN_HEIGHT - 50, 50, 50 },
    dateRect = { SCREEN_WIDTH - 100, SCREEN_HEIGHT - 50, 100, 50 }, timeModsRect = { SCREEN_WIDTH - 150, SCREEN_HEIGHT - 90, 50, 30 },
    fuelRect = { SCREEN_WIDTH - 100, SCREEN_HEIGHT - 75, 100, 25 }, fuelCounter = { SCREEN_WIDTH - 100, SCREEN_HEIGHT - 75, 100, 25 };
  SDL_Point circle[31];
  double diff = 2 * PI / 30, t = 0.0, angle = 0.0;
  switch (all->flags.mapMode)
  {
  case 0:
  case 2:
    for (int i = 0; i < 30; ++i)
    {
      circle[i].x = (int)(46 * cos(t) + SCREEN_WIDTH - 50);
      circle[i].y = (int)(46 * sin(t) + SCREEN_HEIGHT - 100);
      t += diff;
    }
    circle[30] = circle[0];
    if (all->ship.bodyProperties.vel.x - all->ship.well->bodyProperties.vel.x)
    {
      angle = atan((all->ship.bodyProperties.vel.y - all->ship.well->bodyProperties.vel.y) / (all->ship.bodyProperties.vel.x - all->ship.well->bodyProperties.vel.x));
      if (all->ship.bodyProperties.vel.x - all->ship.well->bodyProperties.vel.x < 0)
        angle += PI;
    }
    else
      angle = PI / 2 + PI * (all->ship.bodyProperties.vel.y < 0);
    SDL_SetRenderDrawColor(all->renderer, 50, 50, 50, 0);
    SDL_RenderFillRect(all->renderer, &speedRect);
    SDL_SetRenderDrawColor(all->renderer, 250, 250, 250, 0);
    SDL_RenderDrawLines(all->renderer, circle, 31);
    SDL_SetRenderDrawColor(all->renderer, 0, 250, 0, 0);
    SDL_RenderDrawLine(all->renderer, SCREEN_WIDTH - 50, SCREEN_HEIGHT - 100, (int)(cos(angle) * 45) + SCREEN_WIDTH - 50, (int)(sin(angle) * 45) + SCREEN_HEIGHT - 100);
    angle = _GetAngle(all->ship.bodyProperties.coord, all->ship.well->bodyProperties.coord);
    SDL_SetRenderDrawColor(all->renderer, 250, 250, 0, 0);
    SDL_RenderDrawLine(all->renderer, SCREEN_WIDTH - 50, SCREEN_HEIGHT - 100, (int)(cos(angle) * 45) + SCREEN_WIDTH - 50, (int)(sin(angle) * 45) + SCREEN_HEIGHT - 100);
    if (all->trackPlanet)
    {
      angle = _GetAngle(all->ship.bodyProperties.coord, all->trackPlanet->bodyProperties.coord);
      SDL_SetRenderDrawColor(all->renderer, 0, 0, 250, 0);
      SDL_RenderDrawLine(all->renderer, SCREEN_WIDTH - 50, SCREEN_HEIGHT - 100, (int)(cos(angle) * 45) + SCREEN_WIDTH - 50, (int)(sin(angle) * 45) + SCREEN_HEIGHT - 100);
    }
    fuelCounter = { SCREEN_WIDTH - 100, SCREEN_HEIGHT - 175, 100, 25 };
    fuelRect = { SCREEN_WIDTH - 100, SCREEN_HEIGHT - 175, 100, 25 };
    fuelCounter.w = (int)(fuelCounter.w * (all->ship.enginePowerLevel / 5.0));
    SDL_SetRenderDrawColor(all->renderer, 250, 250, 0, 0);
    SDL_RenderFillRect(all->renderer, &fuelCounter);
    SDL_SetRenderDrawColor(all->renderer, 250, 250, 250, 0);
    SDL_RenderDrawRect(all->renderer, &fuelRect);
    switch (all->ship.orbit)
    {
    case 0:
      SDL_SetRenderDrawColor(all->renderer, 0, 0, 255, 0);
      break;
    case 1:
      SDL_SetRenderDrawColor(all->renderer, 0, 255, 0, 0);
      break;
    case 2:
      SDL_SetRenderDrawColor(all->renderer, 255, 0, 0, 0);
      break;
    case 3:
      SDL_SetRenderDrawColor(all->renderer, 255, 255, 0, 0);
      break;
    }
    SDL_RenderFillRect(all->renderer, &orbitRect);
    if (all->flags.modsChange)
    {
      if (all->modsTexture != NULL)
        SDL_DestroyTexture(all->modsTexture);
      sprintf_s(text, "x%.3f", (float)(all->timeMods.timeControl / all->timeMods.scale));
      timeModsRect.x = 0;
      timeModsRect.y = 0;
      timeModsRect.h = 30;
      timeModsRect.w = 50;
      all->modsTexture = SDL_CreateTexture(all->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, timeModsRect.w, timeModsRect.h);
      SDL_SetRenderTarget(all->renderer, all->modsTexture);
      SDL_SetRenderDrawColor(all->renderer, 50, 50, 50, 0);
      SDL_RenderFillRect(all->renderer, &timeModsRect);
      textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
      textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
      SDL_FreeSurface(textSurf);
      SDL_RenderCopy(all->renderer, textTexture, NULL, &timeModsRect);
      SDL_DestroyTexture(textTexture);
      SDL_SetRenderTarget(all->renderer, NULL);
      timeModsRect.y = SCREEN_HEIGHT - 90;
      timeModsRect.x = SCREEN_WIDTH - 150;
      SDL_RenderCopy(all->renderer, all->modsTexture, NULL, &timeModsRect);
      all->flags.modsChange = false;
    }
    else
      if (all->modsTexture != NULL)
      {
        timeModsRect.h = 30;
        timeModsRect.h = 50;
        timeModsRect.y = SCREEN_HEIGHT - 90;
        timeModsRect.x = SCREEN_WIDTH - 150;
        SDL_RenderCopy(all->renderer, all->modsTexture, NULL, &timeModsRect);
      }
    break;
  case 1:
    fuelCounter.w = (int)(fuelCounter.w * all->ship.fuel);
    SDL_SetRenderDrawColor(all->renderer, 0, 250, 0, 0);
    SDL_RenderFillRect(all->renderer, &fuelCounter);
    SDL_SetRenderDrawColor(all->renderer, 250, 250, 250, 0);
    SDL_RenderDrawRect(all->renderer, &fuelRect);
    break;
  }
  if (all->flags.dateChange)
  {
    if (all->dateTexture != NULL)
      SDL_DestroyTexture(all->dateTexture);
    sprintf_s(text, "%i.%i", all->currDate.day, all->currDate.year);
    dateRect.x = 0;
    dateRect.y = 0;
    dateRect.h = 50;
    dateRect.w = 13 * strlen(text);
    all->dateTexture = SDL_CreateTexture(all->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, dateRect.w, dateRect.h);
    SDL_SetRenderTarget(all->renderer, all->dateTexture);
    SDL_SetRenderDrawColor(all->renderer, 50, 50, 50, 0);
    SDL_RenderFillRect(all->renderer, &dateRect);
    textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 0, 0 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    SDL_RenderCopy(all->renderer, textTexture, NULL, &dateRect);
    SDL_DestroyTexture(textTexture);
    SDL_SetRenderTarget(all->renderer, NULL);
    dateRect.y = SCREEN_HEIGHT - 50;
    dateRect.x = SCREEN_WIDTH - dateRect.w - 2;
    SDL_RenderCopy(all->renderer, all->dateTexture, NULL, &dateRect);
  }
  else
    if (all->dateTexture != NULL)
    {
      dateRect.h = 50;
      dateRect.y = SCREEN_HEIGHT - 50;
      dateRect.w = 5 * 13;
      dateRect.w += (all->currDate.day > 9) * 13;
      dateRect.w += (all->currDate.day > 99) * 13;
      dateRect.x = SCREEN_WIDTH - dateRect.w - 2;
      SDL_RenderCopy(all->renderer, all->dateTexture, NULL, &dateRect);
    }
}

/* Menu modes:
0 - main start
1 - main in-game
2 - save
3 - load
4 - hab planet/trader
5 - other planets
6 - gen event
7 - star gate
8 - ship menu
9 - victory menu
*/
void _DrawMenu(globals *all)
{
  const int textWidth = 25, textHeight = 20, textWidthNormal = 12;
  bool checkQuest = false;
  SDL_Surface *textSurf = NULL;
  SDL_Texture *textTexture = NULL;
  quest_t *currQuest = all->quests;
  int height = 0;
  date_t tmpDate = {};
  char buttonText[20] = {}, *charPtr = NULL, *tmpPtr = NULL, text[100] = {};
  SDL_Rect button = { SCREEN_WIDTH / 2 - 200, 200, 100, 400 }, lineBox = { 0, 0, 0, 0 }, textBox = {}, textBoxSecondary = {},
    scrollBar = {}, scrollGuide = {}, textureBox = {};
  button.w = 400;
  button.x = SCREEN_WIDTH / 2 - 200;
  SDL_SetRenderDrawColor(all->renderer, 200, 200, 200, 0);
  if (all->flags.scroll == 1)
  {
    if (all->flags.shiftModifier)
    {
      if (all->flags.scrollPos > 0)
        all->flags.scrollPos -= 10;
      if (all->flags.scrollPos < 0)
        all->flags.scrollPos = 0;
    }
    else
      if (all->flags.scrollPos > 0)
        --all->flags.scrollPos;
  }
  switch (all->flags.menuMode)
  {
#pragma region Simple Menus
  case 0:
    height = SCREEN_HEIGHT / 13;
    button.h = height;
    button.y = 2 * height;
    SDL_RenderDrawRect(all->renderer, &button);
    textSurf = TTF_RenderText_Solid(all->genFont, "New Game", { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    lineBox = button;
    lineBox.w = 9 * textWidth;
    lineBox.x = (SCREEN_WIDTH - lineBox.w) / 2;
    SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
    SDL_DestroyTexture(textTexture);
    //
    button.y = 4 * height;
    SDL_RenderDrawRect(all->renderer, &button);
    if (all->saveSlots[0] == false)
      textSurf = TTF_RenderText_Solid(all->genFont, "Continue", { 150, 150, 150 });
    else
      textSurf = TTF_RenderText_Solid(all->genFont, "Continue", { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    lineBox = button;
    lineBox.w = 9 * textWidth;
    lineBox.x = (SCREEN_WIDTH - lineBox.w) / 2;
    SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
    SDL_DestroyTexture(textTexture);
    //
    button.y = 6 * height;
    SDL_RenderDrawRect(all->renderer, &button);
    textSurf = TTF_RenderText_Solid(all->genFont, "Save", { 150, 150, 150 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    lineBox = button;
    lineBox.w = 5 * textWidth;
    lineBox.x = (SCREEN_WIDTH - lineBox.w) / 2;
    SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
    SDL_DestroyTexture(textTexture);
    //
    button.y = 8 * height;
    SDL_RenderDrawRect(all->renderer, &button);
    textSurf = TTF_RenderText_Solid(all->genFont, "Load", { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    lineBox = button;
    lineBox.w = 5 * textWidth;
    lineBox.x = (SCREEN_WIDTH - lineBox.w) / 2;
    SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
    SDL_DestroyTexture(textTexture);
    //
    button.y = 10 * height;
    SDL_RenderDrawRect(all->renderer, &button);
    textSurf = TTF_RenderText_Solid(all->genFont, "Exit", { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    lineBox = button;
    lineBox.w = 5 * textWidth;
    lineBox.x = (SCREEN_WIDTH - lineBox.w) / 2;
    SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
    SDL_DestroyTexture(textTexture);
    //
    break;
  case 1:
    height = SCREEN_HEIGHT / 13;
    button.h = height;
    button.y = 2 * height;
    SDL_RenderDrawRect(all->renderer, &button);
    textSurf = TTF_RenderText_Solid(all->genFont, "New Game", { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    lineBox = button;
    lineBox.w = 9 * textWidth;
    lineBox.x = (SCREEN_WIDTH - lineBox.w) / 2;
    SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
    SDL_DestroyTexture(textTexture);
    SDL_RenderDrawRect(all->renderer, &button);
    //
    button.y = 4 * height;
    SDL_RenderDrawRect(all->renderer, &button);
    textSurf = TTF_RenderText_Solid(all->genFont, "Continue", { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    lineBox = button;
    lineBox.w = 9 * textWidth;
    lineBox.x = (SCREEN_WIDTH - lineBox.w) / 2;
    SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
    SDL_DestroyTexture(textTexture);
    //
    button.y = 6 * height;
    SDL_RenderDrawRect(all->renderer, &button);
    textSurf = TTF_RenderText_Solid(all->genFont, "Save", { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    lineBox = button;
    lineBox.w = 5 * textWidth;
    lineBox.x = (SCREEN_WIDTH - lineBox.w) / 2;
    SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
    SDL_DestroyTexture(textTexture);
    //
    button.y = 8 * height;
    SDL_RenderDrawRect(all->renderer, &button);
    textSurf = TTF_RenderText_Solid(all->genFont, "Load", { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    lineBox = button;
    lineBox.w = 5 * textWidth;
    lineBox.x = (SCREEN_WIDTH - lineBox.w) / 2;
    SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
    SDL_DestroyTexture(textTexture);
    //
    button.y = 10 * height;
    SDL_RenderDrawRect(all->renderer, &button);
    textSurf = TTF_RenderText_Solid(all->genFont, "Exit", { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    lineBox = button;
    lineBox.w = 5 * textWidth;
    lineBox.x = (SCREEN_WIDTH - lineBox.w) / 2;
    SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
    SDL_DestroyTexture(textTexture);
    //
    break;
  case 2:
    height = SCREEN_HEIGHT / 15;
    button.h = height;
    button.y = 2 * height;
    SDL_RenderDrawRect(all->renderer, &button);
    if (all->saveSlots[1] == false)
      textSurf = TTF_RenderText_Solid(all->genFont, "Slot 1", { 150, 150, 150 });
    else
      textSurf = TTF_RenderText_Solid(all->genFont, "Slot 1", { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    lineBox = button;
    lineBox.w = 7 * textWidth;
    lineBox.x = (SCREEN_WIDTH - lineBox.w) / 2;
    SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
    SDL_DestroyTexture(textTexture);
    //
    button.y = 4 * height;
    SDL_RenderDrawRect(all->renderer, &button);
    if (all->saveSlots[2] == false)
      textSurf = TTF_RenderText_Solid(all->genFont, "Slot 2", { 150, 150, 150 });
    else
      textSurf = TTF_RenderText_Solid(all->genFont, "Slot 2", { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    lineBox = button;
    lineBox.w = 7 * textWidth;
    lineBox.x = (SCREEN_WIDTH - lineBox.w) / 2;
    SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
    SDL_DestroyTexture(textTexture);
    //
    button.y = 6 * height;
    SDL_RenderDrawRect(all->renderer, &button);
    if (all->saveSlots[3] == false)
      textSurf = TTF_RenderText_Solid(all->genFont, "Slot 3", { 150, 150, 150 });
    else
      textSurf = TTF_RenderText_Solid(all->genFont, "Slot 3", { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    lineBox = button;
    lineBox.w = 7 * textWidth;
    lineBox.x = (SCREEN_WIDTH - lineBox.w) / 2;
    SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
    SDL_DestroyTexture(textTexture);
    //
    button.y = 8 * height;
    SDL_RenderDrawRect(all->renderer, &button);
    if (all->saveSlots[4] == false)
      textSurf = TTF_RenderText_Solid(all->genFont, "Slot 4", { 150, 150, 150 });
    else
      textSurf = TTF_RenderText_Solid(all->genFont, "Slot 4", { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    lineBox = button;
    lineBox.w = 7 * textWidth;
    lineBox.x = (SCREEN_WIDTH - lineBox.w) / 2;
    SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
    SDL_DestroyTexture(textTexture);
    //
    button.y = 10 * height;
    SDL_RenderDrawRect(all->renderer, &button);
    if (all->saveSlots[5] == false)
      textSurf = TTF_RenderText_Solid(all->genFont, "Slot 5", { 150, 150, 150 });
    else
      textSurf = TTF_RenderText_Solid(all->genFont, "Slot 5", { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    lineBox = button;
    lineBox.w = 7 * textWidth;
    lineBox.x = (SCREEN_WIDTH - lineBox.w) / 2;
    SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
    SDL_DestroyTexture(textTexture);
    //
    button.y = 12 * height;
    SDL_RenderDrawRect(all->renderer, &button);
    textSurf = TTF_RenderText_Solid(all->genFont, "Back", { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    lineBox = button;
    lineBox.w = 5 * textWidth;
    lineBox.x = (SCREEN_WIDTH - lineBox.w) / 2;
    SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
    SDL_DestroyTexture(textTexture);
    //
    break;
  case 3:
    height = SCREEN_HEIGHT / 17;
    button.h = height;
    button.y = 2 * height;
    SDL_RenderDrawRect(all->renderer, &button);
    if (all->saveSlots[0] == false)
      textSurf = TTF_RenderText_Solid(all->genFont, "Autosave", { 150, 150, 150 });
    else
      textSurf = TTF_RenderText_Solid(all->genFont, "Autosave", { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    lineBox = button;
    lineBox.w = 9 * textWidth;
    lineBox.x = (SCREEN_WIDTH - lineBox.w) / 2;
    SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
    SDL_DestroyTexture(textTexture);
    //
    button.y = 4 * height;
    SDL_RenderDrawRect(all->renderer, &button);
    if (all->saveSlots[1] == false)
      textSurf = TTF_RenderText_Solid(all->genFont, "Slot 1", { 150, 150, 150 });
    else
      textSurf = TTF_RenderText_Solid(all->genFont, "Slot 1", { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    lineBox = button;
    lineBox.w = 7 * textWidth;
    lineBox.x = (SCREEN_WIDTH - lineBox.w) / 2;
    SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
    SDL_DestroyTexture(textTexture);
    //
    button.y = 6 * height;
    SDL_RenderDrawRect(all->renderer, &button);
    if (all->saveSlots[2] == false)
      textSurf = TTF_RenderText_Solid(all->genFont, "Slot 2", { 150, 150, 150 });
    else
      textSurf = TTF_RenderText_Solid(all->genFont, "Slot 2", { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    lineBox = button;
    lineBox.w = 7 * textWidth;
    lineBox.x = (SCREEN_WIDTH - lineBox.w) / 2;
    SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
    SDL_DestroyTexture(textTexture);
    //
    button.y = 8 * height;
    SDL_RenderDrawRect(all->renderer, &button);
    if (all->saveSlots[3] == false)
      textSurf = TTF_RenderText_Solid(all->genFont, "Slot 3", { 150, 150, 150 });
    else
      textSurf = TTF_RenderText_Solid(all->genFont, "Slot 3", { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    lineBox = button;
    lineBox.w = 7 * textWidth;
    lineBox.x = (SCREEN_WIDTH - lineBox.w) / 2;
    SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
    SDL_DestroyTexture(textTexture);
    //
    button.y = 10 * height;
    SDL_RenderDrawRect(all->renderer, &button);
    if (all->saveSlots[4] == false)
      textSurf = TTF_RenderText_Solid(all->genFont, "Slot 4", { 150, 150, 150 });
    else
      textSurf = TTF_RenderText_Solid(all->genFont, "Slot 4", { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    lineBox = button;
    lineBox.w = 7 * textWidth;
    lineBox.x = (SCREEN_WIDTH - lineBox.w) / 2;
    SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
    SDL_DestroyTexture(textTexture);
    //
    button.y = 12 * height;
    SDL_RenderDrawRect(all->renderer, &button);
    if (all->saveSlots[5] == false)
      textSurf = TTF_RenderText_Solid(all->genFont, "Slot 5", { 150, 150, 150 });
    else
      textSurf = TTF_RenderText_Solid(all->genFont, "Slot 5", { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    lineBox = button;
    lineBox.w = 7 * textWidth;
    lineBox.x = (SCREEN_WIDTH - lineBox.w) / 2;
    SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
    SDL_DestroyTexture(textTexture);
    //
    button.y = 14 * height;
    SDL_RenderDrawRect(all->renderer, &button);
    textSurf = TTF_RenderText_Solid(all->genFont, "Back", { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    lineBox = button;
    lineBox.w = 5 * textWidth;
    lineBox.x = (SCREEN_WIDTH - lineBox.w) / 2;
    SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
    SDL_DestroyTexture(textTexture);
    //
    break;
#pragma endregion
  case 4:
  {
    button = { SCREEN_WIDTH, 0, 0, textHeight };
    textSurf = TTF_RenderText_Solid(all->genFont, all->currInterface->name, { (Uint8)all->currInterface->rcol, (Uint8)all->currInterface->gcol, (Uint8)all->currInterface->bcol });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    button.w = strlen(all->currInterface->name) * textWidthNormal;
    button.x = (SCREEN_WIDTH - button.w) / 2;
    SDL_RenderCopy(all->renderer, textTexture, NULL, &button);
    SDL_DestroyTexture(textTexture);
    //
    button = { SCREEN_WIDTH - 30, 0, 30, 30 };
    SDL_SetRenderDrawColor(all->renderer, 255, 0, 0, 0);
    SDL_RenderFillRect(all->renderer, &button);
    if (all->currInterface->trader == false)
    {
      button = { 30, 30, 200, 50 };
      currQuest = all->quests;
      if (currQuest)
      {
        do
        {
          if ((currQuest->goal == all->currInterface) && (currQuest->stage == 1))
          {
            checkQuest = true;
            break;
          }
          currQuest = currQuest->next;
        } while ((currQuest) && (currQuest != all->quests));
      }
      if (checkQuest)
      {
        SDL_SetRenderDrawColor(all->renderer, 255, 255, 255, 0);
        textSurf = TTF_RenderText_Solid(all->genFont, "Complete Quests", { 255, 255, 255 });
      }
      else
      {
        SDL_SetRenderDrawColor(all->renderer, 155, 155, 155, 0);
        textSurf = TTF_RenderText_Solid(all->genFont, "Complete Quests", { 155, 155, 155 });
      }
      SDL_RenderDrawRect(all->renderer, &button);
      textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
      SDL_FreeSurface(textSurf);
      lineBox = button;
      lineBox.w = 16 * textWidthNormal;
      lineBox.x += (button.w - lineBox.w) / 2;
      SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
      SDL_DestroyTexture(textTexture);
      //
      currQuest = all->currInterface->currQuest;
      textBox = { 0, 200, SCREEN_WIDTH - 550, SCREEN_HEIGHT - 200 };
      if (all->flags.questChange)
      {
        all->flags.questChange = false;
        if (all->questTexturePlanet)
        {
          SDL_DestroyTexture(all->questTexturePlanet);
          all->questTexturePlanet = NULL;
        }
        if ((currQuest) && (currQuest->stage == 0))
        {
          SDL_SetRenderDrawColor(all->renderer, 255, 255, 255, 0);
          SDL_RenderDrawRect(all->renderer, &textBox);
          textBox.h = (currQuest->textLines + 2) * textHeight;
          all->questTexturePlanet = SDL_CreateTexture(all->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, textBox.w, textBox.h);
          SDL_SetRenderTarget(all->renderer, all->questTexturePlanet);
          textBoxSecondary = textBox;
          textBoxSecondary.y = 0;
          SDL_SetRenderDrawColor(all->renderer, 0, 0, 0, 0);
          SDL_RenderFillRect(all->renderer, &textBoxSecondary);
          lineBox = {};
          lineBox.x = 5;
          lineBox.h = textHeight;
          charPtr = currQuest->text;
          for (int i = 0; i < currQuest->textLines; ++i)
          {
            tmpPtr = strchr(charPtr, '\n');
            if (tmpPtr)
            {
              lineBox.w = tmpPtr - charPtr;
              memcpy_s(text, lineBox.w, charPtr, lineBox.w);
              text[lineBox.w] = 0;
              charPtr = tmpPtr + 1;
            }
            else
            {
              lineBox.w = strlen(charPtr);
              strcpy_s(text, charPtr);
            }
            lineBox.w *= textWidthNormal;
            textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
            textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
            SDL_FreeSurface(textSurf);
            SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
            SDL_DestroyTexture(textTexture);
            lineBox.y += textHeight;
          }
          sprintf_s(text, "Reward: %.1lf", currQuest->reward);
          lineBox.w = strlen(text) * textWidthNormal;
          textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 0 });
          textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
          SDL_FreeSurface(textSurf);
          SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
          SDL_DestroyTexture(textTexture);
          lineBox.y += textHeight;
          tmpDate = all->currDate;
          tmpDate.day += currQuest->duration;
          if (tmpDate.day > 300)
          {
            tmpDate.year += tmpDate.day / 300;
            tmpDate.day = tmpDate.day % 300;
          }
          sprintf_s(text, "Finish date: %i.%i", tmpDate.day, tmpDate.year);
          lineBox.w = strlen(text) * textWidthNormal;
          textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 0 });
          textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
          SDL_FreeSurface(textSurf);
          SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
          SDL_DestroyTexture(textTexture);
          SDL_SetRenderTarget(all->renderer, NULL);
          SDL_RenderCopy(all->renderer, all->questTexturePlanet, NULL, &textBox);
        }
      }
      else
        if (currQuest)
        {
          textBox.h = (currQuest->textLines + 2) * textHeight;
          SDL_RenderCopy(all->renderer, all->questTexturePlanet, NULL, &textBox);
        }
        else
          if (all->questTexturePlanet)
          {
            SDL_DestroyTexture(all->questTexturePlanet);
            all->questTexturePlanet = NULL;
          }
      if ((currQuest) && (currQuest->stage != 1))
      {
        button = { 30, 130, 200, 50 };
        if (currQuest->stage == 0)
        {
          strcpy_s(buttonText, 11, "Take Quest");
        }
        else
          strcpy_s(buttonText, 13, "Finish Quest");
        SDL_SetRenderDrawColor(all->renderer, 255, 255, 255, 0);
        textSurf = TTF_RenderText_Solid(all->genFont, buttonText, { 255, 255, 255 });
        SDL_RenderDrawRect(all->renderer, &button);
        textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
        SDL_FreeSurface(textSurf);
        lineBox = button;
        lineBox.w = 12 * textWidthNormal;
        lineBox.x += (button.w - lineBox.w) / 2;
        SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
        SDL_DestroyTexture(textTexture);
      }
    }
    {
      lineBox = { SCREEN_WIDTH - 545, 2 * textHeight, 0, textHeight };
      sprintf_s(text, "Food: %i  Pl: %i, %.2lf/%.2lf", all->ship.foodQ, all->currInterface->foodQ, all->currInterface->foodPB, all->currInterface->foodPS);
      lineBox.w = strlen(text) * textWidthNormal;
      textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
      textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
      SDL_FreeSurface(textSurf);
      SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
      SDL_DestroyTexture(textTexture);
      //
      lineBox.y += 2 * textHeight;
      sprintf_s(text, "Meds: %i  Pl: %i, %.2lf/%.2lf", all->ship.medQ, all->currInterface->medQ, all->currInterface->medPB, all->currInterface->medPS);
      lineBox.w = strlen(text) * textWidthNormal;
      textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
      textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
      SDL_FreeSurface(textSurf);
      SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
      SDL_DestroyTexture(textTexture);
      //
      lineBox.y += 2 * textHeight;
      sprintf_s(text, "Mins: %i  Pl: %i, %.2lf/%.2lf", all->ship.minQ, all->currInterface->minQ, all->currInterface->minPB, all->currInterface->minPS);
      lineBox.w = strlen(text) * textWidthNormal;
      textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
      textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
      SDL_FreeSurface(textSurf);
      SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
      SDL_DestroyTexture(textTexture);
      //
      lineBox.y += 2 * textHeight;
      sprintf_s(text, "Weap: %i  Pl: %i, %.2lf/%.2lf", all->ship.weapQ, all->currInterface->weapQ, all->currInterface->weapPB, all->currInterface->weapPS);
      lineBox.w = strlen(text) * textWidthNormal;
      textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
      textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
      SDL_FreeSurface(textSurf);
      SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
      SDL_DestroyTexture(textTexture);
      //
      lineBox.y += 2 * textHeight;
      sprintf_s(text, "Tech: %i  Pl: %i, %.2lf/%.2lf", all->ship.techQ, all->currInterface->techQ, all->currInterface->techPB, all->currInterface->techPS);
      lineBox.w = strlen(text) * textWidthNormal;
      textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
      textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
      SDL_FreeSurface(textSurf);
      SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
      SDL_DestroyTexture(textTexture);
      //
      lineBox.y += 2 * textHeight;
      sprintf_s(text, "Fuel: %.2f%%  Price: %.2lf", all->ship.fuel * 100, all->currInterface->fuelPrice);
      lineBox.w = strlen(text) * textWidthNormal;
      textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
      textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
      SDL_FreeSurface(textSurf);
      SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
      SDL_DestroyTexture(textTexture);
      //
      lineBox.y += 3 * textHeight;
      sprintf_s(text, "Credits: %.2f", all->ship.credits);
      lineBox.w = strlen(text) * textWidthNormal;
      textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
      textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
      SDL_FreeSurface(textSurf);
      SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
      SDL_DestroyTexture(textTexture);
      //
      lineBox.y += textHeight;
      sprintf_s(text, "Spc awail: %i", (int)(all->ship.maxMass - all->ship.bodyProperties.mass));
      lineBox.w = strlen(text) * textWidthNormal;
      textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
      textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
      SDL_FreeSurface(textSurf);
      SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
      SDL_DestroyTexture(textTexture);
      //
      lineBox.y += textHeight;
      sprintf_s(text, "Date: %i.%i", all->currDate.day, all->currDate.year);
      lineBox.w = strlen(text) * textWidthNormal;
      textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 0 });
      textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
      SDL_FreeSurface(textSurf);
      SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
      SDL_DestroyTexture(textTexture);
      //
      SDL_SetRenderDrawColor(all->renderer, 255, 255, 255, 0);
      button = { SCREEN_WIDTH - 60, 2 * textHeight, 30, textHeight };
      for (int i = 0; i < 5; ++i)
      {
        button.y = 2 * (i + 1) * textHeight;
        button.x = SCREEN_WIDTH - 60;
        textSurf = TTF_RenderText_Solid(all->genFont, "S", { 255, 255, 255 });
        SDL_RenderDrawRect(all->renderer, &button);
        textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
        SDL_FreeSurface(textSurf);
        SDL_RenderCopy(all->renderer, textTexture, NULL, &button);
        SDL_DestroyTexture(textTexture);
        button.x += 30;
        textSurf = TTF_RenderText_Solid(all->genFont, "B", { 255, 255, 255 });
        SDL_RenderDrawRect(all->renderer, &button);
        textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
        SDL_FreeSurface(textSurf);
        SDL_RenderCopy(all->renderer, textTexture, NULL, &button);
        SDL_DestroyTexture(textTexture);
      }
      button = { SCREEN_WIDTH - 200, 13 * textHeight, 50, textHeight };
      {
        textSurf = TTF_RenderText_Solid(all->genFont, "10%", { 255, 255, 255 });
        SDL_RenderDrawRect(all->renderer, &button);
        textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
        SDL_FreeSurface(textSurf);
        SDL_RenderCopy(all->renderer, textTexture, NULL, &button);
        SDL_DestroyTexture(textTexture);
        button.x += 50;
        textSurf = TTF_RenderText_Solid(all->genFont, "25%", { 255, 255, 255 });
        SDL_RenderDrawRect(all->renderer, &button);
        textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
        SDL_FreeSurface(textSurf);
        SDL_RenderCopy(all->renderer, textTexture, NULL, &button);
        SDL_DestroyTexture(textTexture);
        button.x += 50;
        textSurf = TTF_RenderText_Solid(all->genFont, "50%", { 255, 255, 255 });
        SDL_RenderDrawRect(all->renderer, &button);
        textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
        SDL_FreeSurface(textSurf);
        SDL_RenderCopy(all->renderer, textTexture, NULL, &button);
        SDL_DestroyTexture(textTexture);
        button.x += 50;
        textSurf = TTF_RenderText_Solid(all->genFont, "100%", { 255, 255, 255 });
        SDL_RenderDrawRect(all->renderer, &button);
        textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
        SDL_FreeSurface(textSurf);
        SDL_RenderCopy(all->renderer, textTexture, NULL, &button);
        SDL_DestroyTexture(textTexture);
      }
    }
    break;
  }
  case 5:
  {
    button = { SCREEN_WIDTH, 0, 0, textHeight };
    textSurf = TTF_RenderText_Solid(all->genFont, all->currInterface->name, { (Uint8)all->currInterface->rcol, (Uint8)all->currInterface->gcol, (Uint8)all->currInterface->bcol });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    button.w = strlen(all->currInterface->name) * textWidthNormal;
    button.x = (SCREEN_WIDTH - button.w) / 2;
    SDL_RenderCopy(all->renderer, textTexture, NULL, &button);
    SDL_DestroyTexture(textTexture);
    //
    button = { SCREEN_WIDTH - 30, 0, 30, 30 };
    SDL_SetRenderDrawColor(all->renderer, 255, 0, 0, 0);
    SDL_RenderFillRect(all->renderer, &button);
    button = { 30, 30, 200, 50 };
    currQuest = all->quests;
    if (currQuest)
      do
      {
        if ((currQuest->goal == all->currInterface) && (currQuest->stage == 1))
        {
          checkQuest = true;
          break;
        }
        currQuest = currQuest->next;
      } while ((currQuest) && (currQuest != all->quests));
      if (checkQuest)
      {
        SDL_SetRenderDrawColor(all->renderer, 255, 255, 255, 0);
        textSurf = TTF_RenderText_Solid(all->genFont, "Complete Quests", { 255, 255, 255 });
      }
      else
      {
        SDL_SetRenderDrawColor(all->renderer, 155, 155, 155, 0);
        textSurf = TTF_RenderText_Solid(all->genFont, "Complete Quests", { 155, 155, 155 });
      }
      SDL_RenderDrawRect(all->renderer, &button);
      textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
      SDL_FreeSurface(textSurf);
      lineBox = button;
      lineBox.w = 16 * textWidthNormal;
      lineBox.x += (button.w - lineBox.w) / 2;
      SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
      SDL_DestroyTexture(textTexture);
      //
      button = { 30, 130, 200, 50 };
      if ((all->currDate.year * 300 + all->currDate.day > all->currInterface->lastScan.year * 300 + all->currInterface->lastScan.day + 2) && (all->currInterface->scansAvail))
      {
        SDL_SetRenderDrawColor(all->renderer, 255, 255, 255, 0);
        textSurf = TTF_RenderText_Solid(all->genFont, "Scan planet", { 255, 255, 255 });
      }
      else
      {
        SDL_SetRenderDrawColor(all->renderer, 155, 155, 155, 0);
        textSurf = TTF_RenderText_Solid(all->genFont, "Scan planet", { 155, 155, 155 });
      }
      SDL_RenderDrawRect(all->renderer, &button);
      textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
      SDL_FreeSurface(textSurf);
      lineBox = button;
      lineBox.w = 12 * textWidthNormal;
      lineBox.x += (button.w - lineBox.w) / 2;
      SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
      SDL_DestroyTexture(textTexture);
      {
        lineBox = { SCREEN_WIDTH - 495, 2 * textHeight, 0, textHeight };
        sprintf_s(text, "Food: %i  Pl: %i", all->ship.foodQ, all->currInterface->foodQ);
        lineBox.w = strlen(text) * textWidthNormal;
        textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
        textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
        SDL_FreeSurface(textSurf);
        SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
        SDL_DestroyTexture(textTexture);
        //
        lineBox.y += 2 * textHeight;
        sprintf_s(text, "Meds: %i  Pl: %i", all->ship.medQ, all->currInterface->medQ);
        lineBox.w = strlen(text) * textWidthNormal;
        textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
        textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
        SDL_FreeSurface(textSurf);
        SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
        SDL_DestroyTexture(textTexture);
        //
        lineBox.y += 2 * textHeight;
        sprintf_s(text, "Mins: %i  Pl: %i", all->ship.minQ, all->currInterface->minQ);
        lineBox.w = strlen(text) * textWidthNormal;
        textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
        textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
        SDL_FreeSurface(textSurf);
        SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
        SDL_DestroyTexture(textTexture);
        //
        lineBox.y += 2 * textHeight;
        sprintf_s(text, "Weap: %i  Pl: %i", all->ship.weapQ, all->currInterface->weapQ);
        lineBox.w = strlen(text) * textWidthNormal;
        textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
        textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
        SDL_FreeSurface(textSurf);
        SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
        SDL_DestroyTexture(textTexture);
        //
        lineBox.y += 2 * textHeight;
        sprintf_s(text, "Tech: %i  Pl: %i", all->ship.techQ, all->currInterface->techQ);
        lineBox.w = strlen(text) * textWidthNormal;
        textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
        textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
        SDL_FreeSurface(textSurf);
        SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
        SDL_DestroyTexture(textTexture);
        //
        lineBox.y += 2 * textHeight;
        sprintf_s(text, "Fuel: %.2f%%", all->ship.fuel * 100);
        lineBox.w = strlen(text) * textWidthNormal;
        textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
        textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
        SDL_FreeSurface(textSurf);
        SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
        SDL_DestroyTexture(textTexture);
        //
        lineBox.y += 2 * textHeight;
        sprintf_s(text, "Credits: %.2f", all->ship.credits);
        lineBox.w = strlen(text) * textWidthNormal;
        textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
        textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
        SDL_FreeSurface(textSurf);
        SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
        SDL_DestroyTexture(textTexture);
        //
        lineBox.y += textHeight;
        sprintf_s(text, "Spc awail: %i", (int)(all->ship.maxMass - all->ship.bodyProperties.mass));
        lineBox.w = strlen(text) * textWidthNormal;
        textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
        textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
        SDL_FreeSurface(textSurf);
        SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
        SDL_DestroyTexture(textTexture);
        //
        lineBox.y += textHeight;
        sprintf_s(text, "Date: %i.%i", all->currDate.day, all->currDate.year);
        lineBox.w = strlen(text) * textWidthNormal;
        textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 0 });
        textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
        SDL_FreeSurface(textSurf);
        SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
        SDL_DestroyTexture(textTexture);
        SDL_SetRenderDrawColor(all->renderer, 255, 255, 255, 0);
        button = { SCREEN_WIDTH - 60, 2 * textHeight, 30, textHeight };
        for (int i = 0; i < 5; ++i)
        {
          button.y = 2 * (i + 1) * textHeight;
          button.x = SCREEN_WIDTH - 60;
          textSurf = TTF_RenderText_Solid(all->genFont, "D", { 255, 255, 255 });
          SDL_RenderDrawRect(all->renderer, &button);
          textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
          SDL_FreeSurface(textSurf);
          SDL_RenderCopy(all->renderer, textTexture, NULL, &button);
          SDL_DestroyTexture(textTexture);
          button.x += 30;
          textSurf = TTF_RenderText_Solid(all->genFont, "T", { 255, 255, 255 });
          SDL_RenderDrawRect(all->renderer, &button);
          textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
          SDL_FreeSurface(textSurf);
          SDL_RenderCopy(all->renderer, textTexture, NULL, &button);
          SDL_DestroyTexture(textTexture);
        }
      }
      break;
  }
  case 6:
  {
    textBox.x = 50;
    textBox.y = 50;
    textBox.w = SCREEN_WIDTH - 100;
    textBox.h = SCREEN_HEIGHT - 200;
    scrollBar.w = 50;
    scrollBar.y = textBox.y;
    scrollBar.x = textBox.w - scrollBar.w;
    scrollBar.h = textBox.h;
    button.y = SCREEN_HEIGHT - 100;
    button.w = 100;
    button.x = (SCREEN_WIDTH - button.w) / 2;
    button.h = 50;
    textureBox = {};
    textureBox.w = textBox.w - 50;
    textureBox.h = textHeight * all->currEvent.textLines;
    if (!all->eventTexture)
    {
      all->eventTexture = SDL_CreateTexture(all->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, textureBox.w, textureBox.h);
      SDL_SetRenderTarget(all->renderer, all->eventTexture);
      SDL_SetRenderDrawColor(all->renderer, 0, 0, 0, 0);
      SDL_RenderFillRect(all->renderer, &textureBox);
      lineBox = {};
      lineBox.x = 5;
      lineBox.h = textHeight;
      charPtr = all->currEvent.text;
      for (int i = 0; i < all->currEvent.textLines; ++i)
      {
        tmpPtr = strchr(charPtr, '\n');
        if (tmpPtr)
        {
          lineBox.w = tmpPtr - charPtr;
          memcpy_s(text, lineBox.w, charPtr, lineBox.w);
          text[lineBox.w] = 0;
          charPtr = tmpPtr + 1;
        }
        else
        {
          lineBox.w = strlen(charPtr);
          strcpy_s(text, charPtr);
        }
        lineBox.w *= textWidthNormal;
        textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
        textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
        SDL_FreeSurface(textSurf);
        SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
        SDL_DestroyTexture(textTexture);
        lineBox.y += textHeight;
      }
      SDL_SetRenderTarget(all->renderer, NULL);
      textBoxSecondary = textBox;
      if (textureBox.h <= textBox.h)
        textBoxSecondary.h = textureBox.h;
      else
      {

      }
      SDL_RenderCopy(all->renderer, all->eventTexture, &textureBox, &textBoxSecondary);
    }
    else
    {
      textBoxSecondary = textBox;
      if (textureBox.h <= textBox.h)
        textBoxSecondary.h = textureBox.h;
      else
      {

      }
      SDL_RenderCopy(all->renderer, all->eventTexture, &textureBox, &textBoxSecondary);
    }
    SDL_SetRenderDrawColor(all->renderer, 255, 255, 255, 0);
    SDL_RenderDrawRect(all->renderer, &textBox);
    SDL_RenderDrawRect(all->renderer, &button);
    strcpy_s(buttonText, 3, "OK");
    textSurf = TTF_RenderText_Solid(all->genFont, buttonText, { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    lineBox = button;
    lineBox.w = 3 * textWidth;
    lineBox.x = (SCREEN_WIDTH - lineBox.w) / 2;
    SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
    SDL_DestroyTexture(textTexture);
    break;
  }
  case 7:
  {
    button = { SCREEN_WIDTH - 30, 0, 30, 30 };
    SDL_SetRenderDrawColor(all->renderer, 255, 0, 0, 0);
    SDL_RenderFillRect(all->renderer, &button);
    button = { SCREEN_WIDTH / 2 - 250, SCREEN_HEIGHT / 2 - 100, 500, 200 };
    if (all->ship.credits >= all->currInterface->fuelPrice)
    {
      SDL_SetRenderDrawColor(all->renderer, 255, 255, 255, 0);
      textSurf = TTF_RenderText_Solid(all->genFont, "Pay Fee", { 255, 255, 255 });
    }
    else
    {
      SDL_SetRenderDrawColor(all->renderer, 155, 155, 155, 0);
      textSurf = TTF_RenderText_Solid(all->genFont, "Pay Fee", { 155, 155, 155 });
    }
    SDL_RenderDrawRect(all->renderer, &button);
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    lineBox = button;
    lineBox.w = 8 * textWidth;
    lineBox.x = (SCREEN_WIDTH - lineBox.w) / 2;
    SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
    SDL_DestroyTexture(textTexture);
    break;
  }
  case 8:
  {
    if (all->flags.scroll == 2)
    {
      if (all->flags.shiftModifier)
      {
        if (all->flags.scrollPos < all->flags.textLinesGlobal - SCREEN_HEIGHT / textHeight)
          all->flags.scrollPos += 5;
        if (all->flags.scrollPos > all->flags.textLinesGlobal - SCREEN_HEIGHT / textHeight)
          all->flags.scrollPos = all->flags.textLinesGlobal - SCREEN_HEIGHT / textHeight;
      }
      else
        if (all->flags.scrollPos < all->flags.textLinesGlobal - SCREEN_HEIGHT / textHeight)
          ++all->flags.scrollPos;
    }
    SDL_SetRenderDrawColor(all->renderer, 255, 255, 255, 0);
    scrollBar = { SCREEN_WIDTH - 330, 0, 30, SCREEN_HEIGHT };
    SDL_RenderDrawRect(all->renderer, &scrollBar);
    textureBox = { 0, 0, SCREEN_WIDTH - 330, 0 };
    textBox = { 0, 0, SCREEN_WIDTH - 335, SCREEN_HEIGHT };
    if (all->flags.questChange)
    {
      all->flags.questChange = false;
      all->flags.textLinesGlobal = 0;
      if (all->questsTextureGlobal)
      {
        SDL_DestroyTexture(all->questsTextureGlobal);
        all->questsTextureGlobal = NULL;
      }
      if (all->quests)
      {
        do
        {
          if ((currQuest->stage == 1) || (currQuest->stage == 2))
            all->flags.textLinesGlobal += currQuest->textLinesWide + 2;
          currQuest = currQuest->next;
        } while ((currQuest) && (currQuest != all->quests));
        textureBox.h = all->flags.textLinesGlobal * textHeight;
        all->flags.textLinesGlobal = 0;
        all->questsTextureGlobal = SDL_CreateTexture(all->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, textureBox.w, textureBox.h);
        SDL_SetRenderTarget(all->renderer, all->questsTextureGlobal);
        SDL_SetRenderDrawColor(all->renderer, 0, 0, 0, 0);
        SDL_RenderFillRect(all->renderer, &textureBox);
        lineBox = {};
        lineBox.x = 5;
        lineBox.h = textHeight;
        currQuest = all->quests;
        do
        {
          if ((currQuest->stage == 1) || (currQuest->stage == 2))
          {
            charPtr = currQuest->textWide;
            for (int i = 0; i < currQuest->textLinesWide; ++i)
            {
              tmpPtr = strchr(charPtr, '\n');
              if (tmpPtr)
              {
                lineBox.w = tmpPtr - charPtr;
                memcpy_s(text, lineBox.w, charPtr, lineBox.w);
                text[lineBox.w] = 0;
                charPtr = tmpPtr + 1;
              }
              else
              {
                lineBox.w = strlen(charPtr);
                strcpy_s(text, charPtr);
              }
              lineBox.w *= textWidthNormal;
              textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
              textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
              SDL_FreeSurface(textSurf);
              SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
              SDL_DestroyTexture(textTexture);
              lineBox.y += textHeight;
              ++all->flags.textLinesGlobal;
            }
            if (currQuest->type != 4)
            {
              sprintf_s(text, "Reward: %.1lf. Finish date: %i.%i", currQuest->reward, currQuest->end.day, currQuest->end.year);
              lineBox.w = strlen(text) * textWidthNormal;
              textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 0 });
              textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
              SDL_FreeSurface(textSurf);
              SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
              SDL_DestroyTexture(textTexture);
              lineBox.y += textHeight;
              ++all->flags.textLinesGlobal;
            }
            else
            {
              lineBox.w = 35 * textWidthNormal;
              textSurf = TTF_RenderText_Solid(all->genFont, "Reward: lots. Finish date: anytime", { 255, 255, 0 });
              textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
              SDL_FreeSurface(textSurf);
              SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
              SDL_DestroyTexture(textTexture);
              lineBox.y += textHeight;
              ++all->flags.textLinesGlobal;
            }
            if (currQuest->stage == 1)
            {
              textSurf = TTF_RenderText_Solid(all->genFont, "Status: in progress", { 255, 255, 0 });
              lineBox.w = 20 * textWidthNormal;
            }
            else
            {
              textSurf = TTF_RenderText_Solid(all->genFont, "Status: return for reward", { 0, 200, 0 });
              lineBox.w = 26 * textWidthNormal;
            }
            textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
            SDL_FreeSurface(textSurf);
            SDL_RenderCopy(all->renderer, textTexture, NULL, &lineBox);
            SDL_DestroyTexture(textTexture);
            lineBox.y += textHeight;
            ++all->flags.textLinesGlobal;
          }
          currQuest = currQuest->next;
        } while ((currQuest) && (currQuest != all->quests));
        SDL_SetRenderTarget(all->renderer, NULL);
        if (textureBox.h <= textBox.h)
          textBox.h = textureBox.h;
        else
        {
          textureBox.h = textBox.h;
          textureBox.y = all->flags.scrollPos * textHeight;
        }
        SDL_RenderCopy(all->renderer, all->questsTextureGlobal, &textureBox, &textBox);
      }
    }
    else
      if (all->questsTextureGlobal)
      {
        textureBox.h = textHeight * all->flags.textLinesGlobal;
        if (textureBox.h <= textBox.h)
          textBox.h = textureBox.h;
        else
        {
          textureBox.h = textBox.h;
          textureBox.y = all->flags.scrollPos * textHeight;
        }
        SDL_RenderCopy(all->renderer, all->questsTextureGlobal, &textureBox, &textBox);
      }
    if (all->flags.textLinesGlobal > SCREEN_HEIGHT / textHeight)
    {
      scrollGuide = scrollBar;
      scrollGuide.h = SCREEN_HEIGHT * (SCREEN_HEIGHT - 60) / (textHeight * all->flags.textLinesGlobal);
      scrollGuide.y = 30 + all->flags.scrollPos * (SCREEN_HEIGHT - 60 - scrollGuide.h) / (all->flags.textLinesGlobal - (SCREEN_HEIGHT - 60) / textHeight - 1);
      SDL_SetRenderDrawColor(all->renderer, 155, 155, 155, 0);
      SDL_RenderFillRect(all->renderer, &scrollGuide);
      SDL_SetRenderDrawColor(all->renderer, 0, 255, 0, 0);
      scrollBar.h = 30;
      SDL_RenderFillRect(all->renderer, &scrollBar);
      scrollBar.y = SCREEN_HEIGHT - 30;
      SDL_RenderFillRect(all->renderer, &scrollBar);
    }
    button = { SCREEN_WIDTH - 30, 0, 30, 30 };
    SDL_SetRenderDrawColor(all->renderer, 255, 0, 0, 0);
    SDL_RenderFillRect(all->renderer, &button);
    {
      textBox = { SCREEN_WIDTH - 295, textHeight, 0, textHeight };
      sprintf_s(text, "Food: %i", all->ship.foodQ);
      textBox.w = strlen(text) * textWidthNormal;
      textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
      textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
      SDL_FreeSurface(textSurf);
      SDL_RenderCopy(all->renderer, textTexture, NULL, &textBox);
      SDL_DestroyTexture(textTexture);
      //
      textBox.y += textHeight;
      sprintf_s(text, "Meds: %i", all->ship.medQ);
      textBox.w = strlen(text) * textWidthNormal;
      textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
      textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
      SDL_FreeSurface(textSurf);
      SDL_RenderCopy(all->renderer, textTexture, NULL, &textBox);
      SDL_DestroyTexture(textTexture);
      //
      textBox.y += textHeight;
      sprintf_s(text, "Mins: %i", all->ship.minQ);
      textBox.w = strlen(text) * textWidthNormal;
      textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
      textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
      SDL_FreeSurface(textSurf);
      SDL_RenderCopy(all->renderer, textTexture, NULL, &textBox);
      SDL_DestroyTexture(textTexture);
      //
      textBox.y += textHeight;
      sprintf_s(text, "Weap: %i", all->ship.weapQ);
      textBox.w = strlen(text) * textWidthNormal;
      textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
      textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
      SDL_FreeSurface(textSurf);
      SDL_RenderCopy(all->renderer, textTexture, NULL, &textBox);
      SDL_DestroyTexture(textTexture);
      //
      textBox.y += textHeight;
      sprintf_s(text, "Tech: %i", all->ship.techQ);
      textBox.w = strlen(text) * textWidthNormal;
      textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
      textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
      SDL_FreeSurface(textSurf);
      SDL_RenderCopy(all->renderer, textTexture, NULL, &textBox);
      SDL_DestroyTexture(textTexture);
      //
      textBox.y += textHeight;
      sprintf_s(text, "Fuel: %.2f %%", all->ship.fuel * 100);
      textBox.w = strlen(text) * textWidthNormal;
      textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
      textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
      SDL_FreeSurface(textSurf);
      SDL_RenderCopy(all->renderer, textTexture, NULL, &textBox);
      SDL_DestroyTexture(textTexture);
      //
      textBox.y += textHeight;
      sprintf_s(text, "Credits: %.2f", all->ship.credits);
      textBox.w = strlen(text) * textWidthNormal;
      textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
      textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
      SDL_FreeSurface(textSurf);
      SDL_RenderCopy(all->renderer, textTexture, NULL, &textBox);
      SDL_DestroyTexture(textTexture);
      //
      textBox.y += textHeight;
      sprintf_s(text, "Spc awail: %i", (int)(all->ship.maxMass - all->ship.bodyProperties.mass));
      textBox.w = strlen(text) * textWidthNormal;
      textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
      textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
      SDL_FreeSurface(textSurf);
      SDL_RenderCopy(all->renderer, textTexture, NULL, &textBox);
      SDL_DestroyTexture(textTexture);
      //
      textBox.y += textHeight;
      sprintf_s(text, "Date: %i.%i", all->currDate.day, all->currDate.year);
      textBox.w = strlen(text) * textWidthNormal;
      textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 0 });
      textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
      SDL_FreeSurface(textSurf);
      SDL_RenderCopy(all->renderer, textTexture, NULL, &textBox);
      SDL_DestroyTexture(textTexture);
    }
    break;
  }
  case 9:
  {
    button = { SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT / 2 - 100, 400, 200 };
    textSurf = TTF_RenderText_Solid(all->genFont, "VICTORY", { 155, 0, 0 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    SDL_RenderCopy(all->renderer, textTexture, NULL, &button);
    SDL_DestroyTexture(textTexture);
    break;
  }
  }
  all->flags.scroll = 0;
  return;
}

/* Modes
 0 - targeted
 1 - passive
 */
void _DisplayInfo(globals *all, int mode)
{
  const int textHeight = 22, symbolWidth = 11;
  SDL_Surface *textSurf = NULL;
  SDL_Texture *textTexture = NULL;
  int textLength = 0;
  SDL_Rect info = { 0, 0, 0, 0 }, textBox = { 0, 0, 0, 0 };
  planet_t *source;
  char text[50];
  info.h = SCREEN_HEIGHT - 300;
  info.w = (SCREEN_WIDTH - SCREEN_HEIGHT) / 2;
  if (mode == 0)
  {
    source = all->infoTarget;
    if (all->flags.infoChange == false)
    {
      if (!((all->flags.mapMode) || (all->mousePos.x > SCREEN_WIDTH / 2)))
      {
        info.x = (SCREEN_WIDTH + SCREEN_HEIGHT) / 2;
      }
      SDL_RenderCopy(all->renderer, all->info, NULL, &info);
      return;
    }
    SDL_DestroyTexture(all->info);
    all->info = SDL_CreateTexture(all->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, info.w, info.h);
    SDL_SetRenderTarget(all->renderer, all->info);
  }
  else
  {
    source = all->passiveInfoTarg;
    if ((all->flags.passInfChange == false) && (all->passiveInfo))
    {
      info.x = (SCREEN_WIDTH + SCREEN_HEIGHT) / 2;
      SDL_RenderCopy(all->renderer, all->passiveInfo, NULL, &info);
      return;
    }
    SDL_DestroyTexture(all->passiveInfo);
    all->passiveInfo = SDL_CreateTexture(all->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, info.w, info.h);
    SDL_SetRenderTarget(all->renderer, all->passiveInfo);
  }
  SDL_SetRenderDrawColor(all->renderer, 0, 0, 0, 0);
  SDL_RenderFillRect(all->renderer, &info);
  SDL_SetRenderDrawColor(all->renderer, 200, 200, 200, 0);
  SDL_RenderDrawRect(all->renderer, &info);
  textBox.h = textHeight;
  textBox.x = 5;
  if (source == NULL)
    if (mode == 1)
    {
      strcpy_s(text, 12, "Empty Space");
      textLength = 12;
      textBox.w = textLength * 12;
      textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 100 });
      textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
      SDL_FreeSurface(textSurf);
      SDL_RenderCopy(all->renderer, textTexture, NULL, &textBox);
      SDL_DestroyTexture(textTexture);
      SDL_SetRenderTarget(all->renderer, NULL);
      return;
    }
    else
    {
      SDL_DestroyTexture(all->info);
      all->info = NULL;
      SDL_SetRenderTarget(all->renderer, NULL);
      return;
    }
  textLength = strlen(source->name);
  textBox.w = textLength * symbolWidth;
  strcpy_s(text, textLength + 1, source->name);
  textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 100 });
  textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
  SDL_FreeSurface(textSurf);
  SDL_RenderCopy(all->renderer, textTexture, NULL, &textBox);
  SDL_DestroyTexture(textTexture);
  textBox.y += textHeight;
  switch (source->type)
  {
  case STAR:
    strcpy_s(text, 5, "Star");
    textLength = 5;
    break;
  case GAS_GIANT:
    strcpy_s(text, 10, "Gas Giant");
    textLength = 10;
    break;
  case UNINHABITABLE:
    strcpy_s(text, 14, "Uninhabitable");
    textLength = 14;
    break;
  case UNEXPLORABLE:
    strcpy_s(text, 13, "Unexplorable");
    textLength = 13;
    break;
  case HABITABLE:
    strcpy_s(text, 10, "Habitable");
    textLength = 10;
    break;
  case MOON:
    strcpy_s(text, 19, "Uninhabitable moon");
    textLength = 19;
    break;
  case MOON_HABITABLE:
    strcpy_s(text, 15, "Habitable moon");
    textLength = 15;
    break;
  case MOON_UNEXPLORABLE:
    strcpy_s(text, 18, "Unexplorable moon");
    textLength = 18;
    break;
  case STAR_GATE:
    strcpy_s(text, 10, "Star Gate");
    textLength = 10;
    break;
  case COMET:
    strcpy_s(text, 6, "Comet");
    textLength = 6;
    break;
  }
  textBox.w = textLength * symbolWidth;
  textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
  textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
  SDL_FreeSurface(textSurf);
  SDL_RenderCopy(all->renderer, textTexture, NULL, &textBox);
  SDL_DestroyTexture(textTexture);
  //
  textBox.y += textHeight;
  sprintf_s(text, "Mass: %.2lfN", source->bodyProperties.mass);
  textBox.w = strlen(text) * symbolWidth;
  textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
  textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
  SDL_FreeSurface(textSurf);
  SDL_RenderCopy(all->renderer, textTexture, NULL, &textBox);
  SDL_DestroyTexture(textTexture);
  //
  if (source->type == STAR_GATE)
  {
    textBox.y += textHeight;
    sprintf_s(text, "Fee: %.1lf", source->fuelPrice);
    textBox.w = strlen(text) * symbolWidth;
    textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    SDL_RenderCopy(all->renderer, textTexture, NULL, &textBox);
    SDL_DestroyTexture(textTexture);
    SDL_SetRenderTarget(all->renderer, NULL);
    return;
  }
  //
  textBox.y += textHeight;
  sprintf_s(text, "Rad: %.2lfN", source->bodyProperties.rad);
  textBox.w = strlen(text) * symbolWidth;
  textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
  textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
  SDL_FreeSurface(textSurf);
  SDL_RenderCopy(all->renderer, textTexture, NULL, &textBox);
  SDL_DestroyTexture(textTexture);
  //
  if ((source->type >= GAS_GIANT) && (source->type <= HABITABLE))
  {
    textBox.y += textHeight;
    sprintf_s(text, "Satellites: %i", source->satelliteCount);
    textBox.w = strlen(text) * symbolWidth;
    textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    SDL_RenderCopy(all->renderer, textTexture, NULL, &textBox);
    SDL_DestroyTexture(textTexture);
  }
  switch (source->type)
  {
  case HABITABLE:
  case MOON_HABITABLE:
    textBox.y += textHeight;
    switch (source->clim)
    {
    case 0:
      strcpy_s(text, 9, "Forested");
      textLength = 9;
      break;
    case 1:
      strcpy_s(text, 8, "Oceanic");
      textLength = 8;
      break;
    case 2:
      strcpy_s(text, 7, "Jungle");
      textLength = 7;
      break;
    }
    textBox.w = textLength * symbolWidth;
    textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    SDL_RenderCopy(all->renderer, textTexture, NULL, &textBox);
    SDL_DestroyTexture(textTexture);
    //
    textBox.y += textHeight;
    sprintf_s(text, "Popul: %iMil", source->pop);
    textBox.w = strlen(text) * symbolWidth;
    textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    SDL_RenderCopy(all->renderer, textTexture, NULL, &textBox);
    SDL_DestroyTexture(textTexture);
    //
    textBox.y += textHeight;
    sprintf_s(text, "Colonized in: %i", source->age);
    textBox.w = strlen(text) * symbolWidth;
    textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    SDL_RenderCopy(all->renderer, textTexture, NULL, &textBox);
    SDL_DestroyTexture(textTexture);
    //
    textBox.y += textHeight;
    switch (source->eco)
    {
    case 0:
      strcpy_s(text, 11, "Industrial");
      textLength = 11;
      break;
    case 1:
      strcpy_s(text, 10, "Farmworld");
      textLength = 10;
      break;
    case 2:
      strcpy_s(text, 10, "Mixed eco");
      textLength = 10;
      break;
    case 3:
      strcpy_s(text, 16, "Post-industrial");
      textLength = 16;
      break;
    }
    textBox.w = textLength * symbolWidth;
    textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    SDL_RenderCopy(all->renderer, textTexture, NULL, &textBox);
    SDL_DestroyTexture(textTexture);
    //
    textBox.y += textHeight;
    strcpy_s(text, 8, "Prices:");
    textLength = 8;
    textBox.w = textLength * symbolWidth - symbolWidth / 2;
    textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    SDL_RenderCopy(all->renderer, textTexture, NULL, &textBox);
    SDL_DestroyTexture(textTexture);
    //
    textBox.y += textHeight;
    sprintf_s(text, "Food: %.2f/%.2f", source->foodPB, source->foodPS);
    textBox.w = strlen(text) * symbolWidth;
    textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    SDL_RenderCopy(all->renderer, textTexture, NULL, &textBox);
    SDL_DestroyTexture(textTexture);
    //
    textBox.y += textHeight;
    sprintf_s(text, "Meds: %.2f/%.2f", source->medPB, source->medPS);
    textBox.w = strlen(text) * symbolWidth;
    textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    SDL_RenderCopy(all->renderer, textTexture, NULL, &textBox);
    SDL_DestroyTexture(textTexture);
    //
    textBox.y += textHeight;
    sprintf_s(text, "Mins: %.2f/%.2f", source->minPB, source->minPS);
    textBox.w = strlen(text) * symbolWidth;
    textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    SDL_RenderCopy(all->renderer, textTexture, NULL, &textBox);
    SDL_DestroyTexture(textTexture);
    //
    textBox.y += textHeight;
    sprintf_s(text, "Weap: %.2f/%.2f", source->weapPB, source->weapPS);
    textBox.w = strlen(text) * symbolWidth;
    textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    SDL_RenderCopy(all->renderer, textTexture, NULL, &textBox);
    SDL_DestroyTexture(textTexture);
    //
    textBox.y += textHeight;
    sprintf_s(text, "Tech: %.2f/%.2f", source->techPB, source->techPS);
    textBox.w = strlen(text) * symbolWidth;
    textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    SDL_RenderCopy(all->renderer, textTexture, NULL, &textBox);
    SDL_DestroyTexture(textTexture);
    //
    textBox.y += textHeight;
    sprintf_s(text, "Fuel: %.2f", source->fuelPrice);
    textBox.w = strlen(text) * symbolWidth;
    textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    SDL_RenderCopy(all->renderer, textTexture, NULL, &textBox);
    SDL_DestroyTexture(textTexture);
    break;
  case UNINHABITABLE:
  case MOON:
    textBox.y += textHeight;
    switch (source->clim)
    {
    case 0:
      strcpy_s(text, 10, "Dead rock");
      textLength = 10;
      break;
    case 1:
      strcpy_s(text, 7, "Barren");
      textLength = 7;
      break;
    case 2:
      strcpy_s(text, 7, "Desert");
      textLength = 7;
      break;
    }
    textBox.w = textLength * symbolWidth;
    textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    SDL_RenderCopy(all->renderer, textTexture, NULL, &textBox);
    SDL_DestroyTexture(textTexture);
    break;
  case UNEXPLORABLE:
  case MOON_UNEXPLORABLE:
    textBox.y += textHeight;
    switch (source->clim)
    {
    case 0:
      strcpy_s(text, 9, "Volcanic");
      textLength = 9;
      break;
    case 1:
      strcpy_s(text, 7, "Acidic");
      textLength = 7;
      break;
    }
    textBox.w = textLength * symbolWidth;
    textSurf = TTF_RenderText_Solid(all->genFont, text, { 255, 255, 255 });
    textTexture = SDL_CreateTextureFromSurface(all->renderer, textSurf);
    SDL_FreeSurface(textSurf);
    SDL_RenderCopy(all->renderer, textTexture, NULL, &textBox);
    SDL_DestroyTexture(textTexture);
    break;
  }
  //
  SDL_SetRenderTarget(all->renderer, NULL);
}

void _DrawPath(globals *all)
{
  SDL_Point way[2];
  double distScale = SCREEN_HEIGHT / (2 * all->ship.well->gravRadius);
  way[0].x = SCREEN_WIDTH / 2 + (int)(distScale * all->ship.bodyProperties.coord.x);
  way[0].y = SCREEN_HEIGHT / 2 + (int)(distScale * all->ship.bodyProperties.coord.y);
  way[1].x = SCREEN_WIDTH / 2 + (int)(distScale * all->ship.moveDest.x);
  way[1].y = SCREEN_HEIGHT / 2 + (int)(distScale * all->ship.moveDest.y);
  SDL_SetRenderDrawColor(all->renderer, 150, 150, 150, 0);
  SDL_RenderDrawLine(all->renderer, way[0].x, way[0].y, way[1].x, way[1].y);
  SDL_RenderDrawLine(all->renderer, way[1].x - 10, way[1].y - 10, way[1].x + 10, way[1].y + 10);
  SDL_RenderDrawLine(all->renderer, way[1].x + 10, way[1].y - 10, way[1].x - 10, way[1].y + 10);
}

void DrawScene(globals *all)
{
  SDL_SetRenderDrawColor(all->renderer, 0, 0, 0, 0);
  SDL_RenderClear(all->renderer);
  if (!all->flags.menu)
  {
    if (all->flags.mapMode != 1)
      _DrawSystem(all->renderer, *all->system, all->ship, all->timeMods);
    _ShipInterface(all);
    _MapGen(all);
    if (all->infoTarget)
      _DisplayInfo(all, 0);
    if (all->flags.mapMode == 1)
      _DisplayInfo(all, 1);
    if (all->flags.destSet)
      _DrawPath(all);
  }
  else
  {
    _DrawMenu(all);
  }
  SDL_RenderPresent(all->renderer);
}