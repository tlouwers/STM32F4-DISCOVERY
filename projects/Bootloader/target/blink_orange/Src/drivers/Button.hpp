#ifndef BL_BUTTON_HPP_
#define BL_BUTTON_HPP_

#include "stm32f4xx_hal.h"

class Button
{
public:
    static void Init();
    static bool IsPressed();
};

#endif // BL_BUTTON_HPP_
