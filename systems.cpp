#include "SDL.h"
#include "SDL_ttf.h"
#include "systems.h"
#include "physix.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <Windows.h>

void _GameStart(globals *all, int mode, int slot);
void _GenerateWorldEvent(globals *all);
void _LeftMouseClick(globals *all);
void _RightMouseClick(globals *all);
void _GenerateQuest(globals *all, planet_t *starter, int otherType, bool mainQuest);
void _ApplyDestroyEvent(globals *all);

void _DestroyQuests(globals *all)
{
  quest_t *currQuest = NULL, *tmpQuest = NULL;
  if (all->quests)
    currQuest = all->quests->next;
  else
    return;
  while ((currQuest) && (currQuest != all->quests))
  {
    free(currQuest->text);
    free(currQuest->textWide);
    tmpQuest = currQuest->next;
    free(currQuest);
    currQuest = tmpQuest;
  }
  free(all->quests->text);
  free(all->quests->textWide);
  free(all->quests);
  all->quests = NULL;
  return;
}

void _CheckQuests(globals *all)
{
  quest_t *currQuest = NULL, *tmpQuest = NULL;
  int dateComp = 0;
  currQuest = all->quests;
  do
  {
    dateComp = 0;
    if (currQuest->stage > 0)
      dateComp = (currQuest->end.year - all->currDate.year) * 300 + (currQuest->end.day - all->currDate.day);
    if (currQuest->stage == -1)
      all->ship.credits += currQuest->reward;
    if ((dateComp < 0) || (currQuest->stage == -1) || ((currQuest->type != 4) && (currQuest->starter == NULL)) || (currQuest->goal == NULL))
    {
      all->flags.questChange = true;
      free(currQuest->text);
      free(currQuest->textWide);
      currQuest->starter->currQuest = NULL;
      tmpQuest = currQuest->prev;
      if (currQuest == all->quests)
        all->quests = currQuest->next;
      if (currQuest->next)
      {
        currQuest = currQuest->next;
        free(currQuest->prev);
        if (tmpQuest != currQuest)
        {
          currQuest->prev = tmpQuest;
          tmpQuest->next = currQuest;
        }
        else
        {
          currQuest->next = NULL;
          currQuest->prev = NULL;
        }
      }
    }
    else
      currQuest = currQuest->next;
  } while ((currQuest) && (currQuest != all->quests));
  return;
}

planet_t *_FindRandomPlanet(planet_t *system, PLANET_TYPE type, planet_t *excluded)
{
  planet_t *currPlanet = NULL;
  int bodyCount = system->satelliteCount, startPoint = 0, currPoint = 0;
  PLANET_TYPE type1 = type, type2 = type;
  if (type < MOON)
    type2 = (PLANET_TYPE)(type + 3);
  else
    type2 = (PLANET_TYPE)(type - 3);
  currPlanet = system->satellites;
  for (int i = 0; i < system->satelliteCount; ++i)
  {
    bodyCount += currPlanet->satelliteCount;
    currPlanet = currPlanet->next;
  }
  currPlanet = system->satellites;
  startPoint = rand() % bodyCount;
  while (true)
  {
    if (currPoint == startPoint)
      break;
    ++currPoint;
    if (currPlanet->satellites)
      currPlanet = currPlanet->satellites;
    else
      if (currPlanet->next)
        currPlanet = currPlanet->next;
      else
        currPlanet = currPlanet->parent->next;
  }
  while (true)
  {
    if (((currPlanet->type == type1) || (currPlanet->type == type2)) && (currPlanet != excluded))
      return currPlanet;
    if (currPoint == startPoint - 1)
      break;
    ++currPoint;
    if (currPlanet->satellites)
      currPlanet = currPlanet->satellites;
    else
      if (currPlanet->next)
        currPlanet = currPlanet->next;
      else
        if (currPlanet->parent->next)
          currPlanet = currPlanet->parent->next;
        else
        {
          currPlanet = system->satellites;
          currPoint = 0;
        }
  }
  return NULL;
}

void _TextConverter(worldEvent_t *Event, int lineLength)
{
  char *tmpPtr = NULL;
  int textLength = strlen(Event->text) + 1, currPos = 0, tmpPos = 0;
  Event->textLines = 1;
  if (Event->type == 1)
  {
    tmpPtr = strstr(Event->text, "%");
    if (Event->location)
    {
      int nameLength = strlen(Event->location->name);
      memmove_s(tmpPtr + nameLength - 1, sizeof(char) * (textLength - (int)(tmpPtr - Event->text)), tmpPtr, sizeof(char) * (textLength - (int)(tmpPtr - Event->text)));
      memcpy_s(tmpPtr, sizeof(char) * nameLength, Event->location->name, sizeof(char) * nameLength);
      textLength += nameLength - 1;
    }
  }
  while (true)
  {
    if (Event->text[currPos] == 0)
    {
      Event->textLines;
      break;
    }
    currPos += lineLength;
    if (currPos > textLength)
      break;
    if (isspace(Event->text[currPos]))
      Event->text[currPos] = '\n';
    else
    {
      tmpPos = currPos;
      while (!isspace(Event->text[tmpPos]))
        --tmpPos;
      if (tmpPos - lineLength / 2 <= currPos - lineLength)
      {
        memmove_s(Event->text + currPos + 1, sizeof(char) * (textLength - currPos), Event->text + currPos, sizeof(char) * (textLength - currPos));
        Event->text[currPos] = '\n';
        ++textLength;
      }
      else
        Event->text[tmpPos] = '\n';
    }
    ++Event->textLines;
    ++currPos;
  }
  return;
}

void _TextConverter(quest_t *quest, int lineLength, int lineLengthWide)
{
  char *tmpPtr = NULL, days[6] = {};
  int textLength = strlen(quest->text) + 1, currPos = 0, tmpPos = 0;
  quest->textLines = 1;
  quest->textLinesWide = 1;
  tmpPtr = strstr(quest->text, "%s");
  if ((quest->starter) && (tmpPtr))
  {
    int nameLength = strlen(quest->starter->name);
    memmove_s(tmpPtr + nameLength - 1, sizeof(char) * (textLength - (int)(tmpPtr - quest->text)), tmpPtr + 1, sizeof(char) * (textLength - (int)(tmpPtr + 1 - quest->text)));
    memcpy_s(tmpPtr, sizeof(char) * nameLength, quest->starter->name, sizeof(char) * nameLength);
    textLength += nameLength - 2;
  }
  tmpPtr = NULL;
  tmpPtr = strstr(quest->text, "%g");
  if ((quest->goal) && (tmpPtr))
  {
    int nameLength = strlen(quest->goal->name);
    memmove_s(tmpPtr + nameLength - 1, sizeof(char) * (textLength - (int)(tmpPtr - quest->text)), tmpPtr + 1, sizeof(char) * (textLength - (int)(tmpPtr + 1 - quest->text)));
    memcpy_s(tmpPtr, sizeof(char) * nameLength, quest->goal->name, sizeof(char) * nameLength);
    textLength += nameLength - 2;
  }
  tmpPtr = NULL;
  tmpPtr = strstr(quest->text, "%d");
  if ((quest->goal) && (tmpPtr))
  {
    sprintf_s(days, "%i", quest->duration);
    int nameLength = strlen(days);
    memmove_s(tmpPtr + nameLength - 1, sizeof(char) * (textLength - (int)(tmpPtr - quest->text)), tmpPtr + 1, sizeof(char) * (textLength - (int)(tmpPtr + 1 - quest->text)));
    memcpy_s(tmpPtr, sizeof(char) * nameLength, days, sizeof(char) * nameLength);
    textLength += nameLength - 2;
  }
  strcpy_s(quest->textWide, strlen(quest->text) + 1, quest->text);
  while (true)
  {
    if (quest->text[currPos] == 0)
    {
      quest->textLines;
      break;
    }
    currPos += lineLength;
    if (currPos > textLength)
      break;
    if (isspace(quest->text[currPos]))
      quest->text[currPos] = '\n';
    else
    {
      tmpPos = currPos;
      while (!isspace(quest->text[tmpPos]))
        --tmpPos;
      if (tmpPos - lineLength / 2 <= currPos - lineLength)
      {
        memmove_s(quest->text + currPos + 1, sizeof(char) * (textLength - currPos), quest->text + currPos, sizeof(char) * (textLength - currPos));
        quest->text[currPos] = '\n';
        ++textLength;
      }
      else
        quest->text[tmpPos] = '\n';
    }
    ++quest->textLines;
    ++currPos;
  }
  currPos = 0;
  while (true)
  {
    if (quest->textWide[currPos] == 0)
    {
      quest->textLinesWide;
      break;
    }
    currPos += lineLengthWide;
    if (currPos > textLength)
      break;
    if (isspace(quest->textWide[currPos]))
      quest->textWide[currPos] = '\n';
    else
    {
      tmpPos = currPos;
      while (!isspace(quest->textWide[tmpPos]))
        --tmpPos;
      if (tmpPos - lineLengthWide / 2 <= currPos - lineLengthWide)
      {
        memmove_s(quest->textWide + currPos + 1, sizeof(char) * (textLength - currPos), quest->textWide + currPos, sizeof(char) * (textLength - currPos));
        quest->textWide[currPos] = '\n';
        ++textLength;
      }
      else
        quest->textWide[tmpPos] = '\n';
    }
    ++quest->textLinesWide;
    ++currPos;
  }
  return;
}

int _CustAtoI(const char *text, int pos, int length)
{
  int output = 0, mul = 1, startPos = 0;
  startPos = pos + length - 1;
  for (int i = 0; i < length; ++i)
  {
    output += (int)(text[startPos] - (char)(48)) * mul;
    --startPos;
    mul *= 10;
  }
  return output;
}

void _GenerateEventText(worldEvent_t *Event, FILE *source)
{
  int lines = 0;
  char iden[6] = "", line[2000] = "";
  Event->text = (char *)calloc(2100, sizeof(char));
  rewind(source);
  while (!feof(source))
  {
    fgets(line, 2000, source);
    memcpy_s(iden, sizeof(char) * 6, line, sizeof(char) * 6);
    iden[5] = 0;
    if (*iden == 'E')
      if (_CustAtoI(iden, 1, 2) == Event->type)
        if (_CustAtoI(iden, 3, 2) == Event->subtype)
        {
          memcpy_s(Event->text, sizeof(char) * (strlen(line) - 6), line + 6, sizeof(char) * (strlen(line) - 6));
          Event->text[strlen(line) - 7] = 0;
          _TextConverter(Event, 90);
          break;
        }
  }
  return;
}

void _GenerateQuestText(quest_t *quest, FILE *source)
{
  int lines = 0;
  char iden[6] = "", line[2000] = "";
  quest->text = (char *)calloc(2100, sizeof(char));
  quest->textWide = (char *)calloc(2100, sizeof(char));
  rewind(source);
  while (!feof(source))
  {
    fgets(line, 2000, source);
    memcpy_s(iden, sizeof(char) * 6, line, sizeof(char) * 6);
    iden[5] = 0;
    if (*iden == 'Q')
      if (_CustAtoI(iden, 1, 2) == quest->type)
        if (_CustAtoI(iden, 3, 2) == quest->subtype)
        {
          memcpy_s(quest->text, sizeof(char) * (strlen(line) - 6), line + 6, sizeof(char) * (strlen(line) - 6));
          quest->text[strlen(line) - 7] = 0;
          _TextConverter(quest, 50, 75);
          break;
        }
  }
  return;
}

int _GameSave(savedata data, int slot)
{
  FILE *savefile = NULL;
  planet_t *plaPtr = NULL, *satPtr = NULL;
  quest_t *questPtr = NULL;
  char planetBuff[sizeof(planet_t)], sysBuff[sizeof(savedata)], questBuff[sizeof(quest_t)];
  switch (slot)
  {
  case 0:
    fopen_s(&savefile, "autosave.sav", "wb");
    break;
  case 1:
    fopen_s(&savefile, "save1.sav", "wb");
    break;
  case 2:
    fopen_s(&savefile, "save2.sav", "wb");
    break;
  case 3:
    fopen_s(&savefile, "save3.sav", "wb");
    break;
  case 4:
    fopen_s(&savefile, "save4.sav", "wb");
    break;
  case 5:
    fopen_s(&savefile, "save5.sav", "wb");
    break;
  }
  memcpy(sysBuff, &data, sizeof(savedata));
  fwrite(sysBuff, sizeof(savedata), 1, savefile);
  memcpy(planetBuff, data.system, sizeof(planet_t));
  fwrite(planetBuff, sizeof(planet_t), 1, savefile);
  fputs(data.system->name, savefile);
  plaPtr = data.system->satellites;
  for (int i = 0; i < data.system->satelliteCount; ++i)
  {
    memcpy(planetBuff, plaPtr, sizeof(planet_t));
    fwrite(planetBuff, sizeof(planet_t), 1, savefile);
    fputs(plaPtr->name, savefile);
    satPtr = plaPtr->satellites;
    for (int j = 0; j < plaPtr->satelliteCount; ++j)
    {
      memcpy(planetBuff, satPtr, sizeof(planet_t));
      fwrite(planetBuff, sizeof(planet_t), 1, savefile);
      fputs(satPtr->name, savefile);
      satPtr = satPtr->next;
    }
    plaPtr = plaPtr->next;
  }
  if (data.questMain)
  {
    questPtr = data.questMain;
    do
    {
      memcpy(questBuff, questPtr, sizeof(quest_t));
      fwrite(questBuff, sizeof(quest_t), 1, savefile);
      questPtr = questPtr->next;
    }while ((questPtr) && (questPtr != data.questMain));
  }
  fclose(savefile);
  return 0;
}

