#pragma once

#ifdef GTASA
#define BY_GAME(sa, vc, iii) sa
#elif GTAVC
#define BY_GAME(sa, vc, iii) vc
#elif GTA3
#define BY_GAME(sa, vc, iii) iii
#endif
