
#include "stm32f4xx_hal.h"

// Fake implementations — compiled as C to prevent multiple definition errors.

void __NOP(void) { ; }

HAL_StatusTypeDef HAL_Init(void) { return HAL_OK; }

void HAL_Delay(uint32_t Delay) { (void)Delay; }