int _GameLoad(int slot, savedata **gameload, FILE *testsource)
{
  FILE *loadfile = NULL;
  quest_t *currQuest = NULL, *tmpQuest = NULL;
  planet_t *plaPtr = NULL, *satPtr = NULL, *tmpPtr = NULL, *tmpSat = NULL, *plaOld = NULL, *satOld = NULL;
  char planetBuff[sizeof(planet_t)], sysBuff[sizeof(savedata)], questBuff[sizeof(quest_t)];
  loadpointers_t pointers[100] = {};
  int pointerCount = 0, checker = 0;
  savedata *loadLoc = NULL;
  switch (slot)
  {
  case 0:
    fopen_s(&loadfile, "autosave.sav", "rb");
    break;
  case 1:
    fopen_s(&loadfile, "save1.sav", "rb");
    break;
  case 2:
    fopen_s(&loadfile, "save2.sav", "rb");
    break;
  case 3:
    fopen_s(&loadfile, "save3.sav", "rb");
    break;
  case 4:
    fopen_s(&loadfile, "save4.sav", "rb");
    break;
  case 5:
    fopen_s(&loadfile, "save5.sav", "rb");
    break;
  }
  if (loadfile == NULL)
    return 0;
  fread(sysBuff, sizeof(savedata), 1, loadfile);
  loadLoc = (savedata *)calloc(1, sizeof(savedata));
  memcpy(loadLoc, sysBuff, sizeof(savedata));
  pointers->oldPtr = loadLoc->system;
  loadLoc->system = (planet_t *)calloc(1, sizeof(planet_t));
  pointers->newPtr = loadLoc->system;
  ++pointerCount;
  fread(planetBuff, sizeof(planet_t), 1, loadfile);
  memcpy(loadLoc->system, planetBuff, sizeof(planet_t));
  loadLoc->system->name = (char *)calloc(6, sizeof(char));
  fgets(loadLoc->system->name, 6, loadfile);
  plaOld = loadLoc->system->satellites;
  loadLoc->system->satellites = NULL;
  for (int i = 0; i < loadLoc->system->satelliteCount; ++i)
  {
    plaPtr = (planet_t *)calloc(1, sizeof(planet_t));
    fread(planetBuff, sizeof(planet_t), 1, loadfile);
    memcpy(plaPtr, planetBuff, sizeof(planet_t));
    if (loadLoc->system->satellites == NULL)
      loadLoc->system->satellites = plaPtr;
    if (tmpPtr != NULL)
      tmpPtr->next = plaPtr;
    plaPtr->prev = tmpPtr;
    plaPtr->parent = loadLoc->system;
    pointers[pointerCount].oldPtr = plaOld;
    pointers[pointerCount].newPtr = plaPtr;
    ++pointerCount;
    if ((plaPtr->type < 5) || (plaPtr->type == STAR_GATE))
    {
      plaPtr->name = (char *)calloc(9, sizeof(char));
      fgets(plaPtr->name, 9, loadfile);
    }
    else
      if (plaPtr->type < 8)
      {
        plaPtr->name = (char *)calloc(11, sizeof(char));
        fgets(plaPtr->name, 11, loadfile);
      }
    satOld = plaPtr->satellites;
    plaPtr->satellites = NULL;
    for (int j = 0; j < plaPtr->satelliteCount; ++j)
    {
      satPtr = (planet_t *)calloc(1, sizeof(planet_t));
      fread(planetBuff, sizeof(planet_t), 1, loadfile);
      memcpy(satPtr, planetBuff, sizeof(planet_t));
      if (plaPtr->satellites == NULL)
        plaPtr->satellites = satPtr;
      if (tmpSat != NULL)
        tmpSat->next = satPtr;
      satPtr->prev = tmpSat;
      satPtr->parent = plaPtr;
      pointers[pointerCount].oldPtr = satOld;
      pointers[pointerCount].newPtr = satPtr;
      ++pointerCount;
      if ((satPtr->type < 5) || (satPtr->type == STAR_GATE))
      {
        satPtr->name = (char *)calloc(9, sizeof(char));
        fgets(satPtr->name, 9, loadfile);
      }
      else
        if (satPtr->type < 8)
        {
          satPtr->name = (char *)calloc(11, sizeof(char));
          fgets(satPtr->name, 11, loadfile);
        }
      satOld = satPtr->next;
      tmpSat = satPtr;
    }
    tmpSat = NULL;
    plaOld = plaPtr->next;
    tmpPtr = plaPtr;
  }
  for (int i = 0; i < pointerCount; ++i)
    if (loadLoc->ship.well == pointers[i].oldPtr)
    {
      loadLoc->ship.well = (planet_t *)pointers[i].newPtr;
      break;
    }
  if (fread(questBuff, sizeof(quest_t), 1, loadfile))
  {
    loadLoc->questMain = (quest_t *)calloc(1, sizeof(quest_t));
    memcpy(loadLoc->questMain, questBuff, sizeof(quest_t));
    tmpQuest = loadLoc->questMain;
    checker = 0;
    for (int i = 0; i < pointerCount; ++i)
    {
      if (loadLoc->questMain->starter == pointers[i].oldPtr)
      {
        loadLoc->questMain->starter = (planet_t *)pointers[i].newPtr;
        loadLoc->questMain->starter->currQuest = loadLoc->questMain;
        ++checker;
      }
      if (loadLoc->questMain->goal == pointers[i].oldPtr)
      {
        loadLoc->questMain->goal = (planet_t *)pointers[i].newPtr;
        loadLoc->questMain->goal->currQuest = loadLoc->questMain;
        ++checker;
      }
      if (checker == 2)
        break;
    }
    _GenerateQuestText(loadLoc->questMain, testsource);
    if (loadLoc->questMain->next)
    {
      while (fread(questBuff, sizeof(quest_t), 1, loadfile))
      {
        currQuest = (quest_t *)calloc(1, sizeof(quest_t));
        memcpy(currQuest, questBuff, sizeof(quest_t));
        tmpQuest->next = currQuest;
        currQuest->prev = tmpQuest;
        tmpQuest = currQuest;
        checker = 0;
        for (int i = 0; i < pointerCount; ++i)
        {
          if (currQuest->starter == pointers[i].oldPtr)
          {
            currQuest->starter = (planet_t *)pointers[i].newPtr;
            currQuest->starter->currQuest = currQuest;
            ++checker;
          }
          if (currQuest->goal == pointers[i].oldPtr)
          {
            currQuest->goal = (planet_t *)pointers[i].newPtr;
            currQuest->goal->currQuest = currQuest;
            ++checker;
          }
          if (checker == 2)
            break;
        }
        _GenerateQuestText(currQuest, testsource);
      }
      currQuest->next = loadLoc->questMain;
      loadLoc->questMain->prev = currQuest;
    }
  }
  *gameload = loadLoc;
  fclose(loadfile);
  return 1;
}

LONGLONG _StartCounter(double *PCFreq)
{
  LARGE_INTEGER li;
  QueryPerformanceFrequency(&li);
  *PCFreq = (double)(li.QuadPart) / 1000000.0;
  QueryPerformanceCounter(&li);
  return li.QuadPart;
}

double _GetCounter(ULONGLONG CounterStart, double PCFreq)
{
  LARGE_INTEGER li;
  QueryPerformanceCounter(&li);
  return (double)(li.QuadPart - CounterStart) / PCFreq;
}

void _PlaceShip(planet_t system, playerShip_t *ship, bool first)
{
  rand();
  double angle = (double)rand(), dist = 0.0, vel = 0.0;
  planet_t *plaPtr = system.satellites, *designation = NULL;
  int shipPlanet = 0;
  if (first)
    shipPlanet = rand() % (system.satelliteCount - 1);
  else
    shipPlanet = system.satelliteCount - 1;
  for (int i = 0; i < shipPlanet; ++i)
    plaPtr = plaPtr->next;
  designation = plaPtr;
  dist += 150 * designation->bodyProperties.mass * (2 + 0.01  * (rand() % 100 - 50)) + designation->bodyProperties.rad;
  ship->bodyProperties.coord.x = dist * cos(angle) + designation->bodyProperties.coord.x;
  ship->bodyProperties.coord.y = dist * sin(angle) + designation->bodyProperties.coord.y;
  vel = sqrt(GRAV * designation->bodyProperties.mass / dist * (1 + 0.01 * (rand() % 50 - 20)));
  angle -= PI / 2;
  ship->bodyProperties.vel.x = vel * cos(angle) + designation->bodyProperties.vel.x;
  ship->bodyProperties.vel.y = vel * sin(angle) + designation->bodyProperties.vel.y;
  ship->well = designation;
  return;
}

