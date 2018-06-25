#include "systems.h"
#include "physix.h"
#include "graphix.h"
#include "globals.h"

int main(void)
{
  globals all;
  InitAll(&all);
  while (!all.flags.quit)
  {
    EventsGlobal(&all);
    GameUpdate(&all);
    DrawScene(&all);
  }
  DeInitAll(&all);
  return 0;
}