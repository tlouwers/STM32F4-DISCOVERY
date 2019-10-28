# Description
LIS3DSH accelerometer class.

Intended use is to provide an easier means to work with the LIS3DSH accelerometer. This class makes use of the SPI and DMA class and is (per default) configured to readout accelerometer samples (X,Y,Z) at 52 Hz. The hardware FIFO of the LIS3DSH is used, meaning after 25 samples (X,Y,Z) the set threshold (or watermark level) signals to the microcontroller data is available, which is then read via DMA.

To be able to decouple from the ISR as much as possible, data is read to internal buffer in LIS3DSH class first using DMA, after which a callback data is available is triggered. The requesting/consuming class can then (outside ISR context) read the data.

# Requirements
* ST Microelectronics STM32F407G-DISC1 (can be ported easily to other ST microcontrollers)
* C++11 is assumed
* DMA utility class
* SPI peripheral class
* Pins already configured for SPI

# Notes
This is configured to read samples (X,Y,Z) - 16-bit each, at 52 Hz from the LIS3DSH. The hardware FIFO is used, the threshold (or watermark level) is set to 25 samples.
 
# Examples
```cpp
// Declare the required classes (in Application.hpp for example):
DMA mDMA_SPI_Tx;
DMA mDMA_SPI_Rx;
SPI mSPI;
LIS3DSH mLIS3DSH;

// Construct the classes, fill the right parameters:
Application::Application() :
    mDMA_SPI_Tx(DMA::Stream::Dma2_Stream3),
    mDMA_SPI_Rx(DMA::Stream::Dma2_Stream0),
    mSPI(SPIInstance::SPI_1),
    mLIS3DSH(mSPI, PIN_SPI1_CS, PIN_MOTION_INT1, PIN_MOTION_INT2)
{
    // Configure a callback to call when data is available.
    mLIS3DSH.SetHandler( [this](uint8_t length) { this->MotionDataReceived(length); } );
}

// Initialize the class:
bool Application::Initialize()
{
    // Configure DMA for SPI. Note: not checking for returned result for simplicity.
    mDMA_SPI_Tx.Configure(DMA::Channel::Channel3, DMA::Direction::MemoryToPeripheral, DMA::BufferMode::Normal, DMA::Priority::Low, DMA::HalfBufferInterrupt::Disabled);
    mDMA_SPI_Rx.Configure(DMA::Channel::Channel3, DMA::Direction::PeripheralToMemory, DMA::BufferMode::Normal, DMA::Priority::Low, DMA::HalfBufferInterrupt::Disabled);

    // Link DMA utility class with SPI
    mDMA_SPI_Tx.Link(mSPI.GetPeripheralHandle(), mSPI.GetDmaTxHandle());
    mDMA_SPI_Rx.Link(mSPI.GetPeripheralHandle(), mSPI.GetDmaRxHandle());

    // Initialize SPI
    mSPI.Init(SPI::Config(11, SPI::Mode::_3, 1000000));

    // Initialize the LIS3DSH
    mLIS3DSH.Init(LIS3DSH::Config(LIS3DSH::SampleFrequency::_50_Hz));

    // Helper variables
    mMotionDataAvailable = false;
    mMotionLength = 0;

    // Other stuff...

    // Start data acquisition with the LIS3DSH	
    result = mLIS3DSH.Enable();

    return result;
}

// Callback called for the motion data received callback.
void Application::MotionDataReceived(uint8_t length)
{
    // This only sets a flag, data is already read from LIS3DSH FIFO into
    // internal buffer. The application is to read this later.
    // Decouples reading data from ISR as much as possible.
    mMotionDataAvailable = true;
    mMotionLength = length;
}

// Main process loop of the application. This method is to be called
// often and acts as the main processor of data of the application.
void Application::Process()
{
    // Array to store received motion data. Note: this still needs conversion
    // to SI units (this is the RAW data)
    static uint8_t motionArray[25 * 3 * 2] = {};

    if (mMotionDataAvailable)
    {
        mMotionDataAvailable = false;

        // Read data from internal LIS3DSH buffer. Note: not checking for returned result for simplicity.
        bool retrieveResult = mLIS3DSH.RetrieveAxesData(motionArray, mMotionLength);

        // Deinterleave to X,Y,Z samples
        // Convert to SI units
        // Do something with the data
    }
}
```
