#include "SDL.h"
#include "physix.h"
#include <Windows.h>
#include <math.h>
#include <stdio.h>

double _GetSquareDist(coord_t point1, coord_t point2)
{
  return (point1.x - point2.x) * (point1.x - point2.x) + (point1.y - point2.y) * (point1.y - point2.y);
}

double _GetAngle(coord_t point1, coord_t point2)
{
  double angle = 0.0;
  if (point2.x - point1.x)
  {
    angle = atan((point2.y - point1.y) / (point2.x - point1.x));
    if ((point2.x - point1.x) < 0)
      angle += PI;
  }
  else
    angle = PI / 2 + PI * ((point2.y - point1.y) < 0);
  return angle;
}

SHIP_EVENT _ChangeWells(playerShip_t *ship)
{
  planet_t *plaPtr = NULL;
  if (ship->well->parent != NULL)
  {
    if (sqrt(_GetSquareDist(ship->bodyProperties.coord, ship->well->bodyProperties.coord)) > ship->well->gravRadius)
    {
      ship->well = ship->well->parent;
      return GRAV_WELL_CHANGED;
    }
  }
  else
    return NO_EVENT;
  plaPtr = ship->well->satellites;
  for (int i = 0; i < ship->well->satelliteCount; ++i)
  {
    if (sqrt(_GetSquareDist(ship->bodyProperties.coord, plaPtr->bodyProperties.coord)) <= plaPtr->gravRadius)
    {
      ship->well = plaPtr;
      return GRAV_WELL_CHANGED;
    }
    plaPtr = plaPtr->next;
  }
  return NO_EVENT;
}

void _ReformSatList(planet_t *planet, planet_t *newParent)
{
  planet_t *plaPtr = newParent->satellites, *tmpPtr = planet->parent, *tmpNext = planet->next, *tmpPrev = planet->prev;
  double dist = _GetSquareDist(planet->bodyProperties.coord, newParent->bodyProperties.coord);
  planet->next = NULL;
  planet->prev = NULL;
  planet->parent = newParent;
  if (!newParent->satellites)
    newParent->satellites = planet;
  else
  {
    if (dist <= _GetSquareDist(plaPtr->bodyProperties.coord, newParent->bodyProperties.coord))
    {
      plaPtr->prev = planet;
      newParent->satellites = planet;
    }
    else
    {
      for (int i = 0; i < newParent->satelliteCount; ++i)
      {
        if (dist > _GetSquareDist(plaPtr->bodyProperties.coord, newParent->bodyProperties.coord))
          if (!plaPtr->next)
          {
            plaPtr->next = planet;
            planet->prev = plaPtr;
            break;
          }
          else
            if (dist <= _GetSquareDist(plaPtr->next->bodyProperties.coord, newParent->bodyProperties.coord))
            {
              planet->next = plaPtr->next;
              plaPtr->next->prev = planet;
              plaPtr->next = planet;
              planet->prev = plaPtr;
            }
            else
              plaPtr = plaPtr->next;
      }
    }
  }
  if (tmpNext)
  {
    tmpNext->prev = tmpPrev;
    if (tmpPtr->satellites == planet)
      tmpPtr->satellites = tmpNext;
  }
  if (tmpPrev)
    tmpPrev->next = tmpNext;
  ++newParent->satelliteCount;
  --tmpPtr->satelliteCount;
  return;
}

int _ChangeWells(planet_t *planet)
{
  planet_t *plaPtr = NULL, *satPtr = NULL;
  int satCount = 0;
  if (planet->parent->parent != NULL)
    if (sqrt(_GetSquareDist(planet->bodyProperties.coord, planet->parent->bodyProperties.coord)) > planet->parent->gravRadius)
    {
      _ReformSatList(planet, planet->parent->parent);
      return 1;
    }
  plaPtr = planet->parent->satellites;
  for (int i = 0; i < planet->parent->satelliteCount; ++i)
  {
    if ((plaPtr) && (plaPtr->parent) && (plaPtr->parent->parent == NULL) && (sqrt(_GetSquareDist(planet->bodyProperties.coord, plaPtr->bodyProperties.coord)) <= plaPtr->gravRadius) && (planet->bodyProperties.mass < plaPtr->bodyProperties.mass))
    {
      _ReformSatList(planet, plaPtr);
      satPtr = planet->satellites;
      satCount = planet->satelliteCount;
      for (int j = 0; j < satCount; ++j)
      {
        _ReformSatList(satPtr, plaPtr);
        satPtr = satPtr->next;
      }
      return 1;
    }
    plaPtr = plaPtr->next;
  }
  return 0;
}

