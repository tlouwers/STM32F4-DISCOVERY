// ----------------------------------------------------------------------------
//  ImageCompatibility.cs
//
//  "THE BEER-WARE LICENSE" (Revision 42):
//  <terry.louwers@fourtress.nl> wrote this file. As long as you retain this
//  notice you can do whatever you want with this stuff. If we meet some day,
//  and you think this stuff is worth it, you can buy me a beer in return.
//                                                               Terry Louwers
//
//  Image sanity and device-compatibility gates shared by the GUI and CLI:
//  vector-table plausibility, header-declared size, product mismatch, and
//  version downgrade. Each check returns a human-readable reason, or null
//  when the check passes.
//
//  https://github.com/tlouwers/STM32F4-DISCOVERY/tree/develop/projects/Bootloader
//
//  Author:  T. Louwers <terry.louwers@fourtress.nl>
//  Version: 1.0
//  Date:    07-2026
// ----------------------------------------------------------------------------

namespace BootloaderTool.Protocol.Protocol;

/// <summary>
/// Pre-flash gates shared by every front-end, so a scripted CLI flash is held
/// to the same checks the GUI applies. Each method returns a plain-language
/// rejection/warning reason, or null when the check passes. Header-based
/// checks only bite when both sides declare — an unstamped image or an
/// erased/legacy device means no gate.
/// </summary>
public static class ImageCompatibility
{
    // STM32F407 RAM bounds for the stack-pointer plausibility check. End
    // addresses are exclusive; the stack pointer may equal the end (a
    // full-descending stack starts one past the top).
    private const uint SramBase = 0x20000000, SramEnd = 0x20020000;
    private const uint CcmBase  = 0x10000000, CcmEnd  = 0x10010000;

    /// <summary>
    /// Plausibility check on the Cortex-M vector table at the start of the image:
    /// the initial stack pointer must land in SRAM/CCM and the reset handler in
    /// flash with the Thumb bit set. Catches "wrong file" mistakes (an image for
    /// another MCU, a data blob, an .elf renamed to .bin) before anything is
    /// erased, without requiring any change to the image format.
    /// </summary>
    /// <param name="image">The loaded image to inspect.</param>
    /// <returns>A rejection reason, or null when the image looks genuine.</returns>
    public static string? CheckVectorTable(FirmwareImage image)
    {
        if (image.Size < 8)
            return $"Image is too small ({image.Size} bytes) to contain a vector table — not a firmware image.";

        uint sp = image.InitialStackPointer;
        bool spInSram = sp > SramBase && sp <= SramEnd;
        bool spInCcm  = sp > CcmBase  && sp <= CcmEnd;
        if (!spInSram && !spInCcm)
            return $"Not an STM32F407 application image — its stack pointer (0x{sp:X8}) is not in device RAM.";

        uint reset = image.ResetHandler;
        uint flashEnd = Stm32F4FlashLayout.FlashBase + Stm32F4FlashLayout.FlashSize;
        if ((reset & 1) == 0 || reset < Stm32F4FlashLayout.FlashBase || reset >= flashEnd)
            return $"Not an STM32F407 application image — its reset handler (0x{reset:X8}) is not in device flash.";

        return null;
    }

    /// <summary>
    /// Checks the size a stamped header declares against the actual file size.
    /// A mismatch means a truncated or padded file, not the binary the build
    /// produced. Unstamped images (or headers that declare no size) pass.
    /// </summary>
    /// <param name="image">The loaded image to inspect.</param>
    /// <returns>A rejection reason, or null when consistent.</returns>
    public static string? CheckDeclaredSize(FirmwareImage image)
    {
        ImageHeader? header = image.Header;
        if (header is not null && header.ImageSize != 0 && header.ImageSize != (uint)image.Size)
            return $"Image is damaged — its header declares {header.ImageSize:N0} bytes " +
                   $"but the file is {image.Size:N0} bytes.";

        return null;
    }

    /// <summary>
    /// Checks that the image and the connected device declare the same product.
    /// A mismatch blocks the flash: the image was built for different hardware.
    /// </summary>
    /// <param name="image">Header of the image to flash, or null when unstamped.</param>
    /// <param name="device">Header read back from the device, or null when unknown.</param>
    /// <returns>A rejection reason, or null when compatible (or undeclared).</returns>
    public static string? CheckProduct(ImageHeader? image, ImageHeader? device)
    {
        if (image is not null && device is not null
            && !string.Equals(image.Product, device.Product, StringComparison.Ordinal))
            return $"This image is built for '{image.Product}' — the connected device runs '{device.Product}'.";

        return null;
    }

    /// <summary>
    /// Checks whether the image is older than what the device currently runs.
    /// A downgrade is not an error, but front-ends require a deliberate
    /// confirmation (GUI "Flash anyway", CLI <c>--force</c>) before rolling back.
    /// </summary>
    /// <param name="image">Header of the image to flash, or null when unstamped.</param>
    /// <param name="device">Header read back from the device, or null when unknown.</param>
    /// <returns>A warning reason, or null when not a downgrade (or undeclared).</returns>
    public static string? CheckDowngrade(ImageHeader? image, ImageHeader? device)
    {
        if (image is not null && device is not null
            && image.CompareVersionTo(device) < 0)
            return $"The device runs {device.VersionText}; this image is {image.VersionText} (older).";

        return null;
    }
}
