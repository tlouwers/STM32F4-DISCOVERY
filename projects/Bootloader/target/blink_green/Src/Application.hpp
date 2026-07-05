/**
 * \file    Application.hpp
 *
 * \licence "THE BEER-WARE LICENSE" (Revision 42):
 *          <terry.louwers@fourtress.nl> wrote this file. As long as you retain
 *          this notice you can do whatever you want with this stuff. If we
 *          meet some day, and you think this stuff is worth it, you can buy me
 *          a beer in return.
 *                                                                Terry Louwers
 * \class   Application
 *
 * \brief   Composition root for the blink_green sample application.
 *
 * \details Blinks the green LED and provides two paths into a factory reset
 *          (jump to the ST system memory bootloader): the onboard user button,
 *          or a single-byte command handed in by the running application's
 *          host protocol.
 *
 * \note    https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader/target/blink_green/Src
 *
 * \author  T. Louwers <terry.louwers@fourtress.nl>
 * \version 1.0
 * \date    06-2026
 */

#ifndef APPLICATION_HPP_
#define APPLICATION_HPP_


/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>

#include "drivers/Led.hpp"
#include "drivers/Button.hpp"
#include "bootloader/BootloaderEntry.hpp"


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class Application
{
public:
    /** Command byte that requests a factory reset (jump to ST bootloader). */
    static constexpr uint8_t kFactoryResetCommand = 0x52;   // 'R'

    explicit Application(BootloaderEntry& bootloaderEntry);

    bool Init();
    void Process();
    void HandleCommand(uint8_t command);
    void Error();

    // Explicit disabled constructors/operators
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

private:
    BootloaderEntry& mBootloaderEntry;
};


#endif  // APPLICATION_HPP_
