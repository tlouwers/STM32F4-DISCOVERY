/**
 * \file Pin.cpp
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

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "Pin.hpp"
#include <cassert>


/************************************************************************/
/* Constants                                                            */
/************************************************************************/
/**
 * \brief   Number indicating an invalid entry.
 */
static constexpr uint16_t INVALID_ENTRY = UINT16_MAX;

static constexpr uint8_t INTERRUPT_PRIORITY = 5;


static PinInterrupt pinInterruptList[16] = {};


/************************************************************************/
/* Static functions                                                     */
/************************************************************************/
static bool IsOnlyASingleBitSetInIdMask(uint16_t id)
{
    // Check if only a single bit is set in the id GPIO bitmask
    return ((id > 0) && ((id & (id - 1)) == 0));
}

static int GetIndexById(uint16_t id)
{
    // https://www.go4expert.com/articles/builtin-gcc-functions-builtinclz-t29238/

    // Counting trailing zeroes results in our index to use here.
    // Works as id is represented as uint16_t bitmask: 'GPIO_pins_define'.
    return __builtin_ctz(id);
}

static bool IsIRQSharedWithOtherPin(uint16_t id)
{
    if (id < GPIO_PIN_5 )
    {
        return false;
    }
    else if (id < GPIO_PIN_10)
    {
        bool result = true;
        if (id != 5) { result &= (pinInterruptList[5].callback == nullptr); }
        if (id != 6) { result &= (pinInterruptList[6].callback == nullptr); }
        if (id != 7) { result &= (pinInterruptList[7].callback == nullptr); }
        if (id != 8) { result &= (pinInterruptList[8].callback == nullptr); }
        if (id != 9) { result &= (pinInterruptList[9].callback == nullptr); }
        return !result;
    }
    else
    {
        bool result = true;
        if (id != 10) { result &= (pinInterruptList[10].callback == nullptr); }
        if (id != 11) { result &= (pinInterruptList[11].callback == nullptr); }
        if (id != 12) { result &= (pinInterruptList[12].callback == nullptr); }
        if (id != 13) { result &= (pinInterruptList[13].callback == nullptr); }
        if (id != 14) { result &= (pinInterruptList[14].callback == nullptr); }
        if (id != 15) { result &= (pinInterruptList[15].callback == nullptr); }
        return !result;
    }
}

/**
 * \brief   Check if the appropriate AHB1 peripheral clock for the port on
 *          which the pin resides is enabled, if not enable it.
 * \param   port    The port on which the pin resides.
 * \note    Might not be supported by the selected microcontroller, check the
 *          documentation.
 */
static void CheckAndEnableAHB1PeripheralClock(GPIO_TypeDef* port)
{
    assert((port != nullptr) && "Invalid variable for port passed, cannot be nullptr");

         if (port == GPIOA) { if (!__HAL_RCC_GPIOA_IS_CLK_ENABLED()) { __HAL_RCC_GPIOA_CLK_ENABLE(); } }
    else if (port == GPIOB) { if (!__HAL_RCC_GPIOB_IS_CLK_ENABLED()) { __HAL_RCC_GPIOB_CLK_ENABLE(); } }
    else if (port == GPIOC) { if (!__HAL_RCC_GPIOC_IS_CLK_ENABLED()) { __HAL_RCC_GPIOC_CLK_ENABLE(); } }
    else if (port == GPIOD) { if (!__HAL_RCC_GPIOD_IS_CLK_ENABLED()) { __HAL_RCC_GPIOD_CLK_ENABLE(); } }
    else if (port == GPIOE) { if (!__HAL_RCC_GPIOE_IS_CLK_ENABLED()) { __HAL_RCC_GPIOE_CLK_ENABLE(); } }
    else if (port == GPIOF) { if (!__HAL_RCC_GPIOF_IS_CLK_ENABLED()) { __HAL_RCC_GPIOF_CLK_ENABLE(); } }
    else if (port == GPIOG) { if (!__HAL_RCC_GPIOG_IS_CLK_ENABLED()) { __HAL_RCC_GPIOG_CLK_ENABLE(); } }
    else if (port == GPIOH) { if (!__HAL_RCC_GPIOH_IS_CLK_ENABLED()) { __HAL_RCC_GPIOH_CLK_ENABLE(); } }
    else if (port == GPIOI) { if (!__HAL_RCC_GPIOI_IS_CLK_ENABLED()) { __HAL_RCC_GPIOI_CLK_ENABLE(); } }
    else
    {
        assert(false && "Port not known for given pin id");
    }
}

