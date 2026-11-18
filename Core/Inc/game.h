#ifndef GAME_H
#define GAME_H

#include "stm32h5xx_hal_rng.h"

bool game_init(RNG_HandleTypeDef* hrng);
void game_execute(void);

#endif