void _SatUpdate(planet_t *planet, int pos, coord_t savedAccel, double timeMod)
{
  planet_t *satPtr = NULL;
  planet->bodyProperties.accel.x = savedAccel.x;
  planet->bodyProperties.accel.y = savedAccel.y;
  planet->bodyProperties.accel.x += planet->parent->bodyProperties.mass * GRAV / _GetSquareDist(planet->bodyProperties.coord, planet->parent->bodyProperties.coord) * cos(_GetAngle(planet->bodyProperties.coord, planet->parent->bodyProperties.coord));
  planet->bodyProperties.accel.y += planet->parent->bodyProperties.mass * GRAV / _GetSquareDist(planet->bodyProperties.coord, planet->parent->bodyProperties.coord) * sin(_GetAngle(planet->bodyProperties.coord, planet->parent->bodyProperties.coord));
  satPtr = planet->parent->satellites;
  for (int i = 0; i < pos; ++i)
  {
    planet->bodyProperties.accel.x += satPtr->bodyProperties.mass * GRAV / _GetSquareDist(planet->bodyProperties.coord, satPtr->bodyProperties.coord) * cos(_GetAngle(planet->bodyProperties.coord, satPtr->bodyProperties.coord));
    planet->bodyProperties.accel.y += satPtr->bodyProperties.mass * GRAV / _GetSquareDist(planet->bodyProperties.coord, satPtr->bodyProperties.coord) * sin(_GetAngle(planet->bodyProperties.coord, satPtr->bodyProperties.coord));
    satPtr = satPtr->next;
  }
  satPtr = planet->next;
  for (int i = pos + 1; i < planet->parent->satelliteCount; ++i)
  {
    planet->bodyProperties.accel.x += satPtr->bodyProperties.mass * GRAV / _GetSquareDist(planet->bodyProperties.coord, satPtr->bodyProperties.coord) * cos(_GetAngle(planet->bodyProperties.coord, satPtr->bodyProperties.coord));
    planet->bodyProperties.accel.y += satPtr->bodyProperties.mass * GRAV / _GetSquareDist(planet->bodyProperties.coord, satPtr->bodyProperties.coord) * sin(_GetAngle(planet->bodyProperties.coord, satPtr->bodyProperties.coord));
    satPtr = satPtr->next;
  }
  planet->bodyProperties.vel.x += planet->bodyProperties.accel.x * timeMod;
  planet->bodyProperties.vel.y += planet->bodyProperties.accel.y * timeMod;
}

void _SystemUpdate(planet_t *starPtr, double timeMod)
{
  coord_t starAccel = { starPtr->bodyProperties.accel.x, starPtr->bodyProperties.accel.y }, plaAccel = { 0, 0 };
  planet_t *plaPtr = NULL, *satPtr = NULL, *tmpPtr = NULL;
  double dist = 0.0;
  starPtr->bodyProperties.accel.x = 0;
  starPtr->bodyProperties.accel.y = 0;
  plaPtr = starPtr->satellites;
  for (int i = 0; i < starPtr->satelliteCount; ++i)
  {
    dist = _GetSquareDist(starPtr->bodyProperties.coord, plaPtr->bodyProperties.coord);
    starPtr->bodyProperties.accel.x += plaPtr->bodyProperties.mass * GRAV / dist * cos(_GetAngle(starPtr->bodyProperties.coord, plaPtr->bodyProperties.coord));
    starPtr->bodyProperties.accel.y += plaPtr->bodyProperties.mass * GRAV / dist * sin(_GetAngle(starPtr->bodyProperties.coord, plaPtr->bodyProperties.coord));
    _SatUpdate(plaPtr, i, starAccel, timeMod);
    satPtr = plaPtr->satellites;
    for (int j = 0; j < plaPtr->satelliteCount; ++j)
    {
      dist = _GetSquareDist(starPtr->bodyProperties.coord, satPtr->bodyProperties.coord);
      plaAccel = { plaPtr->bodyProperties.accel.x, plaPtr->bodyProperties.accel.y };
      plaPtr->bodyProperties.accel.x += satPtr->bodyProperties.mass * GRAV / dist * cos(_GetAngle(starPtr->bodyProperties.coord, satPtr->bodyProperties.coord));
      plaPtr->bodyProperties.accel.y += satPtr->bodyProperties.mass * GRAV / dist * sin(_GetAngle(starPtr->bodyProperties.coord, satPtr->bodyProperties.coord));
      _SatUpdate(satPtr, j, plaAccel, timeMod);
      tmpPtr = satPtr->prev;
      /*if (_ChangeWells(satPtr))
        satPtr = tmpPtr->next;
      else
        satPtr = satPtr->next;*/
      satPtr = satPtr->next;
    }
    tmpPtr = plaPtr->prev;
    /*if (_ChangeWells(plaPtr))
      plaPtr = tmpPtr->next;
    else
      plaPtr = plaPtr->next;*/
    plaPtr = plaPtr->next;
  }
  starPtr->bodyProperties.vel.x += starPtr->bodyProperties.accel.x * timeMod;
  starPtr->bodyProperties.vel.x += starPtr->bodyProperties.accel.x * timeMod;

  return;
}