// Static methods
static IRQn_Type GetIRQn(uint16_t id)
{
    assert(IsOnlyASingleBitSetInIdMask(id) == true);

         if (id & GPIO_PIN_0)  { return EXTI0_IRQn;     }
    else if (id & GPIO_PIN_1)  { return EXTI1_IRQn;     }
    else if (id & GPIO_PIN_2)  { return EXTI2_IRQn;     }
    else if (id & GPIO_PIN_3)  { return EXTI3_IRQn;     }
    else if (id & GPIO_PIN_4)  { return EXTI3_IRQn;     }
    else if (id & GPIO_PIN_5)  { return EXTI4_IRQn;     }
    else if (id < GPIO_PIN_10) { return EXTI9_5_IRQn;   }
    else                       { return EXTI15_10_IRQn; }
}


/************************************************************************/
/* Public Methods                                                       */
/************************************************************************/
/**
 * \brief   Copy constructor for pin.
 * \param   other   The object to copy from.
 */
Pin::Pin(Pin&& other)
{
    mId        = other.mId;
    mPort      = other.mPort;
    mDirection = other.mDirection;

    other.mId        = INVALID_ENTRY;
    other.mPort      = nullptr;
    other.mDirection = Direction::UNDEFINED;
}

Pin::Pin(PinIdPort idAndPort)
{
    CheckAndSetIdAndPort(idAndPort.id, idAndPort.port);
}

Pin::Pin(PinIdPort idAndPort, Level level)
{
    CheckAndSetIdAndPort(idAndPort.id, idAndPort.port);

	mDirection = Direction::OUTPUT;

	Configure(level);
}

Pin::Pin(PinIdPort idAndPort, PullUpDown pullUpDown)
{
    CheckAndSetIdAndPort(idAndPort.id, idAndPort.port);

	mDirection = Direction::INPUT;

	Configure(pullUpDown);
}

Pin::Pin(PinIdPort idAndPort, Alternate alternate)
{
    CheckAndSetIdAndPort(idAndPort.id, idAndPort.port);

    mDirection = Direction::ALTERNATE;

    Configure(alternate);
}

void Pin::Configure(Level level)
{
	assert((mDirection != Direction::UNDEFINED) && "Pin direction is undefined");

	CheckAndEnableAHB1PeripheralClock(mPort);

	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.Pin   = mId;
	GPIO_InitStructure.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull  = GPIO_NOPULL;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;

	HAL_GPIO_Init(mPort, &GPIO_InitStructure);

	Set(level);
}

void Pin::Configure(PullUpDown pullUpDown)
{
	assert((mDirection != Direction::UNDEFINED) && "Pin direction is undefined");

	CheckAndEnableAHB1PeripheralClock(mPort);

	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.Pin  = mId;
	GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
	switch (pullUpDown)
	{
		case PullUpDown::PULL_UP:      GPIO_InitStructure.Pull = GPIO_PULLUP;                   break;
		case PullUpDown::PULL_DOWN:    GPIO_InitStructure.Pull = GPIO_PULLDOWN;                 break;
		case PullUpDown::PULL_UP_DOWN: GPIO_InitStructure.Pull = (GPIO_PULLUP | GPIO_PULLDOWN); break;

		default:
		case PullUpDown::HIGHZ:        GPIO_InitStructure.Pull = GPIO_NOPULL;                   break;
	}
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;

	HAL_GPIO_Init(mPort, &GPIO_InitStructure);
}

