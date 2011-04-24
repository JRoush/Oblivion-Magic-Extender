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
        kHostility_Neutral          = 0x1,
        kHostility_Hostile          = 0x2,
        kHostility_Beneficial       = 0x4,
    };

};


}   //  end namespace OBME