SHIP_EVENT _ShipUpdate(playerShip_t *ship, double timeMod)
{
  double dist = 0.0, angle = 0.0;
  planet_t *satPtr = ship->well->satellites;
  ship->bodyProperties.accel.x = sqrt((double)ship->enginePowerLevel) / 20000000000.0 / ship->bodyProperties.mass * cos(ship->angle) * timeMod * (ship->engineOn == true);
  ship->bodyProperties.accel.y = sqrt((double)ship->enginePowerLevel) / 20000000000.0 / ship->bodyProperties.mass * sin(ship->angle) * timeMod * (ship->engineOn == true);
  dist = _GetSquareDist(ship->bodyProperties.coord, ship->well->bodyProperties.coord);
  angle = _GetAngle(ship->bodyProperties.coord, ship->well->bodyProperties.coord);
  if (sqrt(dist) <= ship->well->bodyProperties.rad)
    return SHIP_CRASH;
  ship->bodyProperties.accel.x += ship->well->bodyProperties.mass * GRAV / dist * cos(angle);
  ship->bodyProperties.accel.y += ship->well->bodyProperties.mass * GRAV / dist * sin(angle);
  for (int i = 0; i < ship->well->satelliteCount; ++i)
  {
    dist = _GetSquareDist(ship->bodyProperties.coord, satPtr->bodyProperties.coord);
    angle = _GetAngle(ship->bodyProperties.coord, satPtr->bodyProperties.coord);
    if (sqrt(dist) <= satPtr->bodyProperties.rad)
      return SHIP_CRASH;
    ship->bodyProperties.accel.x += satPtr->bodyProperties.mass * GRAV / dist * cos(angle);
    ship->bodyProperties.accel.y += satPtr->bodyProperties.mass * GRAV / dist * sin(angle);
    satPtr = satPtr->next;
  }
  ship->bodyProperties.vel.x += ship->bodyProperties.accel.x * timeMod;
  ship->bodyProperties.vel.y += ship->bodyProperties.accel.y * timeMod;
  ship->bodyProperties.coord.x += ship->bodyProperties.vel.x * timeMod;
  ship->bodyProperties.coord.y += ship->bodyProperties.vel.y * timeMod;
  return NO_EVENT;
}

void _SystemMove(planet_t *starPtr, double timeMod)
{
  planet_t *plaPtr = starPtr->satellites, *satPtr = NULL;
  starPtr->bodyProperties.coord.x += starPtr->bodyProperties.vel.x * timeMod;
  starPtr->bodyProperties.coord.y += starPtr->bodyProperties.vel.y * timeMod;
  for (int i = 0; i < starPtr->satelliteCount; ++i)
  {
    plaPtr->bodyProperties.coord.x += plaPtr->bodyProperties.vel.x * timeMod;
    plaPtr->bodyProperties.coord.y += plaPtr->bodyProperties.vel.y * timeMod;
    satPtr = plaPtr->satellites;
    for (int j = 0; j < plaPtr->satelliteCount; ++j)
    {
      satPtr->bodyProperties.coord.x += satPtr->bodyProperties.vel.x * timeMod;
      satPtr->bodyProperties.coord.y += satPtr->bodyProperties.vel.y * timeMod;
      satPtr = satPtr->next;
    }
    plaPtr = plaPtr->next;
  }
  return;
}

double _GetSemiAxis(body_t body, body_t parentBody)
{
  coord_t velsLocal = { body.vel.x - parentBody.vel.x, body.vel.y - parentBody.vel.y };
  double a = 0.0, vel = sqrt(velsLocal.x * velsLocal.x + velsLocal.y * velsLocal.y),
    d = sqrt(_GetSquareDist(body.coord, parentBody.coord));
  a = (d * GRAV * parentBody.mass / (2 * GRAV * parentBody.mass - d * vel * vel));
  return a;
}

double _GetExcenter(body_t body, body_t parentBody, double msa)
{
  coord_t velsLocal = { body.vel.x - parentBody.vel.x, body.vel.y - parentBody.vel.y };
  double angle = (_GetAngle({ 0.0, 0.0 }, { body.coord.x - parentBody.coord.x, body.coord.y - parentBody.coord.y }) - _GetAngle({ 0.0, 0.0 }, { body.vel.x, body.vel.y })),
    dist = sqrt(_GetSquareDist(body.coord, parentBody.coord)),
    vels = dist * (velsLocal.x * velsLocal.x + velsLocal.y * velsLocal.y) / (GRAV * parentBody.mass);
  if (msa == 0.0)
    return 0.0;
  return sqrt((vels - 1) * (vels - 1) + vels * dist * cos(angle) * cos(angle) / msa);
}

