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
class CRC final : public ICRC
{
public:
    CRC() {}
    virtual ~CRC() {}

    uint32_t Calculate(uint32_t* buffer, uint32_t length) override;
};


#endif  // FAKE_CRC_HPP_
