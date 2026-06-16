#ifndef BL_APPLICATION_HPP_
#define BL_APPLICATION_HPP_

#include <cstdint>

#include "drivers/Led.hpp"
#include "drivers/Button.hpp"
#include "bootloader/BootloaderEntry.hpp"

class Application
{
public:
    // Command byte that requests a factory reset (jump to ST bootloader).
    static constexpr uint8_t kFactoryResetCommand = 0x52;   // 'R'

    explicit Application(BootloaderEntry& bootloaderEntry);
    bool Init();
    void Process();
    void HandleCommand(uint8_t command);
    void Error();

private:
    BootloaderEntry& mBootloaderEntry;
};

#endif // BL_APPLICATION_HPP_
