
# HI_M1388AR
Driver for the HI_M1388AR 8x8 LED matrix display.

## Description
Intended use is to provide an easier means to work with the HI_M1388AR 8x8 LED matrix display. This class makes use of the SPI class.
Via the HI-M1388AR_Lib header file various digits, letters and symbols are provided for display.

## Requirements
- ST Microelectronics STM32F407G-DISC1 (can be ported easily to other ST microcontrollers)
- C++11
- SPI peripheral class
- Pins already configured for SPI

## Notes
If you happen to find an issue, and are able to provide a reproducible scenario I am happy to have a look. If you have a fix, or a refactoring that would improve the code please let me know so I can update it.
To easily design more items to display, have a look at: https://xantorohara.github.io/led-matrix-editor/#
 
## Example
```cpp
// Declare the required classes (in Application.hpp for example):
SPI        mSPI;
HI_M1388AR mMatrix;

// Construct the classes, fill the right parameters:
Application::Application() :
    mSPI(SPIInstance::SPI_2),
    mMatrix(mSPI, PIN_SPI2_CS)
{ }

// Initialize the class:
bool Application::Initialize()
{
    // Initialize SPI
    mSPI.Init(SPI::Config(11, SPI::Mode::_3, 1000000));

    // Initialize the HI-M1388AR
    mMatrix.Init(HI_M1388AR::Config(8));


    // Other stuff...


    return result;
}

// To diplay something on the screen:
{
	// Display a '0'
	mMatrix.WriteDigits(digit_zero);
	
	// Other stuff
	
	// Clear the display	
	mMatrix.ClearDisplay();
}
```
