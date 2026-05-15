#ifndef FAKE_CRC_HPP_
#define FAKE_CRC_HPP_

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <cstdint>
#include "interfaces/ICrc.hpp"


/************************************************************************/
/* Class declaration                                                    */
/************************************************************************/
class Crc final : public ICrc
{
public:
    Crc() {}
    virtual ~Crc() {}

    bool Calculate(const uint32_t* buffer, uint32_t length, uint32_t& out) override;
};


#endif  // FAKE_CRC_HPP_