// Untested, unfinished
void Pin::Configure(Alternate alternate)
{
    assert((mDirection != Direction::UNDEFINED) && "Pin direction is undefined");

    CheckAndEnableAHB1PeripheralClock(mPort);

    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.Pin       = mId;
    GPIO_InitStructure.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStructure.Speed     = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStructure.Alternate = static_cast<uint32_t>(alternate);

    HAL_GPIO_Init(mPort, &GPIO_InitStructure);
}

bool Pin::Interrupt(Edge edge, const std::function<void()>& callback, bool enabledAfterConfigure /** = true */)
{
	assert((mDirection != Direction::INPUT) && "Cannot configure interrupt if pin is not configured as input");
	assert(callback && "Cannot configure interrupt without callback");

	// First disable a possible configured interrupt
	const IRQn_Type irq = GetIRQn(mId);
    if (!IsIRQSharedWithOtherPin(mId))
    {
        HAL_NVIC_DisableIRQ(irq);
    }

    const auto index = GetIndexById(mId);
    // Check: if callback exits then pin is already configured as interrupt
    if (pinInterruptList[index].callback != nullptr)
    {
        // Restore NVIC for pin
        if (pinInterruptList[index].enabled)
        {
            HAL_NVIC_EnableIRQ(irq);
        }
        return false;
    }
    else
    {
        pinInterruptList[index].callback = callback;
        pinInterruptList[index].enabled  = enabledAfterConfigure;
    }

	GPIO_InitTypeDef GPIO_InitStructure;

	switch (edge)
	{
		case Edge::RISING:  GPIO_InitStructure.Mode = GPIO_MODE_IT_RISING;         break;
		case Edge::FALLING: GPIO_InitStructure.Mode = GPIO_MODE_IT_FALLING;        break;
		case Edge::BOTH:    GPIO_InitStructure.Mode = GPIO_MODE_IT_RISING_FALLING; break;
		default: assert(false && "Unknown edge configuration");
	}
	GPIO_InitStructure.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStructure.Pin  = mId;

	HAL_GPIO_Init(mPort, &GPIO_InitStructure);

	// Configure NVIC to generate interrupt
	HAL_NVIC_ClearPendingIRQ(irq);
	HAL_NVIC_SetPriority(irq, INTERRUPT_PRIORITY, 0);
	HAL_NVIC_EnableIRQ(irq);

	return true;
}

bool Pin::InterruptEnable()
{
    assert((mDirection == Direction::INPUT) && "Cannot enable interrupt if pin is not configured as input");

    const auto index = GetIndexById(mId);

    // Check: if callback exits then pin is already configured as interrupt
    if (pinInterruptList[index].callback != nullptr)
    {
        // Enable NVIC for pin
        const IRQn_Type irq = GetIRQn(mId);
        HAL_NVIC_EnableIRQ(irq);

        pinInterruptList[index].enabled = true;

        return true;
    }
    return false;
}

bool Pin::InterruptDisable()
{
    assert((mDirection == Direction::INPUT) && "Cannot disable interrupt if pin is not configured as input");

    const auto index = GetIndexById(mId);

    // Check: if callback exits then pin is already configured as interrupt
    if (pinInterruptList[index].callback != nullptr)
    {
        // Disable NVIC for pin
        if (!IsIRQSharedWithOtherPin(mId))
        {
            const IRQn_Type irq = GetIRQn(mId);
            HAL_NVIC_DisableIRQ(irq);
            HAL_NVIC_ClearPendingIRQ(irq);
        }

        pinInterruptList[index].enabled = false;

        return true;
    }
    return false;
}

bool Pin::InterruptRemove()
{
    assert((mDirection == Direction::INPUT) && "Cannot remove interrupt if pin is not configured as input");

    const auto index = GetIndexById(mId);

    // Check: if callback exits then pin is already configured as interrupt
    if (pinInterruptList[index].callback != nullptr)
    {
        // Disable NVIC for pin
        if (!IsIRQSharedWithOtherPin(mId))
        {
            const IRQn_Type irq = GetIRQn(mId);
            HAL_NVIC_DisableIRQ(irq);
            HAL_NVIC_ClearPendingIRQ(irq);
        }

        pinInterruptList[index].callback = nullptr;
        pinInterruptList[index].enabled  = false;

        return true;
    }
    return false;
}