int _CheckOrbit(playerShip_t ship)
{
  double msa = _GetSemiAxis(ship.bodyProperties, ship.well->bodyProperties), excen = 0.0;
  excen = _GetExcenter(ship.bodyProperties, ship.well->bodyProperties, msa);
  if ((fabs(msa * (1 - excen)) <= ship.well->bodyProperties.rad))
    return 2;
  if (fabs(msa * (1 + excen)) <= ship.well->gravRadius / 2)
    if (sqrt(_GetSquareDist(ship.bodyProperties.coord, ship.well->bodyProperties.coord)) <= ship.well->bodyProperties.rad * 20)
      return 1;
    else
      return 3;
  return 0;
}

void _ShipTurn(SDL_Point mousePos, double timeMod, playerShip_t *ship)
{
  coord_t mouse = { (double)mousePos.x, (double)mousePos.y }, shipPos = { SCREEN_WIDTH / 2.0, SCREEN_HEIGHT / 2.0 };
  double dist1 = 0.0, dist2 = 0.0;
  coord_t left = { cos(ship->angle - PI / 2) + shipPos.x, sin(ship->angle - PI / 2) + shipPos.y },
    right = { cos(ship->angle + PI / 2) + shipPos.x, sin(ship->angle + PI / 2) + shipPos.y };
  dist1 = _GetSquareDist(left, mouse);
  dist2 = _GetSquareDist(right, mouse);
  if (fabs(dist1 - dist2) < 0.05)
    return;
  if (timeMod > 20000)
  {
    ship->angle = _GetAngle(shipPos, mouse);
    return;
  }
  if (dist1 < dist2)
    ship->angle -= ship->turnRate * timeMod / 700000.0;
  else
    ship->angle += ship->turnRate * timeMod / 700000.0;
  return;
}

SHIP_EVENT _GlobalMove(playerShip_t *ship, double time)
{
  if (ship->fuel <= 0.0)
  {
    ship->fuel = 0.0;
    return FUEL_DEPLETE;
  }
  ship->bodyProperties.coord.x += cos(ship->angle) * ship->globalSpeed * time;
  ship->bodyProperties.coord.y += sin(ship->angle) * ship->globalSpeed * time;
  ship->fuel -= time * 0.00000001;
  if (sqrt(_GetSquareDist(ship->moveStart, ship->bodyProperties.coord)) >= sqrt(_GetSquareDist(ship->moveStart, ship->moveDest)))
  {
    ship->bodyProperties.coord = ship->moveDest;
    return DEST_ARRIVED;
  }
  return NO_EVENT;
}

/* Setting modes:
 0 - small scale
 1 - big scale
 */
void _ShipSet(playerShip_t *ship, int mode)
{
  if (mode == 1)
  {
    ship->bodyProperties.vel = { 0.0, 0.0 };
    ship->bodyProperties.accel = { 0.0, 0.0 };
    ship->enginePowerLevel = 0;
    ship->engineOn = false;
  }
  else
  {
    double angle = _GetAngle(ship->well->bodyProperties.coord, ship->bodyProperties.coord), dist = ship->well->gravRadius * 0.9, vel = 0.0;
    ship->bodyProperties.coord.x = dist * cos(angle) + ship->well->bodyProperties.coord.x;
    ship->bodyProperties.coord.y = dist * sin(angle) + ship->well->bodyProperties.coord.y;
    vel = sqrt(GRAV * ship->well->bodyProperties.mass / dist * (1 + 0.0001 * (rand() % 50 - 20)));
    angle -= PI / 2;
    ship->bodyProperties.vel.x = vel * cos(angle) + ship->well->bodyProperties.vel.x;
    ship->bodyProperties.vel.y = vel * sin(angle) + ship->well->bodyProperties.vel.y;
  }
}

void _PhysixUpdate(globals *all, SHIP_EVENT events[3])
{
  events[0] = NO_EVENT;
  events[1] = NO_EVENT;
  events[2] = NO_EVENT;
  events[0] = _ChangeWells(&all->ship);
  _SystemUpdate(all->system, all->timeMods.universal);
  if (all->flags.mapMode != 1)
  {
    _ShipTurn(all->mousePos, all->timeMods.universal, &(all->ship));
    events[1] = _ShipUpdate(&(all->ship), all->timeMods.universal);
  }
  else
    if (all->flags.globalMove && all->flags.destSet)
      events[2] = _GlobalMove(&all->ship, all->timeMods.universal);
  _SystemMove(all->system, all->timeMods.universal);
  all->ship.orbit = _CheckOrbit(all->ship);
  return;
}