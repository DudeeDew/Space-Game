#pragma once
#include "SDL.h"
#include "physix.h"
#include "globals.h"
#include <Windows.h>

void EventsGlobal(globals *all);
void GameUpdate(globals *all);
void InitAll(globals *all);
void DeInitAll(globals *all);
double _GetCounter(ULONGLONG CounterStart, double PCFreq);