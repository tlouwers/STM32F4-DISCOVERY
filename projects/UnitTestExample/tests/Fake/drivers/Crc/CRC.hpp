#ifndef FAKE_CRC_HPP_
#define FAKE_CRC_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include "interfaces/ICRC.hpp"


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class Crc final : public ICRC
{
public:
    Crc() {}
    virtual ~Crc() {}

    uint32_t Calculate(uint32_t* buffer, uint32_t length) override;
};


#endif  // FAKE_CRC_HPP_
