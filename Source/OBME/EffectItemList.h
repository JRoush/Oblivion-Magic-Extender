/* 
    OBME expansion of the EffectItemList.

    This class is *not* intended to be instantiated.  It is simply a convenient way to organize OBME methods
    that operate on vanilla EffectItemLists.  Standard useage is to static_cast a vanilla EffectItemList*
    into an OBME::EffectItemList*.  Any virtual methods introduced by this class will have to be manually appended
    to the vanilla vtbls.

*/
#pragma once

// base classes
#include "API/Magic/EffectItem.h"

namespace OBME {

// argument classes from OBME
class   EffectItem;

class EffectItemList : public ::EffectItemList
{
public:
    // methods - add & remove items
    _LOCAL void                 RemoveEffect(const EffectItem* item); // updating hostile effect count, clear item parent
    _LOCAL void                 AddEffect(EffectItem* item);  // updating hostile effect count, set item parent
    _LOCAL void                 ClearEffects(); // fixed - updates hostile effect count

    // methods - other effect lists
    _LOCAL void                 CopyEffectsFrom(const EffectItemList& copyFrom);    // allows ref incr/decr in CS

    // hooks & debugging
    _LOCAL static void          InitHooks();
};

}   // end namespace OBME