/**
 * \brief   Toggles the output pin level (from high to low, or from low to high).
 */
void Pin::Toggle() const
{
    assert((mDirection == Direction::OUTPUT) && "Cannot toggle level if pin is not configured as output");

    (HAL_GPIO_ReadPin(mPort, mId) == GPIO_PIN_SET) ? HAL_GPIO_WritePin(mPort, mId, GPIO_PIN_RESET) :
                                                     HAL_GPIO_WritePin(mPort, mId, GPIO_PIN_SET);
}

/**
 * \brief   Set the output level on the pin.
 * \param   level   The output level to set.
 */
void Pin::Set(Level level)
{
	assert((mDirection == Direction::OUTPUT) && "Cannot set level if pin is not configured as output");

	(level == Level::HIGH) ? HAL_GPIO_WritePin(mPort, mId, GPIO_PIN_SET) :
							 HAL_GPIO_WritePin(mPort, mId, GPIO_PIN_RESET);
}

/**
 * \brief   Get the actual pin level.
 * \returns The actual level of the pin.
 */
Level Pin::Get() const
{
	switch (mDirection)
	{
	    case Direction::OUTPUT:
	    case Direction::INPUT:
	        return (HAL_GPIO_ReadPin(mPort, mId) == GPIO_PIN_SET) ? Level::HIGH : Level::LOW;
	        break;

	    case Direction::ALTERNATE:
	    case Direction::UNDEFINED:
	    default:
	        assert(false && "Cannot get pin level if pin not defined as input or output");
	        while (true) { __NOP(); }   // User must resolve this incorrect use of Get()
	        break;
	}
}

/**
 * \brief   Move assignment operator.
 * \param   other   The pin object to move.
 * \returns Moved a pin object.
 */
Pin& Pin::operator= (Pin&& other)
{
    if (this != &other)
    {
        mId        = other.mId;
        mPort      = other.mPort;
        mDirection = other.mDirection;

        other.mId        = INVALID_ENTRY;
        other.mPort      = nullptr;
        other.mDirection = Direction::UNDEFINED;
    }
    return *this;
}


/************************************************************************/
/* Private Methods                                                      */
/************************************************************************/
/**
 * \brief   Check if the id and port are valid, then stores them.
 * \param   id      ID of the pin [0..15]. Should contain only a single
 *                  bit in the bitmask.
 * \param   port    GPIO port on which the pin resides.
 */
void Pin::CheckAndSetIdAndPort(uint16_t id, GPIO_TypeDef* port)
{
    assert((port != nullptr) && "Invalid variable for port passed, cannot be nullptr");

    if (IsOnlyASingleBitSetInIdMask(id))
    {
        mId   = id;
        mPort = port;
    }
    else
    {
        assert(false && "Invalid id for pin: either GPIO_PIN_All or more than 1 bit in the mask provided");
    }
}


/************************************************************************/
/* Interrupts                                                           */
/************************************************************************/
void HAL_GPIO_EXTI_Callback(uint16_t id)
{
    // No nested vector priority issue as all interrupt priorities for pins are the same.
    const auto index = GetIndexById(id);

    if ((pinInterruptList[index].enabled == true) && (pinInterruptList[index].callback != nullptr))
    {
        pinInterruptList[index].callback();
    }
}

/**
 * @brief This function handles EXTI line0 interrupt.
 */
void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

/**
 * @brief This function handles EXTI line1 interrupt.
 */
void EXTI1_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
}

/**
 * @brief This function handles EXTI line2 interrupt.
 */
void EXTI2_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
}

/**
 * @brief This function handles EXTI line3 interrupt.
 */
void EXTI3_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);
}

/**
 * @brief This function handles EXTI line4 interrupt.
 */
void EXTI4_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
}

/**
 * @brief This function handles EXTI line[9:5] interrupts.
 */
void EXTI9_5_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_5);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_6);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_7);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_8);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_9);
}

/**
 * @brief This function handles EXTI line[15:10] interrupts.
 */
void EXTI15_10_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_10);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_11);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_14);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_15);
}

