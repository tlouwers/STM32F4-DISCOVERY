/**
 * \file Pin.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 *
 * \brief   Helper class intended as 'set & forget' for pin  configurations.
 * 			State is preserved (partly) within the hardware.
 *
 * \details <todo>
 *
 * \author      T. Louwers <t.louwers@gmail.com>
 * \version     1.0
 * \date        02-2019
 */

#ifndef PIN_HPP_
#define PIN_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include "stm32f4xx_hal.h"


/************************************************************************/
/* Structures                                                           */
/************************************************************************/
struct PinIdPort
{
    uint16_t      id;       ///< The pin id bitmask from 'GPIO_pins_define'.
    GPIO_TypeDef* port;     ///< Pointer to base 'GPIO_TypeDef' struct, like: 'GPIOA'.
};


/************************************************************************/
/* Enums                                                                */
/************************************************************************/
enum class Level : bool
{
	LOW,
	HIGH
};

enum class Direction : uint8_t
{
	UNDEFINED,
	INPUT,
	OUTPUT,
	ALTERNATE
};

enum class PullUpDown : uint8_t
{
	HIGHZ,
	PULL_UP,
	PULL_DOWN,
	PULL_UP_DOWN
};

enum class Trigger : uint8_t
{
	RISING_EDGE,
	FALLING_EDGE,
	BOTH_EDGES
};

/**
 * \details Check: stm32f4xx_hal_gpio_ex.h for details.
 */
enum class Alternate : uint8_t
{
    AF0  = 0x00,
    AF1  = 0x01,
    AF2  = 0x02,
    AF3  = 0x03,
    AF4  = 0x04,
    AF5  = 0x05,
    AF6  = 0x06,
    AF7  = 0x07,
    AF8  = 0x08,
    AF9  = 0x09,
    AF10 = 0x0A,
    AF11 = 0x0B,
    AF12 = 0x0C,
    AF13 = 0x0D,
    AF14 = 0x0E,
    AF15 = 0x0F
};


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class Pin
{
public:
    Pin(Pin&& other);

	explicit Pin(PinIdPort idAndPort);
	Pin(PinIdPort idAndPort, Level level = Level::LOW);
	Pin(PinIdPort idAndPort, PullUpDown pullUpDown = PullUpDown::HIGHZ);
	Pin(PinIdPort idAndPort, Alternate alternate = Alternate::AF0);

	void Configure(Level level);
	void Configure(PullUpDown pullUpDown);
	void Configure(Alternate alternate);

	bool Interrupt(Trigger trigger, bool enabledAfterConfigure = true);
	bool InterruptEnable();
	bool InterruptDisable();
	bool InterruptRemove();

	void Toggle() const;
	void Set(Level level);
	Level Get() const;

	Pin& operator= (Pin&& other);

private:
	uint16_t      mId        = UINT16_MAX;
	GPIO_TypeDef* mPort      = nullptr;
	Direction     mDirection = Direction::UNDEFINED;

	void CheckAndSetIdAndPort(uint16_t id, GPIO_TypeDef* port);
	void CheckAndEnableAHB1PeripheralClock(GPIO_TypeDef* port);

    // Explicit disabled constructors/operators
    Pin() = delete;
    Pin(const Pin&) = delete;
    Pin& operator= (const Pin& other) = delete;
};


#endif	// PIN_HPP_
