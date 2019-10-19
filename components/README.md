
# Description
LIS3DSH accelerometer class.

Used to configure and read X,Y,Z (motion) samples from the LIS3DSH accelerometer. 

# Requirements
* ST Microelectronics STM32F407G-DISC1
* Uses the SPI class
* Uses the DMA class
* C++11 is assumed
* Pins already configured for INT1/INT2

# Notes
The intent is to use the internal fifo, configured for 25 samples (X,Y,Z) of int16_t in size. When this watermark level is reached, INT1 is configured to toggle, after which a DMA transaction is started to read the samples from the fifo to temporary buffer (inside the LIS3DSH class). Once DMA is finished, a calback is called to let the calling class know the samples can be retrieved from temporary buffer. This intend here was to decouple the readout from ISR context.
 
# Examples
```cpp
// Declare the class (in Application.hpp for example):
#include "components/LIS3DSH.hpp"

// And declare the object and helper variables:
Pin mChipSelect;
Pin mMotionInt1;
Pin mMotionInt2;
SPI mSPI;
DMA mDMA_SPI_Tx;
DMA mDMA_SPI_Rx;
LIS3DSH mLIS3DSH;
std::atomic<bool> mMotionDataAvailable;
uint8_t mMotionLength;

// And the callback to call when data is available:
void MotionDataReceived(uint8_t length);

// In the Application.cpp constructor:
Application::Application() :
    mChipSelect(PIN_SPI1_CS, Level::HIGH),              // SPI ChipSelect for Motion
    mMotionInt1(PIN_MOTION_INT1, PullUpDown::HIGHZ),
    mMotionInt2(PIN_MOTION_INT2, PullUpDown::HIGHZ),
    mSPI(SPIInstance::SPI_1),
    mDMA_SPI_Tx(DMA::Stream::Dma2_Stream3),
    mDMA_SPI_Rx(DMA::Stream::Dma2_Stream0),
    mLIS3DSH(mSPI, PIN_SPI1_CS, PIN_MOTION_INT1, PIN_MOTION_INT2),
    mMotionDataAvailable(false),
    mMotionLength(0)
{
    // Configure the callback
    mLIS3DSH.SetHandler( [this](uint8_t length) { this->MotionDataReceived(length); } );
}

// Initialize the components:
bool Application::Init()
{
    // Configure DMA and SPI - note no checks for result for readability here.
    mDMA_SPI_Tx.Configure(DMA::Channel::Channel3, DMA::Direction::MemoryToPeripheral, DMA::BufferMode::Normal, DMA::Priority::Low, DMA::HalfBufferInterrupt::Disabled);
    mDMA_SPI_Rx.Configure(DMA::Channel::Channel3, DMA::Direction::PeripheralToMemory, DMA::BufferMode::Normal, DMA::Priority::Low, DMA::HalfBufferInterrupt::Disabled);
    mDMA_SPI_Tx.Link(mSPI.GetPeripheralHandle(), mSPI.GetDmaTxHandle());
    mDMA_SPI_Rx.Link(mSPI.GetPeripheralHandle(), mSPI.GetDmaRxHandle());
    mSPI.Init(SPI::Config(11, SPI::Mode::_3, 1000000));

	// Initialize the LIS3DSH accelerometer
    mLIS3DSH.Init(LIS3DSH::Config(LIS3DSH::SampleFrequency::_50_Hz));
    mMotionDataAvailable = false;
    mMotionLength = 0;

	// Start acquisition
    mLIS3DSH.Enable();
}

// The callback is called after the watermark level in the LIS3DSH fifo is reached, the data is read via SPI (and DMA) and available in temporary buffer in the LIS3DSH class.
void Application::MotionDataReceived(uint8_t length)
{
    mMotionDataAvailable = true;
    mMotionLength = length;
}

// The (main) Process loop can now read the data (decoupled from ISR):
void Application::Process()
{
	// Array to keep the samples
    static uint8_t motionArray[25 * 3 * 2] = {};

    if (mMotionDataAvailable)
    {
        mMotionDataAvailable = false;

		// Read samples from temporary buffer in LIS3DSH class into the 'motionArray'. Note no checks for result for readability here.
        mLIS3DSH.RetrieveAxesData(motionArray, mMotionLength);

        // ToDo: deinterleave to X,Y,Z samples, in int16_t format.
    }
}
```