planet_t *_GeneratePlanet(PLANET_TYPE type, planet_t *parent)
{
  rand();
  planet_t *planet = (planet_t *)calloc(1, sizeof(planet_t)), *prev = NULL;
  double angle = (double)rand(), dist = 0.0, vel = 0.0, density = 0.0;
  planet->type = type;
  planet->parent = parent;
  if (parent)
  {
    prev = parent->satellites;
    for (int i = 1; i < parent->satelliteCount; ++i)
      prev = prev->next;
  }
  switch (type)
  {
  case STAR:
    planet->bodyProperties.mass = 10000.0 * (1 + 0.03 * (rand() % 60 - 10));
    switch (rand() % 4)
    {
    case 0:
      planet->rcol = 250;
      planet->gcol = 250;
      break;
    case 1:
      planet->rcol = 210;
      break;
    case 2:
      planet->rcol = 255;
      planet->gcol = 255;
      planet->bcol = 255;
      break;
    case 3:
      planet->bcol = 255;
      planet->rcol = 100;
      planet->gcol = 100;
      break;
    }
    planet->name = (char *)calloc(6, sizeof(char));
    for (int i = 0; i < 2; ++i)
      planet->name[i] = 48 + rand() % 10;
    for (int i = 2; i < 5; ++i)
      planet->name[i] = 65 + rand() % 26;
    density = 2 * (1 + 0.01 * (rand() % 30 - 10));
    planet->bodyProperties.rad = 4 * density * planet->bodyProperties.mass;
    planet->gravRadius = 10000.0 * planet->bodyProperties.mass;
    return planet;
  case GAS_GIANT:
    planet->bodyProperties.mass = 100.0 * (2 + 0.05 * (rand() % 60 - 20));
    density = 1.5 * (1 + 0.01 * (rand() % 30 - 10));
    switch (rand() % 3)
    {
    case 0:
      planet->rcol = 100;
      planet->gcol = 100;
      break;
    case 1:
      planet->gcol = 200;
      planet->bcol = 200;
      break;
    case 2:
      planet->rcol = 200;
      planet->gcol = 100;
      break;
    }
    planet->name = (char *)calloc(9, sizeof(char));
    memcpy(planet->name, parent->name, 5 * sizeof(char));
    planet->name[5] = '-';
    planet->name[6] = '0';
    planet->name[7] = (char)(49 + parent->satelliteCount);
    density = 1 * (1 + 0.01 * (rand() % 60 - 20));
    break;
  case UNINHABITABLE:
    planet->bodyProperties.mass = 1.0 * (2 + 0.02 * (rand() % 100 - 40));
    planet->clim = rand() % 3;
    switch (planet->clim)
    {
    case 0:
      planet->rcol = 50;
      planet->gcol = 50;
      planet->bcol = 50;
      break;
    case 1:
      planet->rcol = 150;
      planet->gcol = 150;
      planet->bcol = 150;
      break;
    case 2:
      planet->rcol = 150;
      planet->gcol = 150;
      break;
    }
    planet->scansAvail = 3 + rand() % 4;
    planet->name = (char *)calloc(9, sizeof(char));
    memcpy(planet->name, parent->name, 5 * sizeof(char));
    planet->name[5] = '-';
    planet->name[6] = '0';
    planet->name[7] = (char)(49 + parent->satelliteCount);
    density = 5 * (1 + 0.02 * (rand() % 60 - 20));
    break;
  case UNEXPLORABLE:
    planet->bodyProperties.mass = 0.5 * (4 + 0.02 * (rand() % 100 - 40));
    planet->clim = rand() % 2;
    switch (planet->clim)
    {
    case 0:
      planet->rcol = 150;
      break;
    case 1:
      planet->gcol = 200;
      planet->bcol = 200;
      break;
    }
    planet->name = (char *)calloc(9, sizeof(char));
    memcpy(planet->name, parent->name, 5 * sizeof(char));
    planet->name[5] = '-';
    planet->name[6] = '0';
    planet->name[7] = (char)(49 + parent->satelliteCount);
    density = 5 * (1 + 0.03 * (rand() % 80 - 30));
    break;
  case HABITABLE:
    planet->bodyProperties.mass = 1 * (1 + 0.01 * (rand() % 60 - 30));
    planet->clim = rand() % 3;
    switch (planet->clim)
    {
    case 0:
      planet->rcol = 100;
      planet->gcol = 200;
      break;
    case 1:
      planet->gcol = 100;
      planet->bcol = 200;
      break;
    case 2:
      planet->rcol = 100;
      planet->bcol = 100;
      planet->gcol = 250;
      break;
    }
    planet->age = 2200 + (rand() % 150);
    planet->pop = (int)(150 * (1 + 0.01 * (rand() % 80 - 40)));
    planet->eco = rand() % 4;
    switch (planet->eco)
    {
    case 0:
      planet->foodQ = rand() % 50;
      planet->medQ = rand() % 100;
      planet->minQ = rand() % 20;
      planet->weapQ = 50 + rand() % 150;
      planet->techQ = 50 + rand() % 150;
      planet->foodPB = (FOOD_MAX_PRICE + FOOD_MIN_PRICE) / 2 * (1 + 0.015 * (rand() % 60 - 30));
      planet->foodPS = planet->foodPB + (FOOD_MAX_PRICE + FOOD_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->medPB = (MED_MAX_PRICE + MED_MIN_PRICE) / 2 * (1 + 0.015 * (rand() % 60 - 30));
      planet->medPS = planet->medPB + (MED_MAX_PRICE + MED_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->minPB = (3 * MIN_MAX_PRICE + MIN_MIN_PRICE) / 4 * (1 + 0.015 * (rand() % 60 - 30));
      planet->minPS = planet->minPB + (MIN_MAX_PRICE + MIN_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->weapPB = (WEAP_MAX_PRICE + 3 * WEAP_MIN_PRICE) / 4 * (1 + 0.015 * (rand() % 60 - 30));
      planet->weapPS = planet->weapPB + (WEAP_MAX_PRICE + WEAP_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->techPB = (TECH_MAX_PRICE + 3 * TECH_MIN_PRICE) / 4 * (1 + 0.015 * (rand() % 60 - 30));
      planet->techPS = planet->techPB + (TECH_MAX_PRICE + TECH_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->fuelPrice = (FUEL_MAX_PRICE + 3 * FUEL_MIN_PRICE) / 4 * (1 + 0.015 * (rand() % 60 - 30));
      break;
    case 1:
      planet->foodQ = 50 + rand() % 200;
      planet->medQ = 10 + rand() % 100;
      planet->minQ = rand() % 100;
      planet->weapQ = rand() % 50;
      planet->techQ = rand() % 20;
      planet->foodPB = (FOOD_MAX_PRICE + 3 * FOOD_MIN_PRICE) / 4 * (1 + 0.015 * (rand() % 60 - 30));
      planet->foodPS = planet->foodPB + (FOOD_MAX_PRICE + 2 * FOOD_MIN_PRICE) / 3 * (0.03 * (5 + rand() % 20));
      planet->medPB = (MED_MAX_PRICE + 2 * MED_MIN_PRICE) / 3 * (1 + 0.015 * (rand() % 60 - 30));
      planet->medPS = planet->medPB + (MED_MAX_PRICE + MED_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->minPB = (MIN_MAX_PRICE + MIN_MIN_PRICE) / 2 * (1 + 0.015 * (rand() % 60 - 30));
      planet->minPS = planet->minPB + (MIN_MAX_PRICE + MIN_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->weapPB = (2 * WEAP_MAX_PRICE + WEAP_MIN_PRICE) / 3 * (1 + 0.015 * (rand() % 60 - 30));
      planet->weapPS = planet->weapPB + (WEAP_MAX_PRICE + WEAP_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->techPB = (4 * TECH_MAX_PRICE + TECH_MIN_PRICE) / 5 * (1 + 0.015 * (rand() % 60 - 30));
      planet->techPS = planet->techPB + (2 * TECH_MAX_PRICE + TECH_MIN_PRICE) / 3 * (0.03 * (5 + rand() % 20));
      planet->fuelPrice = (3 * FUEL_MAX_PRICE + FUEL_MIN_PRICE) / 4 * (1 + 0.015 * (rand() % 60 - 30));
      break;
    case 2:
      planet->foodQ = 10 + rand() % 100;
      planet->medQ = 10 + rand() % 100;
      planet->minQ = 10 + rand() % 100;
      planet->weapQ = 10 + rand() % 100;
      planet->techQ = 10 + rand() % 100;
      planet->foodPB = (FOOD_MAX_PRICE + FOOD_MIN_PRICE) / 2 * (1 + 0.015 * (rand() % 60 - 30));
      planet->foodPS = planet->foodPB + (FOOD_MAX_PRICE + FOOD_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->medPB = (MED_MAX_PRICE + MED_MIN_PRICE) / 2 * (1 + 0.015 * (rand() % 60 - 30));
      planet->medPS = planet->medPB + (MED_MAX_PRICE + MED_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->minPB = (MIN_MAX_PRICE + MIN_MIN_PRICE) / 2 * (1 + 0.015 * (rand() % 60 - 30));
      planet->minPS = planet->minPB + (MIN_MAX_PRICE + MIN_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->weapPB = (WEAP_MAX_PRICE + WEAP_MIN_PRICE) / 2 * (1 + 0.015 * (rand() % 60 - 30));
      planet->weapPS = planet->weapPB + (WEAP_MAX_PRICE + WEAP_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->techPB = (TECH_MAX_PRICE + TECH_MIN_PRICE) / 2 * (1 + 0.015 * (rand() % 60 - 30));
      planet->techPS = planet->techPB + (TECH_MAX_PRICE + TECH_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->fuelPrice = (FUEL_MAX_PRICE + FUEL_MIN_PRICE) / 2 * (1 + 0.015 * (rand() % 60 - 30));
      break;
    case 3:
      planet->foodQ = 20 + rand() % 100;
      planet->medQ = 20 + rand() % 100;
      planet->minQ = rand() % 100;
      planet->weapQ = 10 + rand() % 100;
      planet->techQ = 10 + rand() % 100;
      planet->foodPB = (FOOD_MAX_PRICE + 2 * FOOD_MIN_PRICE) / 3 * (1 + 0.015 * (rand() % 60 - 30));
      planet->foodPS = planet->foodPB + (FOOD_MAX_PRICE + FOOD_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->medPB = (MED_MAX_PRICE + 2 * MED_MIN_PRICE) / 3 * (1 + 0.015 * (rand() % 60 - 30));
      planet->medPS = planet->medPB + (MED_MAX_PRICE + MED_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->minPB = (2 * MIN_MAX_PRICE + MIN_MIN_PRICE) / 3 * (1 + 0.015 * (rand() % 60 - 30));
      planet->minPS = planet->minPB + (MIN_MAX_PRICE + MIN_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->weapPB = (WEAP_MAX_PRICE + 2 * WEAP_MIN_PRICE) / 3 * (1 + 0.015 * (rand() % 60 - 30));
      planet->weapPS = planet->weapPB + (WEAP_MAX_PRICE + WEAP_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->techPB = (TECH_MAX_PRICE + 2 * TECH_MIN_PRICE) / 3 * (1 + 0.015 * (rand() % 60 - 30));
      planet->techPS = planet->techPB + (TECH_MAX_PRICE + TECH_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->fuelPrice = (FUEL_MAX_PRICE + FUEL_MIN_PRICE) / 2 * (1 + 0.015 * (rand() % 60 - 30));
      break;
    }
    planet->name = (char *)calloc(9, sizeof(char));
    memcpy(planet->name, parent->name, 5 * sizeof(char));
    planet->name[5] = '-';
    planet->name[6] = '0';
    planet->name[7] = (char)(49 + parent->satelliteCount);
    density = 5 * (1 + 0.1 * (rand() % 3 - 1));
    break;
  case MOON:
    planet->bodyProperties.mass = 0.05 * (2 + 0.04 * (rand() % 100 - 40));
    planet->clim = rand() % 3;
    switch (planet->clim)
    {
    case 0:
      planet->rcol = 50;
      planet->gcol = 50;
      planet->bcol = 50;
      break;
    case 1:
      planet->rcol = 150;
      planet->gcol = 150;
      planet->bcol = 150;
      break;
    case 2:
      planet->rcol = 150;
      planet->gcol = 150;
      break;
    }
    planet->scansAvail = 1 + rand() % 3;
    planet->name = (char *)calloc(11, sizeof(char));
    memcpy(planet->name, parent->name, 8 * sizeof(char));
    planet->name[8] = '-';
    planet->name[9] = (char)(65 + parent->satelliteCount);
    density = 5 * (1 + 0.02 * (rand() % 60 - 20));
    break;
  case MOON_HABITABLE:
    planet->bodyProperties.mass = 0.05 * (2 + 0.04 * (rand() % 100 - 40));
    planet->clim = rand() % 3;
    switch (planet->clim)
    {
    case 0:
      planet->rcol = 100;
      planet->gcol = 200;
      break;
    case 1:
      planet->gcol = 100;
      planet->bcol = 200;
      break;
    case 2:
      planet->rcol = 100;
      planet->bcol = 100;
      planet->gcol = 250;
      break;
    }
    planet->age = 2200 + (rand() % 150);
    planet->pop = (int)(10 * (1 + 0.09 * (rand() % 6 - 3)));
    planet->eco = rand() % 4;
    switch (planet->eco)
    {
    case 0:
      planet->foodQ = rand() % 50;
      planet->medQ = rand() % 100;
      planet->minQ = rand() % 20;
      planet->weapQ = 50 + rand() % 150;
      planet->techQ = 50 + rand() % 150;
      planet->foodPB = (FOOD_MAX_PRICE + FOOD_MIN_PRICE) / 2 * (1 + 0.015 * (rand() % 60 - 30));
      planet->foodPS = planet->foodPB + (FOOD_MAX_PRICE + FOOD_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->medPB = (MED_MAX_PRICE + MED_MIN_PRICE) / 2 * (1 + 0.015 * (rand() % 60 - 30));
      planet->medPS = planet->medPB + (MED_MAX_PRICE + MED_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->minPB = (3 * MIN_MAX_PRICE + MIN_MIN_PRICE) / 4 * (1 + 0.015 * (rand() % 60 - 30));
      planet->minPS = planet->minPB + (MIN_MAX_PRICE + MIN_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->weapPB = (WEAP_MAX_PRICE + 3 * WEAP_MIN_PRICE) / 4 * (1 + 0.015 * (rand() % 60 - 30));
      planet->weapPS = planet->weapPB + (WEAP_MAX_PRICE + WEAP_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->techPB = (TECH_MAX_PRICE + 3 * TECH_MIN_PRICE) / 4 * (1 + 0.015 * (rand() % 60 - 30));
      planet->techPS = planet->techPB + (TECH_MAX_PRICE + TECH_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->fuelPrice = (FUEL_MAX_PRICE + 3 * FUEL_MIN_PRICE) / 4 * (1 + 0.015 * (rand() % 60 - 30));
      break;
    case 1:
      planet->foodQ = 50 + rand() % 200;
      planet->medQ = 10 + rand() % 100;
      planet->minQ = rand() % 100;
      planet->weapQ = rand() % 50;
      planet->techQ = rand() % 20;
      planet->foodPB = (FOOD_MAX_PRICE + 3 * FOOD_MIN_PRICE) / 4 * (1 + 0.015 * (rand() % 60 - 30));
      planet->foodPS = planet->foodPB + (FOOD_MAX_PRICE + 2 * FOOD_MIN_PRICE) / 3 * (0.03 * (5 + rand() % 20));
      planet->medPB = (MED_MAX_PRICE + 2 * MED_MIN_PRICE) / 3 * (1 + 0.015 * (rand() % 60 - 30));
      planet->medPS = planet->medPB + (MED_MAX_PRICE + MED_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->minPB = (MIN_MAX_PRICE + MIN_MIN_PRICE) / 2 * (1 + 0.015 * (rand() % 60 - 30));
      planet->minPS = planet->minPB + (MIN_MAX_PRICE + MIN_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->weapPB = (2 * WEAP_MAX_PRICE + WEAP_MIN_PRICE) / 3 * (1 + 0.015 * (rand() % 60 - 30));
      planet->weapPS = planet->weapPB + (WEAP_MAX_PRICE + WEAP_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->techPB = (4 * TECH_MAX_PRICE + TECH_MIN_PRICE) / 5 * (1 + 0.015 * (rand() % 60 - 30));
      planet->techPS = planet->techPB + (2 * TECH_MAX_PRICE + TECH_MIN_PRICE) / 3 * (0.03 * (5 + rand() % 20));
      planet->fuelPrice = (3 * FUEL_MAX_PRICE + FUEL_MIN_PRICE) / 4 * (1 + 0.015 * (rand() % 60 - 30));
      break;
    case 2:
      planet->foodQ = 10 + rand() % 100;
      planet->medQ = 10 + rand() % 100;
      planet->minQ = 10 + rand() % 100;
      planet->weapQ = 10 + rand() % 100;
      planet->techQ = 10 + rand() % 100;
      planet->foodPB = (FOOD_MAX_PRICE + FOOD_MIN_PRICE) / 2 * (1 + 0.015 * (rand() % 60 - 30));
      planet->foodPS = planet->foodPB + (FOOD_MAX_PRICE + FOOD_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->medPB = (MED_MAX_PRICE + MED_MIN_PRICE) / 2 * (1 + 0.015 * (rand() % 60 - 30));
      planet->medPS = planet->medPB + (MED_MAX_PRICE + MED_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->minPB = (MIN_MAX_PRICE + MIN_MIN_PRICE) / 2 * (1 + 0.015 * (rand() % 60 - 30));
      planet->minPS = planet->minPB + (MIN_MAX_PRICE + MIN_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->weapPB = (WEAP_MAX_PRICE + WEAP_MIN_PRICE) / 2 * (1 + 0.015 * (rand() % 60 - 30));
      planet->weapPS = planet->weapPB + (WEAP_MAX_PRICE + WEAP_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->techPB = (TECH_MAX_PRICE + TECH_MIN_PRICE) / 2 * (1 + 0.015 * (rand() % 60 - 30));
      planet->techPS = planet->techPB + (TECH_MAX_PRICE + TECH_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->fuelPrice = (FUEL_MAX_PRICE + FUEL_MIN_PRICE) / 2 * (1 + 0.015 * (rand() % 60 - 30));
      break;
    case 3:
      planet->foodQ = 20 + rand() % 100;
      planet->medQ = 20 + rand() % 100;
      planet->minQ = rand() % 100;
      planet->weapQ = 10 + rand() % 100;
      planet->techQ = 10 + rand() % 100;
      planet->foodPB = (FOOD_MAX_PRICE + 2 * FOOD_MIN_PRICE) / 3 * (1 + 0.015 * (rand() % 60 - 30));
      planet->foodPS = planet->foodPB + (FOOD_MAX_PRICE + FOOD_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->medPB = (MED_MAX_PRICE + 2 * MED_MIN_PRICE) / 3 * (1 + 0.015 * (rand() % 60 - 30));
      planet->medPS = planet->medPB + (MED_MAX_PRICE + MED_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->minPB = (2 * MIN_MAX_PRICE + MIN_MIN_PRICE) / 3 * (1 + 0.015 * (rand() % 60 - 30));
      planet->minPS = planet->minPB + (MIN_MAX_PRICE + MIN_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->weapPB = (WEAP_MAX_PRICE + 2 * WEAP_MIN_PRICE) / 3 * (1 + 0.015 * (rand() % 60 - 30));
      planet->weapPS = planet->weapPB + (WEAP_MAX_PRICE + WEAP_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->techPB = (TECH_MAX_PRICE + 2 * TECH_MIN_PRICE) / 3 * (1 + 0.015 * (rand() % 60 - 30));
      planet->techPS = planet->techPB + (TECH_MAX_PRICE + TECH_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      planet->fuelPrice = (FUEL_MAX_PRICE + FUEL_MIN_PRICE) / 2 * (1 + 0.015 * (rand() % 60 - 30));
      break;
    }
    planet->name = (char *)calloc(11, sizeof(char));
    memcpy(planet->name, parent->name, 8 * sizeof(char));
    planet->name[8] = '-';
    planet->name[9] = (char)(65 + parent->satelliteCount);
    density = 5 * (1 + 0.02 * (rand() % 60 - 20));
    break;
  case MOON_UNEXPLORABLE:
    planet->bodyProperties.mass = 0.05 * (3 + 0.04 * (rand() % 100 - 40));
    planet->clim = rand() % 2;
    switch (planet->clim)
    {
    case 0:
      planet->rcol = 150;
      break;
    case 1:
      planet->gcol = 200;
      planet->bcol = 200;
      break;
    }
    planet->name = (char *)calloc(11, sizeof(char));
    memcpy(planet->name, parent->name, 8 * sizeof(char));
    planet->name[8] = '-';
    planet->name[9] = (char)(65 + parent->satelliteCount);
    density = 5 * (1 + 0.02 * (rand() % 60 - 20));
    break;
  case STAR_GATE:
    planet->bodyProperties.mass = 0.1;
    planet->rcol = 150;
    planet->bcol = 150;
    planet->fuelPrice = (STARGATE_MAX_PRICE + STARGATE_MIN_PRICE) / 2  * (1 + 0.015 * (rand() % 50 - 50));
    planet->name = (char *)calloc(9, sizeof(char));
    memcpy(planet->name, parent->name, 5 * sizeof(char));
    planet->name[5] = '-';
    planet->name[6] = '0';
    planet->name[7] = (char)(49 + parent->satelliteCount);
    density = 5;
    break;
  }
  planet->gravRadius = planet->bodyProperties.mass * 10000;
  if (prev)
    dist = (sqrt(_GetSquareDist(prev->bodyProperties.coord, parent->bodyProperties.coord)) + prev->gravRadius);
  if (parent->type == STAR)
    dist += 500 * parent->bodyProperties.mass * (2 + 0.005 * (1 + parent->satelliteCount) * (rand() % 10 - 5)) + parent->bodyProperties.rad;
  else
    dist += 800 * parent->bodyProperties.mass * (2 + 0.005 * (1 + parent->satelliteCount) * (rand() % 10 - 5)) + parent->bodyProperties.rad;
  if (prev != NULL)
    dist += planet->gravRadius / 2;
  planet->bodyProperties.coord.x = dist * cos(angle) + parent->bodyProperties.coord.x;
  planet->bodyProperties.coord.y = dist * sin(angle) + parent->bodyProperties.coord.y;
  vel = sqrt(GRAV * parent->bodyProperties.mass / dist * (1 + 0.00001 / (1 + parent->satelliteCount) * (rand() % 20 - 5)));
  angle -= PI / 2;
  planet->bodyProperties.vel.x = vel * cos(angle) + parent->bodyProperties.vel.x;
  planet->bodyProperties.vel.y = vel * sin(angle) + parent->bodyProperties.vel.y;
  planet->bodyProperties.rad = 4 * density * planet->bodyProperties.mass;
  return planet;
}

planet_t *_SystemGen(playerShip_t *ship, bool first)
{
  planet_t *system = NULL, *plaPtr = NULL, *satPtr = NULL, *tmpPtr = NULL, *tmpSatPtr = NULL;
  int planetCount = 0;
  rand();
  system = _GeneratePlanet(STAR, NULL);
  planetCount = 4 + rand() % 5;
  while (system->satelliteCount < planetCount)
  {
    int satelliteCount = 0;
    if (system->satelliteCount == planetCount - 1)
      plaPtr = _GeneratePlanet(STAR_GATE, system);
    else
      plaPtr = _GeneratePlanet((PLANET_TYPE)(1 + rand() % 4), system);
    if (tmpPtr)
      tmpPtr->next = plaPtr;
    plaPtr->prev = tmpPtr;
    if (system->satellites == NULL)
      system->satellites = plaPtr;
    if (plaPtr->type == GAS_GIANT)
      satelliteCount = 2 + rand() % 4;
    else
      satelliteCount = rand() % 4;
    if (plaPtr->type == STAR_GATE)
      satelliteCount = 0;
    while (plaPtr->satelliteCount < satelliteCount)
    {
      satPtr = _GeneratePlanet((PLANET_TYPE)(5 + rand() % 3), plaPtr);
      if (plaPtr->satellites == NULL)
        plaPtr->satellites = satPtr;
      if (tmpSatPtr)
        tmpSatPtr->next = satPtr;
      satPtr->prev = tmpSatPtr;
      if (!plaPtr->satellites)
        plaPtr->satellites = satPtr;
      ++plaPtr->satelliteCount;
      tmpSatPtr = satPtr;
      satPtr = satPtr->next;
    }
    ++system->satelliteCount;
    tmpPtr = plaPtr;
    plaPtr = plaPtr->next;
    tmpSatPtr = NULL;
  }
  _PlaceShip(*system, ship, first);
  return system;
}

void _SystemDestroy(planet_t *system)
{
  planet_t *plaPtr = system->satellites, *satPtr = NULL;
  free(system->name);
  for (int i = 0; i < system->satelliteCount; ++i)
  {
    free(plaPtr->name);
    if (plaPtr->prev)
      free(plaPtr->prev);
    satPtr = plaPtr->satellites;
    for (int j = 0; j < plaPtr->satelliteCount; ++j)
    {
      free(satPtr->name);
      satPtr->name = NULL;
      if (satPtr->prev)
        free(satPtr->prev);
      satPtr = satPtr->next;
    }
    plaPtr = plaPtr->next;
  }
  free(system);
  return;
}

coord_t _GetMouseCoord(SDL_Point mousePos, planet_t *owner, playerShip_t ship, double scale, int mapMode)
{
  coord_t mouse = { 0.0, 0.0 };
  double dist = 0.0, distScale = 0.0;
  if (!mapMode)
  {
    if ((mousePos.x <= 200) && (mousePos.y >= SCREEN_HEIGHT - 200))
    {
      double distScale = 100.0 / owner->gravRadius;
      mouse.x = ship.well->bodyProperties.coord.x + (mousePos.x - 100) / distScale;
      mouse.y = ship.well->bodyProperties.coord.y + (mousePos.y - SCREEN_HEIGHT + 100) / distScale;
    }
    else
    {
      mouse.x = ship.bodyProperties.coord.x + (mousePos.x - SCREEN_WIDTH / 2) / scale;
      mouse.y = ship.bodyProperties.coord.y + (mousePos.y - SCREEN_HEIGHT / 2) / scale;
    }
  }
  else
  {
    distScale = SCREEN_HEIGHT / (2 * owner->gravRadius);
    mouse.x = (mousePos.x - SCREEN_WIDTH / 2) / distScale;
    mouse.y = (mousePos.y - SCREEN_HEIGHT / 2) / distScale;
  }
  return mouse;
}

planet_t *_CheckTarget(globals *all)
{
  if ((all->mousePos.x >= SCREEN_WIDTH - 200) && (all->mousePos.y >= SCREEN_HEIGHT - 200))
    return NULL;
  bool map = false;
  planet_t *satPtr, *center = all->ship.well;
  coord_t mouse;
  double dist = 1.0, currDist = 0.0, rad = 0.0, mapScale = 2 + 4 * (all->flags.mapMode != 0),
    bodyScale = 0.0;
  if (all->flags.mapMode == 2)
    center = all->system;
  bodyScale = (5 + 10 * (all->flags.mapMode != 0)) / sqrt(center->bodyProperties.rad);
  mouse = _GetMouseCoord(all->mousePos, center, all->ship, all->timeMods.scale, all->flags.mapMode);
  if (((all->mousePos.x <= 200) && (all->mousePos.y >= SCREEN_HEIGHT - 200)) || (all->flags.mapMode))
  {
    rad = mapScale;
    map = true;
  }
  if (!map)
  {
    currDist = sqrt(_GetSquareDist(mouse, center->bodyProperties.coord));
    if (currDist < all->ship.well->bodyProperties.rad + rad)
      return center;
    satPtr = center->satellites;
    for (int i = 0; i < center->satelliteCount; ++i)
    {
      currDist = sqrt(_GetSquareDist(mouse, satPtr->bodyProperties.coord));
      if (currDist < satPtr->bodyProperties.rad + rad)
        return satPtr;
      satPtr = satPtr->next;
    }
  }
  else
  {
    double distScale;
    if (all->flags.mapMode == 0)
      distScale = 100 / center->gravRadius;
    else
      distScale = SCREEN_HEIGHT / (2 * center->gravRadius);
    currDist = sqrt(_GetSquareDist(mouse, center->bodyProperties.coord));
    if (currDist * distScale < sqrt(center->bodyProperties.rad) * bodyScale + rad)
      return center;
    satPtr = center->satellites;
    for (int i = 0; i < center->satelliteCount; ++i)
    {
      currDist = sqrt(_GetSquareDist(mouse, satPtr->bodyProperties.coord));
      if (currDist * distScale < sqrt(satPtr->bodyProperties.rad) * bodyScale + rad)
        return satPtr;
      satPtr = satPtr->next;
    }
  }
  return NULL;
}

planet_t *_CheckTarget(globals *all, coord_t shipPos)
{
  planet_t *satPtr;
  double currDist = 0.0, distScale = SCREEN_HEIGHT / (2 * all->ship.well->gravRadius),
    bodyScale = 15 / sqrt(all->ship.well->bodyProperties.rad);
  currDist = sqrt(_GetSquareDist(shipPos, all->ship.well->bodyProperties.coord));
  if (currDist * distScale < sqrt(all->ship.well->bodyProperties.rad) * bodyScale + 6)
    return all->ship.well;
  satPtr = all->ship.well->satellites;
  for (int i = 0; i < all->ship.well->satelliteCount; ++i)
  {
    currDist = sqrt(_GetSquareDist(shipPos, satPtr->bodyProperties.coord));
    if (currDist * distScale < sqrt(satPtr->bodyProperties.rad) * bodyScale + 6)
      return satPtr;
    satPtr = satPtr->next;
  }
  return NULL;
}

void _Teleport(globals *all)
{
  double angle = (double)rand(), dist = 0.0, vel = 0.0;
  planet_t *designation = NULL;
  if (all->trackPlanet)
  {
    designation = all->trackPlanet;
    dist += 150 * designation->bodyProperties.mass * (2 + 0.01  * (rand() % 100 - 50)) + designation->bodyProperties.rad;
    all->ship.bodyProperties.coord.x = dist * cos(angle) + designation->bodyProperties.coord.x;
    all->ship.bodyProperties.coord.y = dist * sin(angle) + designation->bodyProperties.coord.y;
    vel = sqrt(GRAV * designation->bodyProperties.mass / dist * (1 + 0.01 * (rand() % 50 - 20)));
    angle -= PI / 2;
    all->ship.bodyProperties.vel.x = vel * cos(angle) + designation->bodyProperties.vel.x;
    all->ship.bodyProperties.vel.y = vel * sin(angle) + designation->bodyProperties.vel.y;
    all->ship.well = designation;
    all->timeMods.scale = 10 / sqrt(all->ship.well->bodyProperties.rad);
    if (all->timeMods.scale < 0.5)
    {
      vel = log(all->timeMods.scale);
      all->x = (int)(vel * 10) / 10.0;
    }
    else
      all->x = 1.0;
    all->timeMods.timeControl = exp(all->x);
    all->flags.modsChange = true;
    return;
  }
}

void EventsGlobal(globals *all)
{
  while (SDL_PollEvent(&(all->event)))
  {
    if (all->event.type == SDL_QUIT)
      all->flags.quit = true;
    if (all->event.type == SDL_KEYDOWN)
      switch (all->event.key.keysym.sym)
      {
      case SDLK_KP_PLUS:
        if ((all->flags.mapMode == 0) && ((all->timeMods.timeControl / all->timeMods.scale) < 5.0))
        {
          all->x += 0.1;
          all->timeMods.timeControl = exp(all->x);
          all->flags.modsChange = true;
        }
        break;
      case SDLK_KP_MINUS:
        if ((all->flags.mapMode == 0) && ((all->timeMods.timeControl / all->timeMods.scale) >  0.001))
        {
          all->x -= 0.1;
          all->timeMods.timeControl = exp(all->x);
          all->flags.modsChange = true;
        }
        break;
      case SDLK_INSERT:
        if (all->flags.menu == false)
        {
          all->flags.menu = true;
          all->flags.pause = true;
          all->flags.menuMode = 8;
          all->flags.inGameMenu = true;
        }
      case SDLK_ESCAPE:
        if (all->flags.menu == false)
        {
          all->flags.menu = true;
          all->flags.pause = true;
          all->flags.menuMode = 1;
          all->flags.inGameMenu = true;
        }
        else
          switch (all->flags.menuMode)
          {
          case 0:
            all->flags.quit = true;
            break;
          case 1:
          case 4:
          case 5:
          case 6:
          case 7:
            all->currEvent.stage = 1;
            _ApplyDestroyEvent(all);
            break;
          case 2:
            all->flags.menuMode = 1;
            break;
          case 3:
            if (all->flags.inGameMenu)
              all->flags.menuMode = 1;
            else
              all->flags.menuMode = 0;
            break;
          case 8:
            _GameStart(all, 2, 0);
          case 9:
            all->flags.menuMode = 0;
            break;
          }
        break;
      case SDLK_m:
        if (all->flags.mapMode == 0)
          all->flags.mapMode = 2;
        break;
      case SDLK_t:
        if (all->flags.mapMode == 0)
          _Teleport(all);
        break;
      case SDLK_s:
        all->flags.menu = true;
        all->flags.menuMode = 8;
        all->flags.pause = true;
        all->flags.inGameMenu = true;
        break;
      case SDLK_DOWN:
        if (all->ship.enginePowerLevel)
          all->ship.enginePowerLevel += -1;
        break;
      case SDLK_UP:
        if (all->ship.enginePowerLevel < 6)
          all->ship.enginePowerLevel += 1;
        break;
      case SDLK_SPACE:
        if ((!all->flags.menu))
        {
          if ((all->flags.mapMode == 1) && !(all->flags.fuelDeplete))
          {
            if (all->flags.destSet == true)
            {
              all->flags.globalMove = !all->flags.globalMove;
              all->flags.pause = !all->flags.pause;
            }
          }
          else
            all->flags.pause = !all->flags.pause;
        }
        break;
      case SDLK_HOME:
        all->ship.enginePowerLevel = 5;
        break;
      case SDLK_END:
        all->ship.enginePowerLevel = 0;
        break;
      case SDLK_c:
        all->ship.fuel = 1.0;
        all->ship.credits += 100000.0;
        all->flags.fuelDeplete = false;
        break;
      case SDLK_LSHIFT:
      case SDLK_RSHIFT:
        all->flags.shiftModifier = true;
        break;
      }
    if (all->event.type == SDL_KEYUP)
    {
      switch (all->event.key.keysym.sym)
      {
      case SDLK_m:
        if (all->flags.mapMode == 2)
          all->flags.mapMode = 0;
        break;
      case SDLK_LSHIFT:
      case SDLK_RSHIFT:
        all->flags.shiftModifier = false;
        break;
      }
    }
    if (all->event.type == SDL_MOUSEBUTTONDOWN)
    {
      switch (all->event.button.button)
      {
      case SDL_BUTTON_RIGHT:
        if (all->flags.mapMode == 0)
          all->ship.engineOn = true;
        break;
      }
    }
    if (all->event.type == SDL_MOUSEBUTTONUP)
    {
      switch (all->event.button.button)
      {
      case SDL_BUTTON_RIGHT:
        all->ship.engineOn = false;
        if (all->flags.mapMode == 1)
          _RightMouseClick(all);
        break;
      case SDL_BUTTON_LEFT:
        _LeftMouseClick(all);
        break;
      }
    }
    if (all->event.type == SDL_MOUSEMOTION)
    {
      if ((all->event.motion.x > 0) && (all->event.motion.x < SCREEN_WIDTH))
        all->mousePos.x = all->event.motion.x;
      if ((all->event.motion.y > 0) && (all->event.motion.y < SCREEN_HEIGHT))
        all->mousePos.y = all->event.motion.y;
    }
  }
  return;
}

void InitAll(globals *all)
{
  SDL_Init(SDL_INIT_EVERYTHING);
  TTF_Init();
  FILE *checkfile = NULL;
  savedata *gameload = NULL;
  char saveName[] = "save1.sav";
  memset(all, 0, sizeof(globals));
  fopen_s(&checkfile, "textsource.txt", "rt");
  if (checkfile)
    all->textSource = checkfile;
  else
    exit(1);
  all->saveSlots = (bool *)calloc(SAVE_SLOT_COUNT, sizeof(bool));
  fopen_s(&checkfile, "autosave.sav", "rb");
  if (checkfile)
  {
    *all->saveSlots = true;
    fclose(checkfile);
  }
  for (int i = 1; i < SAVE_SLOT_COUNT; ++i)
  {
    checkfile = NULL;
    saveName[4] = (char)(48 + i);
    fopen_s(&checkfile, saveName, "rb");
    if (checkfile)
    {
      all->saveSlots[i] = true;
      fclose(checkfile);
    }
  }
  all->flags.pause = true;
  all->flags.menu = true;
  all->flags.dateChange = true;
  all->timers.starter = _StartCounter(&(all->timers.PCFreq));
  srand(GetTickCount());
  all->mousePos = { SCREEN_WIDTH, SCREEN_HEIGHT };
  all->timeMods = { 1.0, 1.0, 0.0, 0.0 };
  all->window = SDL_CreateWindow("GTFO",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
  all->renderer = SDL_CreateRenderer(all->window, -1, 0);
  all->genFont = TTF_OpenFont("font.ttf", 80);
}

void DeInitAll(globals *all)
{
  FILE *init = NULL;
  savedata save;
  if (all->system)
  {
    save.questMain = all->quests;
    save.globalStuff = all->timeMods;
    save.ship = all->ship;
    save.system = all->system;
    save.currDate = all->currDate;
    save.mapMode = all->flags.mapMode;
    save.systemCount = all->flags.systemCount;
    if (all->system)
      _GameSave(save, 0);
    _SystemDestroy(all->system);
    if (all->quests)
      _DestroyQuests;
  }
  if (all->questsTextureGlobal)
    SDL_DestroyTexture(all->questsTextureGlobal);
  if (all->questTexturePlanet)
    SDL_DestroyTexture(all->questTexturePlanet);
  if (all->eventTexture)
    SDL_DestroyTexture(all->eventTexture);
  if (all->info)
    SDL_DestroyTexture(all->info);
  if (all->passiveInfo)
    SDL_DestroyTexture(all->passiveInfo);
  SDL_DestroyRenderer(all->renderer);
  SDL_DestroyWindow(all->window);
  TTF_CloseFont(all->genFont);
  free(all->saveSlots);
  TTF_Quit();
  SDL_Quit();
}

/* Start modes:
 0 - new game
 1 - load from save
 2 - continue
 3 - new system
 */
void _GameStart(globals *all, int mode, int slot)
{
  double dtemp = 0.0;
  savedata *gameload = NULL;
  all->flags.menuMode = 1;
  all->flags.menu = false;
  all->flags.shipCrashed = false;
  all->flags.destSet = false;
  all->flags.pause = true;
  all->flags.globalMove = false;
  all->flags.inGameMenu = false;
  all->infoTarget = NULL;
  all->passiveInfoTarg = NULL;
  all->flags.infoChange = true;
  all->flags.passInfChange = true;
  all->flags.questChange = true;
  if (all->info)
  {
    SDL_DestroyTexture(all->info);
    all->info = NULL;
  }
  if (all->passiveInfo)
  {
    SDL_DestroyTexture(all->passiveInfo);
    all->passiveInfo = NULL;
  }
  all->currInterface = NULL;
  all->flags.scrollPos = 0;
  all->flags.scroll = 0;
  all->flags.dateChange = true;
  if (all->currEvent.type = 2)
    _ApplyDestroyEvent(all);
  switch (mode)
  {
  case 0:
    if (all->system)
    {
      _SystemDestroy(all->system);
      all->system = NULL;
    }
    if (all->quests)
      _DestroyQuests(all);
    all->quests = NULL;
    memset(&all->ship, 0, sizeof(playerShip_t));
    all->ship.globalSpeed = 5.0;
    all->ship.bodyProperties.mass = 1000.0;
    all->ship.maxMass = 1200.0;
    all->ship.turnRate = 15;
    all->ship.credits = 1000.0;
    all->ship.fuel = 1.0;
    all->flags.mapMode = 0;
    all->system = _SystemGen(&(all->ship), true);
    all->flags.systemCount = 1;
    all->currDate = { 0.0, 1, 2451 };
    all->timeMods.timeControl = 1.0;
    all->x = 0.0;
    break;
  case 1:
    if (all->system)
    {
      _SystemDestroy(all->system);
      all->system = NULL;
    }
    if (all->quests)
      _DestroyQuests(all);
    all->quests = NULL;
    _GameLoad(slot, &gameload, all->textSource);
    all->system = gameload->system;
    all->ship = gameload->ship;
    all->quests = gameload->questMain;
    all->timeMods = gameload->globalStuff;
    all->currDate = gameload->currDate;
    all->flags.mapMode = gameload->mapMode;
    all->flags.systemCount = gameload->systemCount;
    if (all->flags.mapMode == 2)
      all->flags.mapMode = 0;
    free(gameload);
    break;
  case 2:
    return;
  case 3:
    if (all->system)
    {
      _SystemDestroy(all->system);
      all->system = NULL;
    }
    if (all->quests)
      _DestroyQuests(all);
    all->quests = NULL;
    all->system = _SystemGen(&(all->ship), false);
    ++all->flags.systemCount;
    if (all->flags.systemCount == 5)
      _GenerateQuest(all, NULL, 0, true);
    break;
  }
  if (all->flags.mapMode == 0)
  {
    all->timeMods.scale = 10 / sqrt(all->ship.well->bodyProperties.rad);
    if (all->timeMods.scale < 0.5)
    {
      dtemp = log(all->timeMods.scale);
      all->x = (int)(dtemp * 10) / 10.0;
    }
    else
      all->x = 1.0;
    all->timeMods.timeControl = exp(all->x);
    all->flags.modsChange = true;
  }
  return;
}

void _ApplyDestroyEvent(globals *all)
{
  bool check = false;
  switch (all->currEvent.type)
  {
  case 0:
    switch (all->currEvent.subtype)
    {
    case 0:
      all->ship.fuel -= 0.1 * (1 + 0.1 * (rand() % 20 - 10));
      if (all->ship.fuel < 0.0)
        all->ship.fuel = 0.0;
      break;
    case 1:
      all->ship.foodQ -= 1 + rand() % 4;
      if (all->ship.foodQ < 0)
        all->ship.foodQ = 0;
      break;
    case 2:
      while (!check)
      {
        if ((!all->ship.foodQ) && (!all->ship.medQ) && (!all->ship.minQ) && (!all->ship.techQ) && (!all->ship.weapQ))
          break;
        switch (rand() % 5)
        {
        case 0:
          if (all->ship.foodQ)
            check = true;
          all->ship.foodQ -= 2 + rand() % 5;
          if (all->ship.foodQ < 0)
            all->ship.foodQ = 0;
          break;
        case 1:
          if (all->ship.medQ)
            check = true;
          all->ship.medQ -= 2 + rand() % 5;
          if (all->ship.medQ < 0)
            all->ship.medQ = 0;
          break;
        case 2:
          if (all->ship.minQ)
            check = true;
          all->ship.minQ -= 2 + rand() % 5;
          if (all->ship.minQ < 0)
            all->ship.minQ = 0;
          break;
        case 3:
          if (all->ship.weapQ)
            check = true;
          all->ship.weapQ -= 2 + rand() % 5;
          if (all->ship.weapQ < 0)
            all->ship.weapQ = 0;
          break;
        case 4:
          if (all->ship.techQ)
            check = true;
          all->ship.techQ -= 2 + rand() % 5;
          if (all->ship.techQ < 0)
            all->ship.techQ = 0;
          break;
        }
      }
      check = false;
      break;
    case 3:
      all->ship.credits -= 50 * (1 + 0.1 * (rand() % 80 - 30));
      break;
    case 4:
      all->ship.fuel += 0.1 * (1 + 0.1 * (rand() % 20 - 10));
      all->flags.fuelDeplete = false;
      break;
    case 5:
      switch (rand() % 5)
      {
      case 0:
        all->ship.foodQ += 2 + rand() % 5;
        break;
      case 1:
        all->ship.medQ += 2 + rand() % 5;
        break;
      case 2:
        all->ship.minQ += 2 + rand() % 5;
        break;
      case 3:
        all->ship.weapQ += 2 + rand() % 5;
        break;
      case 4:
        all->ship.techQ += 2 + rand() % 5;
        break;
      }
      break;
    case 6:
      all->ship.credits += 100 * (1 + 0.2 * (rand() % 80 - 30));
      break;
    }
    break;
  case 1:
    if (all->currEvent.location == all->infoTarget)
      all->flags.infoChange = true;
    if (all->currEvent.location == all->passiveInfoTarg)
      all->flags.passInfChange = true;
    switch (all->currEvent.subtype)
    {
    case 0: // Re-generating prices
      if (all->currEvent.location)
        switch (all->currEvent.location->eco)
        {
        case 0:
          all->currEvent.location->foodPB = (FOOD_MAX_PRICE + FOOD_MIN_PRICE) / 2 * (1 + 0.015 * (rand() % 60 - 30));
          all->currEvent.location->foodPS = all->currEvent.location->foodPB + (FOOD_MAX_PRICE + FOOD_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
          all->currEvent.location->medPB = (MED_MAX_PRICE + MED_MIN_PRICE) / 2 * (1 + 0.015 * (rand() % 60 - 30));
          all->currEvent.location->medPS = all->currEvent.location->medPB + (MED_MAX_PRICE + MED_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
          all->currEvent.location->minPB = (3 * MIN_MAX_PRICE + MIN_MIN_PRICE) / 4 * (1 + 0.015 * (rand() % 60 - 30));
          all->currEvent.location->minPS = all->currEvent.location->minPB + (MIN_MAX_PRICE + MIN_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
          all->currEvent.location->weapPB = (WEAP_MAX_PRICE + 3 * WEAP_MIN_PRICE) / 4 * (1 + 0.015 * (rand() % 60 - 30));
          all->currEvent.location->weapPS = all->currEvent.location->weapPB + (WEAP_MAX_PRICE + WEAP_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
          all->currEvent.location->techPB = (TECH_MAX_PRICE + 3 * TECH_MIN_PRICE) / 4 * (1 + 0.015 * (rand() % 60 - 30));
          all->currEvent.location->techPS = all->currEvent.location->techPB + (TECH_MAX_PRICE + TECH_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
          all->currEvent.location->fuelPrice = (FUEL_MAX_PRICE + 3 * FUEL_MIN_PRICE) / 4 * (1 + 0.015 * (rand() % 60 - 30));
          break;
        case 1:
          all->currEvent.location->foodPB = (FOOD_MAX_PRICE + 3 * FOOD_MIN_PRICE) / 4 * (1 + 0.015 * (rand() % 60 - 30));
          all->currEvent.location->foodPS = all->currEvent.location->foodPB + (FOOD_MAX_PRICE + 2 * FOOD_MIN_PRICE) / 3 * (0.03 * (5 + rand() % 20));
          all->currEvent.location->medPB = (MED_MAX_PRICE + 2 * MED_MIN_PRICE) / 3 * (1 + 0.015 * (rand() % 60 - 30));
          all->currEvent.location->medPS = all->currEvent.location->medPB + (MED_MAX_PRICE + MED_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
          all->currEvent.location->minPB = (MIN_MAX_PRICE + MIN_MIN_PRICE) / 2 * (1 + 0.015 * (rand() % 60 - 30));
          all->currEvent.location->minPS = all->currEvent.location->minPB + (MIN_MAX_PRICE + MIN_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
          all->currEvent.location->weapPB = (2 * WEAP_MAX_PRICE + WEAP_MIN_PRICE) / 3 * (1 + 0.015 * (rand() % 60 - 30));
          all->currEvent.location->weapPS = all->currEvent.location->weapPB + (WEAP_MAX_PRICE + WEAP_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
          all->currEvent.location->techPB = (4 * TECH_MAX_PRICE + TECH_MIN_PRICE) / 5 * (1 + 0.015 * (rand() % 60 - 30));
          all->currEvent.location->techPS = all->currEvent.location->techPB + (2 * TECH_MAX_PRICE + TECH_MIN_PRICE) / 3 * (0.03 * (5 + rand() % 20));
          all->currEvent.location->fuelPrice = (3 * FUEL_MAX_PRICE + FUEL_MIN_PRICE) / 4 * (1 + 0.015 * (rand() % 60 - 30));
          break;
        case 2:
          all->currEvent.location->foodPB = (FOOD_MAX_PRICE + FOOD_MIN_PRICE) / 2 * (1 + 0.015 * (rand() % 60 - 30));
          all->currEvent.location->foodPS = all->currEvent.location->foodPB + (FOOD_MAX_PRICE + FOOD_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
          all->currEvent.location->medPB = (MED_MAX_PRICE + MED_MIN_PRICE) / 2 * (1 + 0.015 * (rand() % 60 - 30));
          all->currEvent.location->medPS = all->currEvent.location->medPB + (MED_MAX_PRICE + MED_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
          all->currEvent.location->minPB = (MIN_MAX_PRICE + MIN_MIN_PRICE) / 2 * (1 + 0.015 * (rand() % 60 - 30));
          all->currEvent.location->minPS = all->currEvent.location->minPB + (MIN_MAX_PRICE + MIN_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
          all->currEvent.location->weapPB = (WEAP_MAX_PRICE + WEAP_MIN_PRICE) / 2 * (1 + 0.015 * (rand() % 60 - 30));
          all->currEvent.location->weapPS = all->currEvent.location->weapPB + (WEAP_MAX_PRICE + WEAP_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
          all->currEvent.location->techPB = (TECH_MAX_PRICE + TECH_MIN_PRICE) / 2 * (1 + 0.015 * (rand() % 60 - 30));
          all->currEvent.location->techPS = all->currEvent.location->techPB + (TECH_MAX_PRICE + TECH_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
          all->currEvent.location->fuelPrice = (FUEL_MAX_PRICE + FUEL_MIN_PRICE) / 2 * (1 + 0.015 * (rand() % 60 - 30));
          break;
        case 3:
          all->currEvent.location->foodPB = (FOOD_MAX_PRICE + 2 * FOOD_MIN_PRICE) / 3 * (1 + 0.015 * (rand() % 60 - 30));
          all->currEvent.location->foodPS = all->currEvent.location->foodPB + (FOOD_MAX_PRICE + FOOD_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
          all->currEvent.location->medPB = (MED_MAX_PRICE + 2 * MED_MIN_PRICE) / 3 * (1 + 0.015 * (rand() % 60 - 30));
          all->currEvent.location->medPS = all->currEvent.location->medPB + (MED_MAX_PRICE + MED_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
          all->currEvent.location->minPB = (2 * MIN_MAX_PRICE + MIN_MIN_PRICE) / 3 * (1 + 0.015 * (rand() % 60 - 30));
          all->currEvent.location->minPS = all->currEvent.location->minPB + (MIN_MAX_PRICE + MIN_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
          all->currEvent.location->weapPB = (WEAP_MAX_PRICE + 2 * WEAP_MIN_PRICE) / 3 * (1 + 0.015 * (rand() % 60 - 30));
          all->currEvent.location->weapPS = all->currEvent.location->weapPB + (WEAP_MAX_PRICE + WEAP_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
          all->currEvent.location->techPB = (TECH_MAX_PRICE + 2 * TECH_MIN_PRICE) / 3 * (1 + 0.015 * (rand() % 60 - 30));
          all->currEvent.location->techPS = all->currEvent.location->techPB + (TECH_MAX_PRICE + TECH_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
          all->currEvent.location->fuelPrice = (FUEL_MAX_PRICE + FUEL_MIN_PRICE) / 2 * (1 + 0.015 * (rand() % 60 - 30));
          break;
        }
      break;
    case 1:
      all->currEvent.location->foodQ /= 2;
      all->currEvent.location->medQ /= 2;
      all->currEvent.location->foodPB = all->currEvent.location->foodPB + (FOOD_MAX_PRICE + FOOD_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      all->currEvent.location->foodPS = all->currEvent.location->foodPB + (FOOD_MAX_PRICE + FOOD_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      all->currEvent.location->medPB = all->currEvent.location->medPB + (MED_MAX_PRICE + MED_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      all->currEvent.location->medPS = all->currEvent.location->medPB + (MED_MAX_PRICE + MED_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      break;
    case 2:
      all->currEvent.location->weapQ /= 2;
      all->currEvent.location->weapPB = all->currEvent.location->weapPB + (WEAP_MAX_PRICE + WEAP_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      all->currEvent.location->weapPS = all->currEvent.location->weapPB + (WEAP_MAX_PRICE + WEAP_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      break;
    case 3:
      all->currEvent.location->minQ /= 2;
      all->currEvent.location->techQ *= 1.5;
      all->currEvent.location->minPB = all->currEvent.location->minPB + (MIN_MAX_PRICE + MIN_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      all->currEvent.location->minPS = all->currEvent.location->minPB + (MIN_MAX_PRICE + MIN_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      all->currEvent.location->techPS = all->currEvent.location->techPS - (TECH_MAX_PRICE + TECH_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      all->currEvent.location->techPB = all->currEvent.location->techPS - (TECH_MAX_PRICE + TECH_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      break;
    case 4:
      all->currEvent.location->foodQ *= 2;
      all->currEvent.location->medQ *= 2;
      all->currEvent.location->foodPS = all->currEvent.location->foodPS - (FOOD_MAX_PRICE + FOOD_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      all->currEvent.location->foodPB = all->currEvent.location->foodPS - (FOOD_MAX_PRICE + FOOD_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      all->currEvent.location->medPS = all->currEvent.location->medPS - (MED_MAX_PRICE + MED_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      all->currEvent.location->medPB = all->currEvent.location->medPS - (MED_MAX_PRICE + MED_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      all->currEvent.location->techPB = all->currEvent.location->techPB + (TECH_MAX_PRICE + TECH_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      all->currEvent.location->techPS = all->currEvent.location->techPB + (TECH_MAX_PRICE + TECH_MIN_PRICE) / 2 * (0.03 * (5 + rand() % 20));
      break;
    case 5:
      all->currEvent.location->weapQ *= 2;
      all->currEvent.location->weapPS = all->currEvent.location->weapPS - (2 * WEAP_MAX_PRICE + WEAP_MIN_PRICE) / 3 * (0.03 * (5 + rand() % 20));
      all->currEvent.location->weapPB = all->currEvent.location->weapPS - (2 * WEAP_MAX_PRICE + WEAP_MIN_PRICE) / 3 * (0.03 * (5 + rand() % 20));
      break;
    }
    break;
  case 2:
    if (all->currEvent.stage == 0)
    {
      all->flags.menuMode = 4;
      all->currEvent.stage = 1;
      return;
    }
    else
    {
      if (all->currEvent.location)
      {
        if (all->currEvent.location->name)
          free(all->currEvent.location->name);
        free(all->currEvent.location);
      }
    }
    break;
  }
  all->currEvent = {};
  SDL_DestroyTexture(all->eventTexture);
  all->eventTexture = NULL;
  _GameStart(all, 2, 0);
  return;
}

planet_t *_GenerateTrader(int type)
{
  planet_t *output = (planet_t *)calloc(1, sizeof(planet_t));
  output->name = (char *)calloc(20, sizeof(char));
  switch (type)
  {
  case 0:
    strcpy_s(output->name, sizeof(char) * 7, "Trader");
    output->foodQ = rand() % 15;
    output->medQ = rand() % 15;
    output->minQ = rand() % 15;
    output->weapQ = rand() % 15;
    output->techQ = rand() % 15;
    output->foodPB = (FOOD_MAX_PRICE + FOOD_MIN_PRICE) / 2 * (1 +  0.02 * (rand() % 50 - 50));
    output->foodPS = output->foodPB + (FOOD_MAX_PRICE + FOOD_MIN_PRICE) / 2 * (0.03 * (rand() % 30));
    output->medPB = (MED_MAX_PRICE + MED_MIN_PRICE) / 2 * (1 + 0.02 * (rand() % 50 - 50));
    output->medPS = output->medPB + (MED_MAX_PRICE + MED_MIN_PRICE) / 2 * (0.03 * (rand() % 30));
    output->minPB = (MIN_MAX_PRICE + MIN_MIN_PRICE) / 2 * (1 + 0.02 * (rand() % 50 - 50));
    output->minPS = output->minPB + (MIN_MAX_PRICE + MIN_MIN_PRICE) / 2 * (0.03 * (rand() % 30));
    output->weapPB = (WEAP_MAX_PRICE + WEAP_MIN_PRICE) / 2 * (1 + 0.02 * (rand() % 50 - 50));
    output->weapPS = output->weapPB + (WEAP_MAX_PRICE + WEAP_MIN_PRICE) / 2 * (0.03 * (rand() % 30));
    output->techPB = (TECH_MAX_PRICE + TECH_MIN_PRICE) / 2 * (1 + 0.02 * (rand() % 50 - 50));
    output->techPS = output->techPB + (TECH_MAX_PRICE + TECH_MIN_PRICE) / 2 * (0.03 * (rand() % 30));
    break;
  case 1:
    strcpy_s(output->name, sizeof(char) * 7, "Tanker");
    output->fuelPrice = FUEL_MAX_PRICE - 10.0 * (1 + 0.2 * (rand() % 50 - 35));
    break;
  case 2:
    strcpy_s(output->name, sizeof(char) * 5, "Ship");
    output->fuelPrice = -(FUEL_MAX_PRICE - 5.0 * (1 + 0.1 * (rand() % 50 - 35)));
    break;
  }
  output->trader = true;
  return output;
}

void _GenerateWorldEvent(globals *all)
{
  memset(&all->currEvent, 0, sizeof(worldEvent_t));
  int type = rand() % 3;
  if (all->currEvent.text)
  {
    free(all->currEvent.text);
    all->currEvent.text = NULL;
  }
  if ((type == 1) && (_FindRandomPlanet(all->system, HABITABLE, NULL) == NULL))
    type = 2;
  all->currEvent.type = type;
  all->flags.menuMode = 6;
  switch (type)
  {
  case 0: // ship change
    all->currEvent.subtype = rand() % 8;
    break;
  case 1: // planet change
    all->currEvent.subtype = rand() % 6;
    all->currEvent.location = _FindRandomPlanet(all->system, HABITABLE, NULL);
    break;
  case 2: // trade encounter
    all->currEvent.subtype = rand() % 3;
    all->currEvent.location = _GenerateTrader(all->currEvent.subtype);
    all->currInterface = all->currEvent.location;
    break;
  case 3: // a comet/asteroid
    break;
  }
  _GenerateEventText(&all->currEvent, all->textSource);
  all->flags.menu = true;
  all->flags.inGameMenu = true;
}

void _GenerateQuest(globals *all, planet_t *starter, int otherType, bool mainQuest)
{
  int excludedType = 0;
  quest_t *quest = NULL;
  planet_t *goal = NULL;
  PLANET_TYPE randPlanet = STAR;
  quest = (quest_t *)calloc(1, sizeof(quest_t));
  quest->starter = starter;
  if (mainQuest)
  {
    quest->type = 4;
    quest->subtype = 0;
  }
  else
    if (otherType)
      quest->type = rand() % 2 + 2 * (otherType == 2);
    else
      quest->type = rand() % 4;
  switch (quest->type)
  {
  case 0: // go to uninhabited and return
    randPlanet = (PLANET_TYPE)(2 + 3 * (rand() % 2));
    quest->subtype = rand() % 2;
    break;
  case 1: // go to uninhabited and do stuff
    randPlanet = (PLANET_TYPE)(2 + 3 * (rand() % 2));
    quest->subtype = rand() % 2;
    break;
  case 2: // go to habited and return
    randPlanet = (PLANET_TYPE)(4 + 3 * (rand() % 2));
    quest->subtype = 0;
    break;
  case 3: // deliver stuff to habited
    randPlanet = (PLANET_TYPE)(4 + 3 * (rand() % 2));
    quest->subtype = rand() % 2;
    break;
  case 4:
    randPlanet = (PLANET_TYPE)(1 + (rand() % 7));
    quest->stage = 1;
    break;
  }
  do
  {
    quest->goal = _FindRandomPlanet(all->system, randPlanet, quest->starter);
    randPlanet = (PLANET_TYPE)(1 + (rand() % 7));
  } while (quest->goal == NULL);
  quest->duration = 80 + (rand() % 70 - 20);
  if ((quest->type == 0) || (quest->type == 2))
    quest->duration += 30;
  if (quest->goal == NULL)
  {
    if (quest->type < 2)
      excludedType = 1;
    else
      excludedType = 2;
    if (excludedType == otherType)
    {
      all->flags.disableQuests = true;
      return;
    }
    free(quest);
    _GenerateQuest(all, starter, excludedType, false);
  }
  _GenerateQuestText(quest, all->textSource);
  quest->reward = 500.0 * (2 + 0.02 * (rand() % 80 - 20));
  quest->start = all->currDate;
  quest->end = quest->start;
  quest->end.day += quest->duration;
  if (quest->end.day > 300)
  {
    quest->end.day = quest->end.day % 300;
    ++quest->end.year;
  }
  if (starter)
    starter->currQuest = quest;
  if (all->quests == NULL)
  {
    all->quests = quest;
    return;
  }
  if (all->quests->next == NULL)
  {
    quest->next = all->quests;
    quest->prev = all->quests;
    all->quests->next = quest;
    all->quests->prev = quest;
    return;
  }
  quest->prev = all->quests->prev;
  quest->next = all->quests;
  all->quests->prev->next = quest;
  all->quests->prev = quest;
  return;
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
void _LeftMouseClick(globals *all)
{
  int tradeTemp = 0;
  quest_t *currQuest = all->quests;
  if (all->flags.menu)
  {
    int height = SCREEN_HEIGHT / 13;
    switch (all->flags.menuMode)
    {
    case 0:
    {
      if ((all->mousePos.x > SCREEN_WIDTH / 2 - 200) && (all->mousePos.x < SCREEN_WIDTH / 2 + 200))
      {
        if ((all->mousePos.y > height * 2) && (all->mousePos.y < height * 3))
        {
          _GameStart(all, 0, 0);
          break;
        }
        if ((all->mousePos.y > height * 4) && (all->mousePos.y < height * 5))
        {
          if (*all->saveSlots == true)
            _GameStart(all, 1, 0);
          break;
        }
        if ((all->mousePos.y > height * 8) && (all->mousePos.y < height * 9))
        {
          all->flags.menuMode = 3;
          break;
        }
        if ((all->mousePos.y > height * 10) && (all->mousePos.y < height * 11))
        {
          all->flags.quit = true;
          break;
        }
      }
      break;
    }
    case 1:
    {
      if ((all->mousePos.x > SCREEN_WIDTH / 2 - 200) && (all->mousePos.x < SCREEN_WIDTH / 2 + 200))
      {
        if ((all->mousePos.y > height * 2) && (all->mousePos.y < height * 3))
        {
          _GameStart(all, 0, 0);
          break;
        }
        if ((all->mousePos.y > height * 4) && (all->mousePos.y < height * 5))
        {
          _GameStart(all, 2, 0);
          break;
        }
        if ((all->mousePos.y > height * 6) && (all->mousePos.y < height * 7))
        {
          all->flags.menuMode = 2;
          break;
        }
        if ((all->mousePos.y > height * 8) && (all->mousePos.y < height * 9))
        {
          all->flags.menuMode = 3;
          break;
        }
        if ((all->mousePos.y > height * 10) && (all->mousePos.y < height * 11))
        {
          all->flags.quit = true;
          break;
        }
      }
      break;
    }
    case 2:
    {
      if ((all->mousePos.x > SCREEN_WIDTH / 2 - 200) && (all->mousePos.x < SCREEN_WIDTH / 2 + 200))
      {
        height = SCREEN_HEIGHT / 15;
        savedata save;
        save.systemCount = all->flags.systemCount;
        save.questMain = all->quests;
        save.globalStuff = all->timeMods;
        save.ship = all->ship;
        save.system = all->system;
        save.currDate = all->currDate;
        save.mapMode = all->flags.mapMode;
        if ((all->mousePos.y > height * 2) && (all->mousePos.y < height * 3))
        {
          _GameSave(save, 1);
          all->saveSlots[1] = true;
          break;
        }
        if ((all->mousePos.y > height * 4) && (all->mousePos.y < height * 5))
        {
          _GameSave(save, 2);
          all->saveSlots[2] = true;
          break;
        }
        if ((all->mousePos.y > height * 6) && (all->mousePos.y < height * 7))
        {
          _GameSave(save, 3);
          all->saveSlots[3] = true;
          break;
        }
        if ((all->mousePos.y > height * 8) && (all->mousePos.y < height * 9))
        {
          _GameSave(save, 4);
          all->saveSlots[4] = true;
          break;
        }
        if ((all->mousePos.y > height * 10) && (all->mousePos.y < height * 11))
        {
          _GameSave(save, 5);
          all->saveSlots[5] = true;
          break;
        }
        if ((all->mousePos.y > height * 12) && (all->mousePos.y < height * 13))
        {
          all->flags.menuMode = 1;
          break;
        }
      }
      break;
    }
    case 3:
    {
      if ((all->mousePos.x > SCREEN_WIDTH / 2 - 200) && (all->mousePos.x < SCREEN_WIDTH / 2 + 200))
      {
        height = SCREEN_HEIGHT / 17;
        if ((all->mousePos.y > height * 2) && (all->mousePos.y < height * 3))
        {
          if (all->saveSlots[0] == true)
            _GameStart(all, 1, 0);
          break;
        }
        if ((all->mousePos.y > height * 4) && (all->mousePos.y < height * 5))
        {
          if (all->saveSlots[1] == true)
            _GameStart(all, 1, 1);
          break;
        }
        if ((all->mousePos.y > height * 6) && (all->mousePos.y < height * 7))
        {
          if (all->saveSlots[2] == true)
            _GameStart(all, 1, 2);
          break;
        }
        if ((all->mousePos.y > height * 8) && (all->mousePos.y < height * 9))
        {
          if (all->saveSlots[3] == true)
            _GameStart(all, 1, 3);
          break;
        }
        if ((all->mousePos.y > height * 10) && (all->mousePos.y < height * 11))
        {
          if (all->saveSlots[4] == true)
            _GameStart(all, 1, 4);
          break;
        }
        if ((all->mousePos.y > height * 12) && (all->mousePos.y < height * 13))
        {
          if (all->saveSlots[5] == true)
            _GameStart(all, 1, 5);
          break;
        }
        if ((all->mousePos.y > height * 14) && (all->mousePos.y < height * 15))
        {
          if (all->flags.inGameMenu)
            all->flags.menuMode = 1;
          else
            all->flags.menuMode = 0;
          break;
        }
      }
    }
    case 4:
    {
      if (all->currInterface)
      {
        if ((all->mousePos.x < SCREEN_WIDTH) && (all->mousePos.x > SCREEN_WIDTH - 30))
        {
          if ((all->mousePos.y < 30) && (all->mousePos.y > 0))
          {
            _GameStart(all, 2, 0);
            break;
          }
          if ((all->mousePos.y < 60) && (all->mousePos.y > 40))
            if ((all->currInterface->foodQ) && (all->ship.credits >= all->currInterface->foodPS))
              if (all->flags.shiftModifier)
              {
                tradeTemp = (int)(all->ship.credits / all->currInterface->foodPS);
                if (tradeTemp > all->currInterface->foodQ)
                  tradeTemp = all->currInterface->foodQ;
                if (tradeTemp > (int)(all->ship.maxMass - all->ship.bodyProperties.mass))
                  tradeTemp = (int)(all->ship.maxMass - all->ship.bodyProperties.mass);
                if (tradeTemp >= 5)
                {
                  all->ship.foodQ += 5;
                  all->currInterface->foodQ -= 5;
                  all->ship.credits -= all->currInterface->foodPS * 5;
                  all->ship.bodyProperties.mass += 5;
                }
                else
                {
                  all->ship.foodQ += tradeTemp;
                  all->currInterface->foodQ -= tradeTemp;
                  all->ship.credits -= all->currInterface->foodPS * tradeTemp;
                  all->ship.bodyProperties.mass += tradeTemp;
                }
              }
              else
              {
                ++all->ship.foodQ;
                --all->currInterface->foodQ;
                all->ship.credits -= all->currInterface->foodPS;
                ++all->ship.bodyProperties.mass;
              }
          if ((all->mousePos.y < 100) && (all->mousePos.y > 80))
            if ((all->currInterface->medQ) && (all->ship.credits >= all->currInterface->medPS))
              if (all->flags.shiftModifier)
              {
                tradeTemp = (int)(all->ship.credits / all->currInterface->medPS);
                if (tradeTemp > all->currInterface->medQ)
                  tradeTemp = all->currInterface->medQ;
                if (tradeTemp > (int)(all->ship.maxMass - all->ship.bodyProperties.mass))
                  tradeTemp = (int)(all->ship.maxMass - all->ship.bodyProperties.mass);
                if (tradeTemp >= 5)
                {
                  all->ship.medQ += 5;
                  all->currInterface->medQ -= 5;
                  all->ship.credits -= all->currInterface->medPS * 5;
                  all->ship.bodyProperties.mass += 5;
                }
                else
                {
                  all->ship.medQ += tradeTemp;
                  all->currInterface->medQ -= tradeTemp;
                  all->ship.credits -= all->currInterface->medPS * tradeTemp;
                  all->ship.bodyProperties.mass += tradeTemp;
                }
              }
              else
              {
                ++all->ship.medQ;
                --all->currInterface->medQ;
                all->ship.credits -= all->currInterface->medPS;
                ++all->ship.bodyProperties.mass;
              }
          if ((all->mousePos.y < 140) && (all->mousePos.y > 120))
            if ((all->currInterface->minQ) && (all->ship.credits >= all->currInterface->minPS))
              if (all->flags.shiftModifier)
              {
                tradeTemp = (int)(all->ship.credits / all->currInterface->minPS);
                if (tradeTemp > all->currInterface->minQ)
                  tradeTemp = all->currInterface->minQ;
                if (tradeTemp > (int)(all->ship.maxMass - all->ship.bodyProperties.mass))
                  tradeTemp = (int)(all->ship.maxMass - all->ship.bodyProperties.mass);
                if (tradeTemp >= 5)
                {
                  all->ship.minQ += 5;
                  all->currInterface->minQ -= 5;
                  all->ship.credits -= all->currInterface->minPS * 5;
                  all->ship.bodyProperties.mass += 5;
                }
                else
                {
                  all->ship.minQ += tradeTemp;
                  all->currInterface->minQ -= tradeTemp;
                  all->ship.credits -= all->currInterface->minPS * tradeTemp;
                  all->ship.bodyProperties.mass += tradeTemp;
                }
              }
              else
              {
                ++all->ship.minQ;
                --all->currInterface->minQ;
                all->ship.credits -= all->currInterface->minPS;
                ++all->ship.bodyProperties.mass;
              }
          if ((all->mousePos.y < 180) && (all->mousePos.y > 160))
            if ((all->currInterface->weapQ) && (all->ship.credits >= all->currInterface->weapPS))
              if (all->flags.shiftModifier)
              {
                tradeTemp = (int)(all->ship.credits / all->currInterface->weapPS);
                if (tradeTemp > all->currInterface->weapQ)
                  tradeTemp = all->currInterface->weapQ;
                if (tradeTemp > (int)(all->ship.maxMass - all->ship.bodyProperties.mass))
                  tradeTemp = (int)(all->ship.maxMass - all->ship.bodyProperties.mass);
                if (tradeTemp >= 5)
                {
                  all->ship.weapQ += 5;
                  all->currInterface->weapQ -= 5;
                  all->ship.credits -= all->currInterface->weapPS * 5;
                  all->ship.bodyProperties.mass += 5;
                }
                else
                {
                  all->ship.weapQ += tradeTemp;
                  all->currInterface->weapQ -= tradeTemp;
                  all->ship.credits -= all->currInterface->weapPS * tradeTemp;
                  all->ship.bodyProperties.mass += tradeTemp;
                }
              }
              else
              {
                ++all->ship.weapQ;
                --all->currInterface->weapQ;
                all->ship.credits -= all->currInterface->weapPS;
                ++all->ship.bodyProperties.mass;
              }
          if ((all->mousePos.y < 220) && (all->mousePos.y > 200))
            if ((all->currInterface->techQ) && (all->ship.credits >= all->currInterface->techPS))
              if (all->flags.shiftModifier)
              {
                tradeTemp = (int)(all->ship.credits / all->currInterface->techPS);
                if (tradeTemp > all->currInterface->medQ)
                  tradeTemp = all->currInterface->medQ;
                if (tradeTemp > (int)(all->ship.maxMass - all->ship.bodyProperties.mass))
                  tradeTemp = (int)(all->ship.maxMass - all->ship.bodyProperties.mass);
                if (tradeTemp >= 5)
                {
                  all->ship.techQ += 5;
                  all->currInterface->techQ -= 5;
                  all->ship.credits -= all->currInterface->techPS * 5;
                  all->ship.bodyProperties.mass += 5;
                }
                else
                {
                  all->ship.techQ += tradeTemp;
                  all->currInterface->techQ -= tradeTemp;
                  all->ship.credits -= all->currInterface->techPS * tradeTemp;
                  all->ship.bodyProperties.mass += tradeTemp;
                }
              }
              else
              {
                ++all->ship.techQ;
                --all->currInterface->techQ;
                all->ship.credits -= all->currInterface->techPS;
                ++all->ship.bodyProperties.mass;
              }
        }
        if ((all->mousePos.x < SCREEN_WIDTH - 30) && (all->mousePos.x > SCREEN_WIDTH - 60))
        {
          if ((all->mousePos.y < 60) && (all->mousePos.y > 40))
            if (all->ship.foodQ)
              if (all->flags.shiftModifier)
              {
                tradeTemp = all->ship.foodQ;
                if (tradeTemp >= 5)
                {
                  all->ship.foodQ -= 5;
                  all->currInterface->foodQ += 5;
                  all->ship.credits += all->currInterface->foodPB * 5;
                  all->ship.bodyProperties.mass -= 5;
                }
                else
                {
                  all->ship.foodQ -= tradeTemp;
                  all->currInterface->foodQ += tradeTemp;
                  all->ship.credits += all->currInterface->foodPB * tradeTemp;
                  all->ship.bodyProperties.mass -= tradeTemp;
                }
              }
              else
              {
                --all->ship.foodQ;
                ++all->currInterface->foodQ;
                all->ship.credits += all->currInterface->foodPB;
                --all->ship.bodyProperties.mass;
              }
          if ((all->mousePos.y < 100) && (all->mousePos.y > 80))
            if (all->ship.medQ)
              if (all->flags.shiftModifier)
              {
                tradeTemp = all->ship.medQ;
                if (tradeTemp >= 5)
                {
                  all->ship.medQ -= 5;
                  all->currInterface->medQ += 5;
                  all->ship.credits += all->currInterface->medPB * 5;
                  all->ship.bodyProperties.mass -= 5;
                }
                else
                {
                  all->ship.medQ -= tradeTemp;
                  all->currInterface->medQ += tradeTemp;
                  all->ship.credits += all->currInterface->medPB * tradeTemp;
                  all->ship.bodyProperties.mass -= tradeTemp;
                }
              }
              else
              {
                --all->ship.medQ;
                ++all->currInterface->medQ;
                all->ship.credits += all->currInterface->medPB;
                --all->ship.bodyProperties.mass;
              }
          if ((all->mousePos.y < 140) && (all->mousePos.y > 120))
            if (all->ship.minQ)
              if (all->flags.shiftModifier)
              {
                tradeTemp = all->ship.minQ;
                if (tradeTemp >= 5)
                {
                  all->ship.minQ -= 5;
                  all->currInterface->minQ += 5;
                  all->ship.credits += all->currInterface->minPB * 5;
                  all->ship.bodyProperties.mass -= 5;
                }
                else
                {
                  all->ship.minQ -= tradeTemp;
                  all->currInterface->minQ += tradeTemp;
                  all->ship.credits += all->currInterface->minPB * tradeTemp;
                  all->ship.bodyProperties.mass -= tradeTemp;
                }
              }
              else
              {
                --all->ship.minQ;
                ++all->currInterface->minQ;
                all->ship.credits += all->currInterface->minPB;
                --all->ship.bodyProperties.mass;
              }
          if ((all->mousePos.y < 180) && (all->mousePos.y > 160))
            if (all->ship.weapQ)
              if (all->flags.shiftModifier)
              {
                tradeTemp = all->ship.weapQ;
                if (tradeTemp >= 5)
                {
                  all->ship.weapQ -= 5;
                  all->currInterface->weapQ += 5;
                  all->ship.credits += all->currInterface->weapPB * 5;
                  all->ship.bodyProperties.mass -= 5;
                }
                else
                {
                  all->ship.weapQ -= tradeTemp;
                  all->currInterface->weapQ += tradeTemp;
                  all->ship.credits += all->currInterface->weapPB * tradeTemp;
                  all->ship.bodyProperties.mass -= tradeTemp;
                }
              }
              else
              {
                --all->ship.weapQ;
                ++all->currInterface->weapQ;
                all->ship.credits += all->currInterface->weapPB;
                --all->ship.bodyProperties.mass;
              }
          if ((all->mousePos.y < 220) && (all->mousePos.y > 200))
            if (all->ship.techQ)
              if (all->flags.shiftModifier)
              {
                tradeTemp = all->ship.techQ;
                if (tradeTemp >= 5)
                {
                  all->ship.techQ -= 5;
                  all->currInterface->techQ += 5;
                  all->ship.credits += all->currInterface->techPB * 5;
                  all->ship.bodyProperties.mass -= 5;
                }
                else
                {
                  all->ship.techQ -= tradeTemp;
                  all->currInterface->techQ += tradeTemp;
                  all->ship.credits += all->currInterface->techPB * tradeTemp;
                  all->ship.bodyProperties.mass -= tradeTemp;
                }
              }
              else
              {
                --all->ship.techQ;
                ++all->currInterface->techQ;
                all->ship.credits += all->currInterface->techPB;
                --all->ship.bodyProperties.mass;
              }
        }
        if ((all->mousePos.x < 230) && (all->mousePos.x > 30))
        {
          if ((all->mousePos.y < 80) && (all->mousePos.y > 30))
          {
            if (currQuest)
              do
              {
                if ((currQuest->goal == all->currInterface) && (currQuest->stage == 1))
                {
                  if (currQuest->type == 2)
                    currQuest->stage = 2;
                  if (currQuest->type == 3)
                    currQuest->stage = -1;
                  all->flags.questChange = true;
                  _CheckQuests(all);
                }
                currQuest = currQuest->next;
              } while ((currQuest) && (currQuest != all->quests));
          }
          if ((all->mousePos.y < 180) && (all->mousePos.y > 130))
          {
            currQuest = all->currInterface->currQuest;
            if (currQuest)
            {
              if (currQuest->stage == 0)
              {
                currQuest->stage = 1;
                currQuest->start = all->currDate;
                currQuest->end = currQuest->start;
                currQuest->end.day += currQuest->duration;
                if (currQuest->end.day > 300)
                {
                  currQuest->end.year += currQuest->end.day / 300;
                  currQuest->end.day = currQuest->end.day % 300;
                }
                all->flags.questChange = true;
              }
              if (currQuest->stage == 2)
              {
                currQuest->stage = -1;
                all->flags.questChange = true;
                _CheckQuests(all);
              }
            }
          }
        }
        if (all->currInterface->fuelPrice != 0.0)
        {
          if ((all->mousePos.y < 280) && (all->mousePos.y > 260))
          {
            if (all->currInterface->fuelPrice > 0.0)
            {
              if ((all->mousePos.x < SCREEN_WIDTH) && (all->mousePos.x > SCREEN_WIDTH - 50))
                if (all->ship.credits >= all->currInterface->fuelPrice)
                {
                  all->ship.credits -= all->currInterface->fuelPrice;
                  all->ship.fuel = 1.0;
                  all->flags.fuelDeplete = false;
                }
              if ((all->mousePos.x < SCREEN_WIDTH - 50) && (all->mousePos.x > SCREEN_WIDTH - 100))
                if (all->ship.credits >= all->currInterface->fuelPrice * 0.55)
                {
                  all->ship.credits -= all->currInterface->fuelPrice * 0.55;
                  all->ship.fuel += 0.5;
                  if (all->ship.fuel > 1.0)
                    all->ship.fuel = 1.0;
                  all->flags.fuelDeplete = false;
                }
              if ((all->mousePos.x < SCREEN_WIDTH - 100) && (all->mousePos.x > SCREEN_WIDTH - 150))
                if (all->ship.credits >= all->currInterface->fuelPrice * 0.3)
                {
                  all->ship.credits -= all->currInterface->fuelPrice * 0.3;
                  all->ship.fuel += 0.25;
                  if (all->ship.fuel > 1.0)
                    all->ship.fuel = 1.0;
                  all->flags.fuelDeplete = false;
                }
              if ((all->mousePos.x < SCREEN_WIDTH - 150) && (all->mousePos.x > SCREEN_WIDTH - 200))
                if (all->ship.credits >= all->currInterface->fuelPrice * 0.15)
                {
                  all->ship.credits -= all->currInterface->fuelPrice * 0.15;
                  all->ship.fuel += 0.1;
                  if (all->ship.fuel > 1.0)
                    all->ship.fuel = 1.0;
                  all->flags.fuelDeplete = false;
                }
            }
            else
            {
              if (all->ship.fuel)
              {
                if ((all->mousePos.x < SCREEN_WIDTH) && (all->mousePos.x > SCREEN_WIDTH - 50))
                {
                  all->ship.credits -= all->currInterface->fuelPrice;
                  all->ship.fuel = 0.0;
                  all->flags.fuelDeplete = false;
                }
                if ((all->mousePos.x < SCREEN_WIDTH - 50) && (all->mousePos.x > SCREEN_WIDTH - 100))
                {
                  all->ship.credits -= all->currInterface->fuelPrice * 0.55;
                  all->ship.fuel -= 0.5;
                  if (all->ship.fuel < 0.0)
                    all->ship.fuel = 0.0;
                  all->flags.fuelDeplete = false;
                }
                if ((all->mousePos.x < SCREEN_WIDTH - 100) && (all->mousePos.x > SCREEN_WIDTH - 150))
                {
                  all->ship.credits -= all->currInterface->fuelPrice * 0.3;
                  all->ship.fuel -= 0.25;
                  if (all->ship.fuel < 0.0)
                    all->ship.fuel = 0.0;
                  all->flags.fuelDeplete = false;
                }
                if ((all->mousePos.x < SCREEN_WIDTH - 150) && (all->mousePos.x > SCREEN_WIDTH - 200))
                {
                  all->ship.credits -= all->currInterface->fuelPrice * 0.15;
                  all->ship.fuel -= 0.1;
                  if (all->ship.fuel < 0.0)
                    all->ship.fuel = 0.0;
                  all->flags.fuelDeplete = false;
                }
              }
            }
          }
        }
      }
      break;
    }
    case 5:
    {
      if (all->currInterface)
      {
        if ((all->mousePos.x < SCREEN_WIDTH) && (all->mousePos.x > SCREEN_WIDTH - 30))
        {
          if ((all->mousePos.y < 30) && (all->mousePos.y > 0))
          {
            _GameStart(all, 2, 0);
            break;
          }
          if ((all->mousePos.y < 60) && (all->mousePos.y > 40))
            if (all->currInterface->foodQ)
              if (all->flags.shiftModifier)
              {
                tradeTemp = all->currInterface->foodQ;
                if (tradeTemp > (int)(all->ship.maxMass - all->ship.bodyProperties.mass))
                  tradeTemp = (int)(all->ship.maxMass - all->ship.bodyProperties.mass);
                if (tradeTemp >= 5)
                {
                  all->ship.foodQ += 5;
                  all->currInterface->foodQ -= 5;
                  all->ship.bodyProperties.mass += 5;
                }
                else
                {
                  all->ship.foodQ += tradeTemp;
                  all->currInterface->foodQ -= tradeTemp;
                  all->ship.bodyProperties.mass += tradeTemp;
                }
              }
              else
              {
                ++all->ship.foodQ;
                --all->currInterface->foodQ;
                ++all->ship.bodyProperties.mass;
              }
          if ((all->mousePos.y < 100) && (all->mousePos.y > 80))
            if (all->currInterface->medQ)
              if (all->flags.shiftModifier)
              {
                tradeTemp = all->currInterface->medQ;
                if (tradeTemp > (int)(all->ship.maxMass - all->ship.bodyProperties.mass))
                  tradeTemp = (int)(all->ship.maxMass - all->ship.bodyProperties.mass);
                if (tradeTemp >= 5)
                {
                  all->ship.medQ += 5;
                  all->currInterface->medQ -= 5;
                  all->ship.bodyProperties.mass += 5;
                }
                else
                {
                  all->ship.medQ += tradeTemp;
                  all->currInterface->medQ -= tradeTemp;
                  all->ship.bodyProperties.mass += tradeTemp;
                }
              }
              else
              {
                ++all->ship.medQ;
                --all->currInterface->medQ;
                ++all->ship.bodyProperties.mass;
              }
          if ((all->mousePos.y < 140) && (all->mousePos.y > 120))
            if (all->currInterface->minQ)
              if (all->flags.shiftModifier)
              {
                tradeTemp = all->currInterface->minQ;
                if (tradeTemp > (int)(all->ship.maxMass - all->ship.bodyProperties.mass))
                  tradeTemp = (int)(all->ship.maxMass - all->ship.bodyProperties.mass);
                if (tradeTemp >= 5)
                {
                  all->ship.minQ += 5;
                  all->currInterface->minQ -= 5;
                  all->ship.bodyProperties.mass += 5;
                }
                else
                {
                  all->ship.minQ += tradeTemp;
                  all->currInterface->minQ -= tradeTemp;
                  all->ship.bodyProperties.mass += tradeTemp;
                }
              }
              else
              {
                ++all->ship.minQ;
                --all->currInterface->minQ;
                ++all->ship.bodyProperties.mass;
              }
          if ((all->mousePos.y < 180) && (all->mousePos.y > 160))
            if (all->currInterface->weapQ)
              if (all->flags.shiftModifier)
              {
                tradeTemp = all->currInterface->weapQ;
                if (tradeTemp > (int)(all->ship.maxMass - all->ship.bodyProperties.mass))
                  tradeTemp = (int)(all->ship.maxMass - all->ship.bodyProperties.mass);
                if (tradeTemp >= 5)
                {
                  all->ship.weapQ += 5;
                  all->currInterface->weapQ -= 5;
                  all->ship.bodyProperties.mass += 5;
                }
                else
                {
                  all->ship.weapQ += tradeTemp;
                  all->currInterface->weapQ -= tradeTemp;
                  all->ship.bodyProperties.mass += tradeTemp;
                }
              }
              else
              {
                ++all->ship.weapQ;
                --all->currInterface->weapQ;
                ++all->ship.bodyProperties.mass;
              }
          if ((all->mousePos.y < 220) && (all->mousePos.y > 200))
            if (all->currInterface->techQ)
              if (all->flags.shiftModifier)
              {
                tradeTemp = all->currInterface->medQ;
                if (tradeTemp > (int)(all->ship.maxMass - all->ship.bodyProperties.mass))
                  tradeTemp = (int)(all->ship.maxMass - all->ship.bodyProperties.mass);
                if (tradeTemp >= 5)
                {
                  all->ship.techQ += 5;
                  all->currInterface->techQ -= 5;
                  all->ship.bodyProperties.mass += 5;
                }
                else
                {
                  all->ship.techQ += tradeTemp;
                  all->currInterface->techQ -= tradeTemp;
                  all->ship.bodyProperties.mass += tradeTemp;
                }
              }
              else
              {
                ++all->ship.techQ;
                --all->currInterface->techQ;
                ++all->ship.bodyProperties.mass;
              }
        }
        if ((all->mousePos.x < SCREEN_WIDTH - 30) && (all->mousePos.x > SCREEN_WIDTH - 60))
        {
          if ((all->mousePos.y < 60) && (all->mousePos.y > 40))
            if (all->ship.foodQ)
              if (all->flags.shiftModifier)
              {
                tradeTemp = all->ship.foodQ;
                if (tradeTemp >= 5)
                {
                  all->ship.foodQ -= 5;
                  all->currInterface->foodQ += 5;
                  all->ship.bodyProperties.mass -= 5;
                }
                else
                {
                  all->ship.foodQ -= tradeTemp;
                  all->currInterface->foodQ += tradeTemp;
                  all->ship.bodyProperties.mass -= tradeTemp;
                }
              }
              else
              {
                --all->ship.foodQ;
                ++all->currInterface->foodQ;
                --all->ship.bodyProperties.mass;
              }
          if ((all->mousePos.y < 100) && (all->mousePos.y > 80))
            if (all->ship.medQ)
              if (all->flags.shiftModifier)
              {
                tradeTemp = all->ship.medQ;
                if (tradeTemp >= 5)
                {
                  all->ship.medQ -= 5;
                  all->currInterface->medQ += 5;
                  all->ship.bodyProperties.mass -= 5;
                }
                else
                {
                  all->ship.medQ -= tradeTemp;
                  all->currInterface->medQ += tradeTemp;
                  all->ship.bodyProperties.mass -= tradeTemp;
                }
              }
              else
              {
                --all->ship.medQ;
                ++all->currInterface->medQ;
                --all->ship.bodyProperties.mass;
              }
          if ((all->mousePos.y < 140) && (all->mousePos.y > 120))
            if (all->ship.minQ)
              if (all->flags.shiftModifier)
              {
                tradeTemp = all->ship.minQ;
                if (tradeTemp >= 5)
                {
                  all->ship.minQ -= 5;
                  all->currInterface->minQ += 5;
                  all->ship.bodyProperties.mass -= 5;
                }
                else
                {
                  all->ship.minQ -= tradeTemp;
                  all->currInterface->minQ += tradeTemp;
                  all->ship.bodyProperties.mass -= tradeTemp;
                }
              }
              else
              {
                --all->ship.minQ;
                ++all->currInterface->minQ;
                --all->ship.bodyProperties.mass;
              }
          if ((all->mousePos.y < 180) && (all->mousePos.y > 160))
            if (all->ship.weapQ)
              if (all->flags.shiftModifier)
              {
                tradeTemp = all->ship.weapQ;
                if (tradeTemp >= 5)
                {
                  all->ship.weapQ -= 5;
                  all->currInterface->weapQ += 5;
                  all->ship.bodyProperties.mass -= 5;
                }
                else
                {
                  all->ship.weapQ -= tradeTemp;
                  all->currInterface->weapQ += tradeTemp;
                  all->ship.bodyProperties.mass -= tradeTemp;
                }
              }
              else
              {
                --all->ship.weapQ;
                ++all->currInterface->weapQ;
                --all->ship.bodyProperties.mass;
              }
          if ((all->mousePos.y < 220) && (all->mousePos.y > 200))
            if (all->ship.techQ)
              if (all->flags.shiftModifier)
              {
                tradeTemp = all->ship.techQ;
                if (tradeTemp >= 5)
                {
                  all->ship.techQ -= 5;
                  all->currInterface->techQ += 5;
                  all->ship.bodyProperties.mass -= 5;
                }
                else
                {
                  all->ship.techQ -= tradeTemp;
                  all->currInterface->techQ += tradeTemp;
                  all->ship.bodyProperties.mass -= tradeTemp;
                }
              }
              else
              {
                --all->ship.techQ;
                ++all->currInterface->techQ;
                --all->ship.bodyProperties.mass;
              }
        }
        if ((all->mousePos.x < 230) && (all->mousePos.x > 30))
        {
          if ((all->mousePos.y < 80) && (all->mousePos.y > 30))
          {
            if (currQuest)
              do
              {
                if ((currQuest->goal == all->currInterface) && (currQuest->stage == 1))
                {
                  if (currQuest->type == 0)
                    currQuest->stage = 2;
                  if (currQuest->type == 1)
                    currQuest->stage = -1;
                  all->flags.questChange = true;
                  _CheckQuests(all);
                }
                currQuest = currQuest->next;
              } while ((currQuest) && (currQuest != all->quests));
          }
          if ((all->mousePos.y < 180) && (all->mousePos.y > 130))
          {
            if ((all->currDate.year * 300 + all->currDate.day > all->currInterface->lastScan.year * 300 + all->currInterface->lastScan.day + 2) && (all->currInterface->scansAvail))
            {
              --all->currInterface->scansAvail;
              all->currInterface->lastScan = all->currDate;
              switch (rand() % 7)
              {
              case 0:
                tradeTemp = 2 + rand() % 5;
                all->currInterface->foodQ = tradeTemp - (int)(all->ship.maxMass - all->ship.bodyProperties.mass);
                if (all->currInterface->foodQ < 0)
                  all->currInterface->foodQ = 0;
                tradeTemp = tradeTemp - all->currInterface->foodQ;
                all->ship.bodyProperties.mass += tradeTemp;
                all->ship.foodQ += tradeTemp;
                break;
              case 1:
                tradeTemp = 2 + rand() % 5;
                all->currInterface->medQ = tradeTemp - (int)(all->ship.maxMass - all->ship.bodyProperties.mass);
                if (all->currInterface->medQ < 0)
                  all->currInterface->medQ = 0;
                tradeTemp = tradeTemp - all->currInterface->medQ;
                all->ship.bodyProperties.mass += tradeTemp;
                all->ship.medQ += tradeTemp;
                break;
              case 2:
                tradeTemp = 2 + rand() % 5;
                all->currInterface->minQ = tradeTemp - (int)(all->ship.maxMass - all->ship.bodyProperties.mass);
                if (all->currInterface->minQ < 0)
                  all->currInterface->minQ = 0;
                tradeTemp = tradeTemp - all->currInterface->minQ;
                all->ship.bodyProperties.mass += tradeTemp;
                all->ship.minQ += tradeTemp;
                break;
              case 3:
                tradeTemp = 2 + rand() % 5;
                all->currInterface->weapQ = tradeTemp - (int)(all->ship.maxMass - all->ship.bodyProperties.mass);
                if (all->currInterface->weapQ < 0)
                  all->currInterface->weapQ = 0;
                tradeTemp = tradeTemp - all->currInterface->weapQ;
                all->ship.bodyProperties.mass += tradeTemp;
                all->ship.weapQ += tradeTemp;
                break;
              case 4:
                tradeTemp = 2 + rand() % 5;
                all->currInterface->techQ = tradeTemp - (int)(all->ship.maxMass - all->ship.bodyProperties.mass);
                if (all->currInterface->techQ < 0)
                  all->currInterface->techQ = 0;
                tradeTemp = tradeTemp - all->currInterface->techQ;
                all->ship.bodyProperties.mass += tradeTemp;
                all->ship.techQ += tradeTemp;
                break;
              case 5:
                all->ship.fuel += 0.1 * (1 + 0.1 * (rand() % 20 - 10));
                all->flags.fuelDeplete = false;
                break;
              case 6:
                all->ship.credits += 100 * (1 + 0.2 * (rand() % 80 - 30));
                break;
              }
            }
          }
        }
      }
      break;
    }
    case 6:
    {
      if ((all->mousePos.x > SCREEN_WIDTH / 2 - 100) && (all->mousePos.x < SCREEN_WIDTH / 2 + 100))
        if ((all->mousePos.y < SCREEN_HEIGHT - 50) && (all->mousePos.y > SCREEN_HEIGHT - 100))
          _ApplyDestroyEvent(all);
      if ((all->mousePos.x > SCREEN_WIDTH - 100) && (all->mousePos.x < SCREEN_WIDTH - 50))
      {
        if ((all->mousePos.y < SCREEN_HEIGHT - 150) && (all->mousePos.y > SCREEN_HEIGHT - 200))
        {
          all->flags.scroll = true;
          if (all->currEvent.scrollLine < all->currEvent.textLines)
            ++all->currEvent.scrollLine;
        }
        if ((all->mousePos.y < 100) && (all->mousePos.y > 50))
        {
          all->flags.scroll = true;
          if (all->currEvent.scrollLine)
            --all->currEvent.scrollLine;
        }
      }
      break;
    }
    case 7:
    {
      if (all->currInterface)
      {
        if ((all->mousePos.x < SCREEN_WIDTH) && (all->mousePos.x > SCREEN_WIDTH - 30) && (all->mousePos.y < 30) && (all->mousePos.x > 0))
          _GameStart(all, 2, 0);
        if ((all->mousePos.x < SCREEN_WIDTH / 2 + 250) && (all->mousePos.x > SCREEN_WIDTH / 2 - 250) && (all->mousePos.y < SCREEN_HEIGHT / 2 + 100) && (all->mousePos.x > SCREEN_HEIGHT / 2 - 100))
          if (all->ship.credits >= all->currInterface->fuelPrice)
          {
            all->ship.credits -= all->currInterface->fuelPrice;
            _GameStart(all, 3, 0);
          }
      }
      break;
    }
    case 8:
    {
      if ((all->mousePos.x < SCREEN_WIDTH) && (all->mousePos.x > SCREEN_WIDTH - 30) && (all->mousePos.y < 30) && (all->mousePos.y > 0))
      {
        _GameStart(all, 2, 0);
        break;
      }
      if ((all->mousePos.x < SCREEN_WIDTH - 300) && (all->mousePos.x > SCREEN_WIDTH - 330))
      {
        if ((all->mousePos.y < 30) && (all->mousePos.y > 0))
        {
          all->flags.scroll = 1;
          break;
        }
        if ((all->mousePos.y < SCREEN_HEIGHT) && (all->mousePos.y > SCREEN_HEIGHT - 30))
        {
          all->flags.scroll = 2;
          break;
        }
      }
      break;
    }
    case 9:
    {
      all->flags.menuMode = 0;
      break;
    }
    }
  }
  else
  {
    double dtemp = 0.0;
    planet_t *loc = NULL, *target = NULL;
    switch (all->flags.mapMode)
    {
    case 1:
    {
      loc = _CheckTarget(all, all->ship.bodyProperties.coord);
      target = _CheckTarget(all);
      if ((loc) && (loc == target) && (loc->type != STAR))
      {
        all->ship.well = loc;
        _ShipSet(&all->ship, 0);
        all->timeMods.scale = 10 / sqrt(all->ship.well->bodyProperties.rad);
        if (all->timeMods.scale < 0.5)
        {
          dtemp = log(all->timeMods.scale);
          all->x = (int)(dtemp * 10) / 10.0;
        }
        else
          all->x = 1.0;
        all->timeMods.timeControl = exp(all->x);
        all->flags.mapMode = 0;
        all->flags.modsChange = true;
      }
      all->flags.globalMove = false;
      all->flags.destSet = false;
      all->flags.pause = true;
      break;
    }
    case 0:
    {
      target = _CheckTarget(all);
      if ((target == all->ship.well) && (all->ship.orbit == 1) && ((target->type == HABITABLE) || (target->type == UNINHABITABLE) || \
        (target->type == MOON_HABITABLE) || (target->type == MOON) || (target->type == STAR_GATE)))
      {
        all->currInterface = target;
        all->flags.menu = true;
        all->flags.pause = true;
        switch (target->type)
        {
        case HABITABLE:
        case MOON_HABITABLE:
          all->flags.menuMode = 4;
          break;
        case UNINHABITABLE:
        case MOON:
          all->flags.menuMode = 5;
          break;
        case STAR_GATE:
          all->flags.menuMode = 7;
          break;
        }
      }
      else
        all->trackPlanet = target;
      break;
    }
    case 2:
    {
      all->trackPlanet = _CheckTarget(all);
      break;
    }
    }
  }
  return;
}

void _RightMouseClick(globals *all)
{
  coord_t mouse;
  if ((!(all->flags.fuelDeplete)) && (all->mousePos.x > (SCREEN_WIDTH - SCREEN_HEIGHT) / 2) && (all->mousePos.x < (SCREEN_WIDTH + SCREEN_HEIGHT) / 2))
  {
    double distScale = SCREEN_HEIGHT / (2 * all->ship.well->gravRadius);
    mouse.x = (all->mousePos.x - SCREEN_WIDTH / 2) / distScale;
    mouse.y = (all->mousePos.y - SCREEN_HEIGHT / 2) / distScale;
    all->ship.angle = _GetAngle(all->ship.bodyProperties.coord, mouse);
    all->flags.destSet = true;
    all->ship.moveStart = all->ship.bodyProperties.coord;
    all->ship.moveDest = mouse;
  }
}

/* Events by function
 0 - change wells
 1 - ship update
 2 - global move
 */
void _ShipEvents(globals *all, SHIP_EVENT events[3])
{
  double dtemp = 0.0;
  switch (events[0])
  {
  case GRAV_WELL_CHANGED:
    int temp = all->flags.mapMode;
    if (all->ship.well->parent == NULL)
    {
      all->flags.mapMode = 1;
      all->timeMods.scale = 1.0;
      all->timeMods.timeControl = 3.0;
      all->flags.pause = true;
    }
    if (all->flags.mapMode != temp)
      _ShipSet(&all->ship, all->flags.mapMode);
    if (all->flags.mapMode != 1)
    {
      all->timeMods.scale = 10 / sqrt(all->ship.well->bodyProperties.rad);
      if (all->timeMods.scale < 1.0)
      {
        dtemp = log(all->timeMods.scale);
        all->x = (int)(dtemp * 10) / 10.0;
      }
      else
        all->x = 1.0;
      all->timeMods.timeControl = exp(all->x);
      all->flags.modsChange = true;
    }
    break;
  }
  switch (events[1])
  {
  case SHIP_CRASH:
    all->flags.shipCrashed = true;
    all->flags.menu = true;
    all->flags.menuMode = 0;
    _SystemDestroy(all->system);
    all->system = NULL;
  }
  switch (events[2])
  {
  case DEST_ARRIVED:
    all->flags.destSet = false;
    all->flags.globalMove = false;
    all->flags.pause = true;
    break;
  case FUEL_DEPLETE:
    all->flags.fuelDeplete = true;
    all->flags.destSet = false;
    all->flags.globalMove = false;
    all->flags.pause = true;
    break;
  }
}

void _TimedUpdate(globals *all)
{
  planet_t *plaPtr = NULL, *satPtr = NULL;
  if (all->flags.mapMode == 1)
  {
    if ((all->flags.eventTrigger == 5) || !(rand() % (5 - all->flags.eventTrigger)))
    {
      all->flags.eventTrigger = 0;
      _GenerateWorldEvent(all);
    }
    else
      ++all->flags.eventTrigger;
  }
  else
  {
    if ((all->flags.eventTrigger == 8) || !(rand() % (8 - all->flags.eventTrigger)))
    {
      all->flags.eventTrigger = 0;
      _GenerateWorldEvent(all);
    }
    else
      ++all->flags.eventTrigger;
  }
  if (!all->flags.disableQuests)
  {
    plaPtr = all->system->satellites;
    for (int i = 0; i < all->system->satelliteCount - 1; ++i)
    {
      if ((plaPtr->currQuest == NULL) && ((plaPtr->type == HABITABLE) || (plaPtr->type == MOON_HABITABLE)))
      {
        _GenerateQuest(all, plaPtr, 0, false);
        all->flags.questChange = true;
      }
      satPtr = plaPtr->satellites;
      for (int j = 0; j < plaPtr->satelliteCount; ++j)
      {
        if ((satPtr->currQuest == NULL) && ((satPtr->type == HABITABLE) || (satPtr->type == MOON_HABITABLE)))
        {
          _GenerateQuest(all, satPtr, 0, false);
          all->flags.questChange = true;
        }
        satPtr = satPtr->next;
      }
      plaPtr = plaPtr->next;
    }
  }
  if (all->quests)
    _CheckQuests(all);
  return;
}

void GameUpdate(globals *all)
{
  int temp = 0;
  SHIP_EVENT events[3] = {};
  planet_t *tempInf = NULL, *tempPassInf = NULL;
  quest_t *currQuest = NULL;
  if (!all->flags.menu)
  {
    if ((all->flags.systemCount == 5) && (all->ship.orbit == 1) && (all->quests))
    {
      currQuest = all->quests;
      do
      {
        if (currQuest->type == 4)
          if (all->ship.well == currQuest->goal)
          {
            all->flags.menu = true;
            all->flags.menuMode = 9;
            all->flags.pause = true;
            return;
          }
        currQuest = currQuest->next;
      } while ((currQuest) && (currQuest != all->quests));
    }
    tempInf = all->infoTarget;
    all->infoTarget = _CheckTarget(all);
    if (tempInf == all->infoTarget)
      all->flags.infoChange = false;
    else
      all->flags.infoChange = true;
    if (all->flags.mapMode == 1)
    {
      tempPassInf = all->passiveInfoTarg;
      all->passiveInfoTarg = _CheckTarget(all, all->ship.bodyProperties.coord);
      if (tempPassInf == all->passiveInfoTarg)
        all->flags.passInfChange = false;
      else
        all->flags.passInfChange = true;
    }
    all->timers.newTime = _GetCounter(all->timers.starter, all->timers.PCFreq);
    all->timeMods.speed = all->timers.newTime - all->timers.oldTime;
    all->timers.oldTime = all->timers.newTime;
    all->timeMods.universal = all->timeMods.speed / all->timeMods.scale * all->timeMods.timeControl;
    if (!all->flags.pause)
    {
      all->currDate.time += all->timeMods.universal;
      temp = all->currDate.day;
      all->currDate.day = 1 + (int)(all->currDate.time / 10000000.0);
      if (temp == all->currDate.day)
        all->flags.dateChange = false;
      else
      {
        all->flags.dateChange = true;
        _TimedUpdate(all);
      }
      if (all->currDate.day == 301)
      {
        all->currDate.day = 0;
        ++all->currDate.year;
      }
      _PhysixUpdate(all, events);
      _ShipEvents(all, events);
    }
  }
  return;
}