#ifndef BL_APPLICATION_HPP_
#define BL_APPLICATION_HPP_

#include "drivers/Led.hpp"

class Application
{
public:
    Application();
    bool Init();
    void Process();
    void Error();
};

#endif // BL_APPLICATION_HPP_
