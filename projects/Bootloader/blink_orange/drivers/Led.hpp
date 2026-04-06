#ifndef BL_LED_HPP_
#define BL_LED_HPP_

#include "stm32f4xx_hal.h"

class Led
{
public:
    static void Init();
    static void Toggle();
    static void On();
    static void Off();
};

#endif // BL_LED_HPP_
