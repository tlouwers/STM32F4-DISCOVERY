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
 *          State is preserved (partly) within the hardware.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/master/drivers/Pin
 *
 * \details Intended use is to have a method at board startup which sets each
 *          pin to a defined state. This is done by constructing a Pin object,
 *          and let it go out of scope.
 *          Later in the application, for the few pins where needed, pass along
 *          the PinIdPort struct to the class where a pin object is needed.
 *          Then during the initialisation of that class (not construction)
 *          create and fill the Pin object with desired values. At this point
 *          the interrupts can be configured as well.
 *
 * \author      T. Louwers <terry.louwers@fourtress.nl>
 * \version     1.0
 * \date        03-2019
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include "Pin.hpp"
#include "utility/SlimAssert.h"


/************************************************************************/
/* Constants                                                            */
/************************************************************************/
/**
 * \brief   Number indicating an invalid entry.
 * \note    Assumed is that no pin is using this as id.
 */
static constexpr uint16_t INVALID_ENTRY = UINT16_MAX;

/**
 * \brief   Default interrupt priority of all pins.
 */
static constexpr uint8_t INTERRUPT_PRIORITY = 5;


/************************************************************************/
/* Static variables                                                     */
/************************************************************************/
/**
 * \brief   Internal administration to keep track for which pin a callback
 *          is subscribed and if it is enabled or not.
 */
static PinInterrupt pinInterruptList[16] = {};


/************************************************************************/
/* Static functions                                                     */
/************************************************************************/
/**
 * \brief   Check if only a single bit is set in the id GPIO bitmask.
 */
static bool IsOnlyASingleBitSetInIdMask(uint16_t id)
{
    return ((id > 0) && ((id & (id - 1)) == 0));
}

/**
 * \brief   Get an array index based upon the pin id.
 * \details Counting trailing zeroes results an array index.
 *          Works as id is represented as uint16_t bitmask: 'GPIO_pins_define'.
 * \note    https://www.go4expert.com/articles/builtin-gcc-functions-builtinclz-t29238/
 */
static int GetIndexById(uint16_t id)
{
    return __builtin_ctz(id);
}

/**
 * \brief   Check to see if the IRQ is shared with another pin which is
 *          configured or not.
 * \param   id      Id of the pin for which the check is done.
 * \returns True if another pin is configured as interrupt on a shared
 *          EXT interrupt line, else false.
 */
static bool IsIRQSharedWithOtherPin(uint16_t id)
{
    bool result = true;

    if (id < GPIO_PIN_5 )
    {
        return false;
    }
    else if (id < GPIO_PIN_10)
    {
        if (id !=  5) { result &= (pinInterruptList[5].callback  == nullptr); }
        if (id !=  6) { result &= (pinInterruptList[6].callback  == nullptr); }
        if (id !=  7) { result &= (pinInterruptList[7].callback  == nullptr); }
        if (id !=  8) { result &= (pinInterruptList[8].callback  == nullptr); }
        if (id !=  9) { result &= (pinInterruptList[9].callback  == nullptr); }
    }
    else
    {
        if (id != 10) { result &= (pinInterruptList[10].callback == nullptr); }
        if (id != 11) { result &= (pinInterruptList[11].callback == nullptr); }
        if (id != 12) { result &= (pinInterruptList[12].callback == nullptr); }
        if (id != 13) { result &= (pinInterruptList[13].callback == nullptr); }
        if (id != 14) { result &= (pinInterruptList[14].callback == nullptr); }
        if (id != 15) { result &= (pinInterruptList[15].callback == nullptr); }
    }

    return !result;
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
    ASSERT(port != nullptr);        // Invalid variable for port passed, cannot be nullptr"

         if (port == GPIOA) { if (__HAL_RCC_GPIOA_IS_CLK_DISABLED()) { __HAL_RCC_GPIOA_CLK_ENABLE(); } }
    else if (port == GPIOB) { if (__HAL_RCC_GPIOB_IS_CLK_DISABLED()) { __HAL_RCC_GPIOB_CLK_ENABLE(); } }
    else if (port == GPIOC) { if (__HAL_RCC_GPIOC_IS_CLK_DISABLED()) { __HAL_RCC_GPIOC_CLK_ENABLE(); } }
    else if (port == GPIOD) { if (__HAL_RCC_GPIOD_IS_CLK_DISABLED()) { __HAL_RCC_GPIOD_CLK_ENABLE(); } }
    else if (port == GPIOE) { if (__HAL_RCC_GPIOE_IS_CLK_DISABLED()) { __HAL_RCC_GPIOE_CLK_ENABLE(); } }
    else if (port == GPIOF) { if (__HAL_RCC_GPIOF_IS_CLK_DISABLED()) { __HAL_RCC_GPIOF_CLK_ENABLE(); } }
    else if (port == GPIOG) { if (__HAL_RCC_GPIOG_IS_CLK_DISABLED()) { __HAL_RCC_GPIOG_CLK_ENABLE(); } }
    else if (port == GPIOH) { if (__HAL_RCC_GPIOH_IS_CLK_DISABLED()) { __HAL_RCC_GPIOH_CLK_ENABLE(); } }
    else if (port == GPIOI) { if (__HAL_RCC_GPIOI_IS_CLK_DISABLED()) { __HAL_RCC_GPIOI_CLK_ENABLE(); } }
    else
    {
        ASSERT(false);              // Port not known for given pin id
    }
}

