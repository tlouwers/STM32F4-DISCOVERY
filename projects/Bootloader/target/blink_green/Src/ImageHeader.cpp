// ----------------------------------------------------------------------------
//  ImageHeader.cpp
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  TLFWIMG1 metadata header (docs/image-header.md), version kept low for the
//  L56 downgrade-gate hardware verification (this app plays the "older image"
//  role, flashed onto a device that already runs blink_orange's higher
//  version).
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    07-2026
// ----------------------------------------------------------------------------

#include <cstdint>

struct ImageHeader
{
    char     magic[8];
    char     product[8];
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
    uint16_t reserved;
    uint32_t imageSize;
    uint32_t headerCrc;
};
static_assert(sizeof(ImageHeader) == 32, "ImageHeader must be exactly 32 bytes");

__attribute__((section(".image_header"), used))
const ImageHeader gImageHeader =
{
    { 'T','L','F','W','I','M','G','1' },
    { 'F','4','D','I','S','C','O','1' },
    1, 0, 0,
    0, 0, 0
};
