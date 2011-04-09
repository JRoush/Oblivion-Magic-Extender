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
    _LOCAL void                 ClearEffects(); // fixed - updates hostile effect count, detsroys list nodes as well as actual items

    // methods - other effect lists
    _LOCAL void                 CopyEffectsFrom(const EffectItemList& copyFrom);    // allows ref incr/decr in CS

    // methods - aggregate properties
    //_LOCAL bool                 HasEffect(UInt32 mgefCode, UInt32 avCode); // use ValueModifierEfitHandler flags to regulate avCode check
    //_LOCAL bool                 HasEffectWithFlag(UInt32 mgefFlag); // run flag check through effectitem overrides, just for comleteness
    //_LOCAL bool                 HasHostileEffect(); // forward to MagicItemEx hostility check
    //_LOCAL bool                 HasAllHostileEffects(); // forawrd to MagicItemEx hostility check
    //_LOCAL EffectItem*          GetStrongestEffect(UInt32 range = Magic::kRange__MAX, bool areaRequired = false); //
                                  // don't modify directly, but patch calls to implement appropriate MagicItemEx override fields
    #ifdef OBLIVION        
    //_LOCAL bool                 HasIgnoredEffect(); // used to diable casting of spells with ignored effectsm replace call to override flag purpose ?
    //_LOCAL UInt32               GetSchoolAV(); // forward to MagicItemEx school method
    //_LOCAL BSStringT            SkillRequiredText(); // forward to MagicItemEx menu rendering methods, patch calls?
    //_LOCAL BSStringT            MagicSchoolText(); // forward to MagicItemEx menu rendering methods, patch calls?
    #endif

    // methods - serialization
    //_LOCAL void                 SaveEffects();
    //_LOCAL void                 LoadEffects(TESFile& file, const char* parentEditorID);

    // methods - CD dialog
    #ifndef OBLIVION
    //_LOCAL static void          InitializeDialogEffectList(HWND listView);    // support new columns
    //_LOCAL static void CALLBACK GetEffectDisplayInfo(void* displayInfo);     // support new columns
    //_LOCAL static int CALLBACK  EffectItemComparator(const EffectItem& itemA, const EffectItem& itemB, int compareBy);    // support new columns
    #endif

    // hooks & debugging
    _LOCAL static void          InitHooks();

    // private, undefined constructor & destructor to guarantee this class in not instantiated or inherited from by accident
private:
    _NOUSE EffectItemList();
    _NOUSE ~EffectItemList();
};

}   // end namespace OBME