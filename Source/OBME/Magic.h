/* 
    OBME additions to the 'Magic' container class
*/
#pragma once

// base classes
#include "API/Magic/Magic.h"

namespace OBME {

class Magic : public ::Magic
{
public:

    // 3-state hostility
    enum MagicHostility
    {   
        kHostility__NONE            = 0x0,
        kHostility_Neutral          = 1 << 0,
        kHostility_Hostile          = 1 << 1,
        kHostility_Beneficial       = 1 << 2,
    };

};


}   //  end namespace OBME