/**
 * \brief   Get the IRQ belonging to the pin.
 * \returns The EXT interrupt line IRQ to which the pin belongs.
 */
static IRQn_Type GetIRQn(uint16_t id)
{
    ASSERT(IsOnlyASingleBitSetInIdMask(id) == true);

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
 * \brief   Move constructor for pin.
 * \param   other   The object to move.
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

/**
 * \brief   Constructor for pin.
 * \details Default constructor for a pin. This does not configure the pin,
 *          be sure to do this before using it.
 * \param   idAndPort   Pin id and port to which the pin belongs.
 */
Pin::Pin(PinIdPort idAndPort)
{
    CheckAndSetIdAndPort(idAndPort.id, idAndPort.port);
}

/**
 * \brief   Constructor for pin as output.
 * \param   idAndPort   Pin id and port to which the pin belongs.
 * \param   level       Initial output level of the pin.
 * \param   drive       Drive mode configuration, default push pull.
 */
Pin::Pin(PinIdPort idAndPort, Level level, Drive drive /* = Drive::PUSH_PULL */)
{
    CheckAndSetIdAndPort(idAndPort.id, idAndPort.port);

	mDirection = Direction::OUTPUT;

	Configure(level, drive);
}

/**
 * \brief   Constructor for pin as input.
 * \param   idAndPort   Pin id and port to which the pin belongs.
 * \param   pullUpDown  Pull up or pull down mode configuration.
 */
Pin::Pin(PinIdPort idAndPort, PullUpDown pullUpDown)
{
    CheckAndSetIdAndPort(idAndPort.id, idAndPort.port);

	mDirection = Direction::INPUT;

	Configure(pullUpDown);
}

/**
 * \brief   Constructor for pin as alternate function.
 * \param   idAndPort   Pin id and port to which the pin belongs.
 * \param   alternate   The alternate function for the pin.
 * \param   pullUpDown  Pull up or pull down mode configuration.
 * \note    If pullUpDown is set to HIGHZ it is ignored.
 */
Pin::Pin(PinIdPort idAndPort, Alternate alternate, PullUpDown pullUpDown /* = PullUpDown::HIGHZ */)
{
    CheckAndSetIdAndPort(idAndPort.id, idAndPort.port);

    mDirection = Direction::ALTERNATE;

    Configure(alternate, pullUpDown);
}

/**
 * \brief   Configuration method for pin as output.
 * \param   level   Initial output level of the pin.
 * \param   drive   Drive mode configuration, default push pull.
 */
void Pin::Configure(Level level, Drive drive /* = Drive::PUSH_PULL */)
{
    ASSERT(mDirection != Direction::UNDEFINED);     // Pin direction is undefined

    CheckAndEnableAHB1PeripheralClock(mPort);

    GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_InitStructure.Pin  = mId;
    GPIO_InitStructure.Mode = (drive == Drive::PUSH_PULL) ? GPIO_MODE_OUTPUT_PP : GPIO_MODE_OUTPUT_OD;
    switch (drive)
    {
        case Drive::PUSH_PULL:               // Fall through
        case Drive::OPEN_DRAIN:              GPIO_InitStructure.Pull = GPIO_NOPULL;                   break;
        case Drive::OPEN_DRAIN_PULL_UP: 	 GPIO_InitStructure.Pull = GPIO_PULLUP;                   break;
        case Drive::OPEN_DRAIN_PULL_DOWN:    GPIO_InitStructure.Pull = GPIO_PULLDOWN;                 break;
        case Drive::OPEN_DRAIN_PULL_UP_DOWN: GPIO_InitStructure.Pull = (GPIO_PULLUP | GPIO_PULLDOWN); break;
        default: ASSERT(false);              GPIO_InitStructure.Pull = GPIO_NOPULL;                   break;    // Invalid drive
    }
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(mPort, &GPIO_InitStructure);

    mDirection = Direction::OUTPUT;

    Set(level);
}

/**
 * \brief   Configuration method for pin as input.
 * \param   pullUpDown  Pull up or pull down mode configuration.
 */
void Pin::Configure(PullUpDown pullUpDown)
{
    ASSERT(mDirection != Direction::UNDEFINED);     // Pin direction is undefined

    CheckAndEnableAHB1PeripheralClock(mPort);

    GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_InitStructure.Pin  = mId;
    GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
    switch (pullUpDown)
    {
        case PullUpDown::UP:      GPIO_InitStructure.Pull = GPIO_PULLUP;                   break;
        case PullUpDown::DOWN:    GPIO_InitStructure.Pull = GPIO_PULLDOWN;                 break;
        case PullUpDown::UP_DOWN: GPIO_InitStructure.Pull = (GPIO_PULLUP | GPIO_PULLDOWN); break;
        case PullUpDown::HIGHZ:   // Fall through
        default:                  GPIO_InitStructure.Pull = GPIO_NOPULL;                   break;
    }
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(mPort, &GPIO_InitStructure);

    mDirection = Direction::INPUT;
}

/**
 * \brief   Configuration method for pin as alternate function.
 * \param   alternate   The alternate function for the pin.
 * \param   pullUpDown  Pull up or pull down mode configuration.
 * \note    If pullUpDown is set to HIGHZ it is ignored.
 */
void Pin::Configure(Alternate alternate, PullUpDown pullUpDown /* = PullUpDown::HIGHZ */)
{
    ASSERT(mDirection != Direction::UNDEFINED);     // Pin direction is undefined

    CheckAndEnableAHB1PeripheralClock(mPort);

    GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_InitStructure.Pin  = mId;
    GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
    switch (pullUpDown)
    {
        case PullUpDown::UP:      GPIO_InitStructure.Pull = GPIO_PULLUP;                   break;
        case PullUpDown::DOWN:    GPIO_InitStructure.Pull = GPIO_PULLDOWN;                 break;
        case PullUpDown::UP_DOWN: GPIO_InitStructure.Pull = (GPIO_PULLUP | GPIO_PULLDOWN); break;
        case PullUpDown::HIGHZ:   // Fall through
        default:                  GPIO_InitStructure.Pull = GPIO_NOPULL;                   break;
    }
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStructure.Alternate = static_cast<uint8_t>(alternate);

    HAL_GPIO_Init(mPort, &GPIO_InitStructure);

    mDirection = Direction::ALTERNATE;
}

/**
 * \brief   Configures an interrupt for a pin.
 * \param   trigger                 The trigger condition for the interrupt.
 * \param   callback                The callback to call when the interrupt occurs.
 * \param   enableAfterConfigure    Flag indicating the interrupt should be enabled
 *                                  after the method is done, default is enabled.
 * \note    This method assumes the HAL has set NVIC_PRIORITYGROUP_4.
 * \returns True if the interrupt could be configured, else false.
 */
bool Pin::Interrupt(Trigger trigger, const std::function<void()>& callback, bool enableAfterConfigure /* = true */)
{
    ASSERT(mDirection == Direction::INPUT);     // Cannot configure interrupt if pin is not configured as input
    ASSERT(callback);                           // Cannot configure interrupt without callback

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
        pinInterruptList[index].enabled  = enableAfterConfigure;
    }

	GPIO_InitTypeDef GPIO_InitStructure = {0};

    switch (trigger)
    {
        case Trigger::RISING:  GPIO_InitStructure.Mode = GPIO_MODE_IT_RISING;         break;
        case Trigger::FALLING: GPIO_InitStructure.Mode = GPIO_MODE_IT_FALLING;        break;
        case Trigger::BOTH:    GPIO_InitStructure.Mode = GPIO_MODE_IT_RISING_FALLING; break;
        default: ASSERT(false);                                                       break;    // Unknown trigger configuration
    }
    GPIO_InitStructure.Pin = mId;

    HAL_GPIO_Init(mPort, &GPIO_InitStructure);

    // Configure NVIC to generate interrupt
    HAL_NVIC_ClearPendingIRQ(irq);
    HAL_NVIC_SetPriority(irq, INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(irq);

    return true;
}

/**
 * \brief   Enable a previously configured interrupt for a pin.
 * \returns True if the interrupt could be enabled, else false.
 */
bool Pin::InterruptEnable()
{
    ASSERT(mDirection == Direction::INPUT);     // Cannot enable interrupt if pin is not configured as input

    const auto index = GetIndexById(mId);

    // Check: if callback exits then pin is already configured as interrupt
    if (pinInterruptList[index].callback != nullptr)
    {
        // Enable NVIC for pin
        HAL_NVIC_EnableIRQ( GetIRQn(mId) );

        pinInterruptList[index].enabled = true;

        return true;
    }
    return false;
}

/**
 * \brief   Disable a previously configured interrupt for a pin.
 * \returns True if the interrupt could be disabled, else false.
 */
bool Pin::InterruptDisable()
{
    ASSERT(mDirection == Direction::INPUT);     // Cannot disable interrupt if pin is not configured as input

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

/**
 * \brief   Removes a previously configured interrupt for a pin.
 * \returns True if the interrupt could be removed, else false.
 */
bool Pin::InterruptRemove()
{
    ASSERT(mDirection == Direction::INPUT);     // Cannot remove interrupt if pin is not configured as input

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
    ASSERT(mDirection == Direction::OUTPUT);    // Cannot toggle level if pin is not configured as output

    (HAL_GPIO_ReadPin(mPort, mId) == GPIO_PIN_SET) ? HAL_GPIO_WritePin(mPort, mId, GPIO_PIN_RESET) :
                                                     HAL_GPIO_WritePin(mPort, mId, GPIO_PIN_SET);
}

/**
 * \brief   Set the output level on the pin.
 * \param   level   The output level to set.
 */
void Pin::Set(Level level)
{
    ASSERT(mDirection == Direction::OUTPUT);    // Cannot set level if pin is not configured as output

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
        case Direction::UNDEFINED:      // Fall through
        default:
            ASSERT(false);              // Cannot get pin level if pin not defined as input or output
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
    ASSERT(port != nullptr);        // Invalid variable for port passed, cannot be nullptr

    if (IsOnlyASingleBitSetInIdMask(id))
    {
        mId   = id;
        mPort = port;
    }
    else
    {
        ASSERT(false);              // Invalid id for pin: either GPIO_PIN_All or more than 1 bit in the mask provided
    }
}


/************************************************************************/
/* Interrupts                                                           */
/************************************************************************/
/**
 * \brief   ISR: handler to dispatch line interrupt into configured pin
 *          callback. If interrupt for pin is not enabled the interrupt is
 *          absorbed here.
 */
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
 * \brief   ISR: route EXT line[0] interrupt to 'HAL_GPIO_EXTI_IRQHandler'.
 */
extern "C" void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

/**
 * \brief   ISR: route EXT line[1] interrupt to 'HAL_GPIO_EXTI_IRQHandler'.
 */
extern "C" void EXTI1_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
}

/**
 * \brief   ISR: route EXT line[2] interrupt to 'HAL_GPIO_EXTI_IRQHandler'.
 */
extern "C" void EXTI2_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
}

/**
 * \brief   ISR: route EXT line[3] interrupt to 'HAL_GPIO_EXTI_IRQHandler'.
 */
extern "C" void EXTI3_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);
}

/**
 * \brief   ISR: route EXT line[4] interrupt to 'HAL_GPIO_EXTI_IRQHandler'.
 */
extern "C" void EXTI4_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
}

/**
 * \brief   ISR: route EXT line[9:5] interrupts to 'HAL_GPIO_EXTI_IRQHandler'.
 * \note    Cannot make distinction between the pin ids listed below.
 */
extern "C" void EXTI9_5_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_5);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_6);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_7);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_8);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_9);
}

/**
 * \brief   ISR: route EXT line[15..10] interrupts to 'HAL_GPIO_EXTI_IRQHandler'.
 * \note    Cannot make distinction between the pin ids listed below.
 */
extern "C" void EXTI15_10_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_10);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_11);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_14);
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_15);
}

