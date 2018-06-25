#pragma once
#include "systems.h"
#include "globals.h"
#include <Windows.h>

int _CheckOrbit(playerShip_t ship);
double _GetAngle(coord_t point1, coord_t point2);
void _PhysixUpdate(globals *all, SHIP_EVENT events[3]);
void _ShipSet(playerShip_t *ship, int mode);
double _GetSquareDist(coord_t, coord_t);