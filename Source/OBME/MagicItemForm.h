/* 
    OBME expansion of the SpellItem, EnchantmentItem classes
*/
#pragma once

// base classes
#include "API/Magic/MagicItemForm.h"
#include "OBME/MagicGroup.h"

namespace OBME {

class SpellItem : public ::SpellItem
{
public:

    // hooks & debugging
    _LOCAL static void          InitHooks();
};

}   // end namespace OBME