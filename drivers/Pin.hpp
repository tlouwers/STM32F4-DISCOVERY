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
 * \details Intended use is to have a method at board startup which sets each
 *          pin to a defined state. This is done by constructing a Pin object,
 *          and let it go out of scope.
 *          Later in the application, for the few pins where needed, pass along
 *          the PinIdPort struct to the class where a pin object is needed.
 *          Then during the Initialisation of that class (not construction)
 *          create and fill the Pin object with desired values. At this point
 *          the interrupts can be configured as well.
 *
 *          // Examples:
 *          Pin a1(id, Level::LOW);                     // Construction - output
 *          Pin a2(id, PullUpDown::DOWN);               // Construction - input, pull-down
 *
 *          // As output
 *          a1.Set(Level::HIGH);                        // Set output high
 *          a1.Toggle();                                // High to low, low to high
 *          Level lvl1 = a1.Get();                      // Get the actual pin level
 *          a1.Configure(PullUpDown::HIGHZ);            // Make pin input, floating
 *
 *          // As Input:
 *          Level lvl2 = a2.Get();                      // Get the actual pin level
 *          a2.Interrupt(Trigger::RISING, callback);    // Configure interrupt, attach callback, default active
 *          a2.InterruptDisable();                      // Disable the callback (ignores the interrupt)
 *          a2.InterruptEnable();                       // Enables the callback
 *          a2.InterruptRemove();                       // Removed the interrupt, detaches the callback
 *
 *          // Allowed (moves):
 *          Pin a4 = std::move(a1);                     // Move assignment
 *          Pin a5(std::move(a2));                      // Move constructor
 *
 *          // Not allowed (copies):
 *          Pin a7 = a1;                                // Copy assignment
 *          Pin a8(a2);                                 // Copy constructor
 *          Pin();                                      // No empty constructor
 *
 * \author      T. Louwers <t.louwers@gmail.com>
 * \version     1.0
 * \date        03-2019
 */

#ifndef PIN_HPP_
#define PIN_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include <functional>
#include "stm32f4xx_hal.h"


/************************************************************************/
/* Structures                                                           */
/************************************************************************/
/**
 * \brief   Structure to group the pin id and port.
 */
struct PinIdPort
{
    uint16_t      id;       ///< The pin id bitmask from 'GPIO_pins_define'.
    GPIO_TypeDef* port;     ///< Pointer to base 'GPIO_TypeDef' struct, like: 'GPIOA'.
};

/**
 * \brief   Structure to group the interrupt callback (if any) and flag if the
 *          callback should be called when the interrupt is triggered.
 */
struct PinInterrupt
{
    std::function<void()> callback = nullptr;   ///< Callback to call when interrupt for pin triggers.
    bool                  enabled  = false;     ///< Flag, indicating interrupt for pin is enabled or not.
};


/************************************************************************/
/* Enums                                                                */
/************************************************************************/
/**
 * \enum    Level
 * \brief   Pin level.
 */
enum class Level : bool
{
	LOW,
	HIGH
};

/**
 * \enum    Direction
 * \brief   Pin direction.
 */
enum class Direction : uint8_t
{
	UNDEFINED,
	INPUT,
	OUTPUT
};

/**
 * \enum    Drive
 * \brief   Output drive mode of a pin.
 */
enum class Drive : uint8_t
{
    PUSH_PULL,
    OPEN_DRAIN,
    OPEN_DRAIN_PULL_UP,
    OPEN_DRAIN_PULL_DOWN,
    OPEN_DRAIN_PULL_UP_DOWN
};

/**
 * \enum    PullUpDown
 * \brief   Input pull up or pull down mode of a pin.
 */
enum class PullUpDown : uint8_t
{
	HIGHZ,
	UP,
	DOWN,
	UP_DOWN
};

/**
 * \enum    Trigger
 * \brief   Interrupt trigger condition for a pin.
 */
enum class Trigger : uint8_t
{
	RISING,
	FALLING,
	BOTH
};


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
/**
 * \brief   GPIO pin convenience class.
 */
class Pin
{
public:
    Pin(Pin&& other);

	explicit Pin(PinIdPort idAndPort);
	Pin(PinIdPort idAndPort, Level level, Drive drive = Drive::PUSH_PULL);
	Pin(PinIdPort idAndPort, PullUpDown pullUpDown);

	void Configure(Level level, Drive drive = Drive::PUSH_PULL);
	void Configure(PullUpDown pullUpDown);

	bool Interrupt(Trigger trigger, const std::function<void()>& callback, bool enabledAfterConfigure = true);
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

    // Explicit disabled constructors/operators
    Pin() = delete;
    Pin(const Pin&) = delete;
    Pin& operator= (const Pin& other) = delete;
};


extern "C"
{
    void EXTI0_IRQHandler(void);
    void EXTI1_IRQHandler(void);
    void EXTI2_IRQHandler(void);
    void EXTI3_IRQHandler(void);
    void EXTI4_IRQHandler(void);
    void EXTI9_5_IRQHandler(void);
    void EXTI15_10_IRQHandler(void);
}


#endif	// PIN_HPP_
