#include "Pin.hpp"


Pin::Pin(PinIdPort idAndPort)
{}
Pin::Pin(PinIdPort idAndPort, Level level, Drive drive)
{}
Pin::Pin(PinIdPort idAndPort, PullUpDown pullUpDown)
{}
Pin::Pin(PinIdPort idAndPort, Alternate alternate, PullUpDown pullUpDown, Mode mode)
{}

void Pin::Configure(Level level, Drive drive)
{}
void Pin::Configure(PullUpDown pullUpDown)
{}
void Pin::Configure(Alternate alternate, PullUpDown pullUpDown, Mode mode)
{}

bool Pin::Interrupt(Trigger trigger, const std::function<void()>& callback, bool enabledAfterConfigure)
{
    return true;
}

bool Pin::InterruptEnable()
{
    return true;
}

bool Pin::InterruptDisable()
{
    return true;
}

bool Pin::InterruptRemove()
{
    return true;
}

void Pin::Toggle() const
{}
void Pin::Set(Level level)
{}

Level Pin::Get() const
{
    return Level::LOW;
}
