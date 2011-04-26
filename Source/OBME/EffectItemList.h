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
class   MagicItemEx;

class EffectItemList : public ::EffectItemList
{
public:    
    // methods - parents & siblings
    _LOCAL TESForm*             GetParentForm() const;
    _LOCAL MagicItemEx*         GetMagicItemEx() const;

    // methods - add, remove, & iterate items
    _LOCAL void                 RemoveEffect(const EffectItem* item); // updating hostile effect count, clear item parent
    _LOCAL void                 AddEffect(EffectItem* item);  // updating hostile effect count, set item parent
    _LOCAL void                 InsertBefore(EffectItem* item, EffectItem* position); // prepends to beginning of list if position not found
    _LOCAL void                 InsertAfter(EffectItem* item, EffectItem* position); // appends to end of list if position not found
    _LOCAL void                 MoveUp(EffectItem* item);
    _LOCAL void                 MoveDown(EffectItem* item);
    _LOCAL void                 ClearEffects(); // fixed - updates hostile effect count, detsroys list nodes as well as actual items
    _LOCAL UInt8                GetIndexOfEffect(const EffectItem* item) const; // reproduces game code, but returns -1 if not found

    // methods - other effect lists
    _LOCAL void                 CopyEffectsFrom(const EffectItemList& copyFrom);    // allows ref incr/decr in CS
    _LOCAL void                 CopyFromVanilla(::EffectItemList* source, bool clearList); // for copying existing vanilla lists

    // methods - aggregate properties
    //_LOCAL bool                 HasEffect(UInt32 mgefCode, UInt32 avCode); // use ValueModifierEfitHandler flags to regulate avCode check
    //_LOCAL bool                 HasEffectWithFlag(UInt32 mgefFlag); // run flag check through effectitem overrides, just for comleteness
    //_LOCAL bool                 HasHostileEffect(); // forward to MagicItemEx hostility check
    //_LOCAL bool                 HasAllHostileEffects(); // forawrd to MagicItemEx hostility check
    _LOCAL UInt32               GetAggregateHostilityMask();    // gets a mask of effect item hostilities (ignores hostileCount)
    //_LOCAL EffectItem*          GetStrongestEffect(UInt32 range = Magic::kRange__MAX, bool areaRequired = false); //
                                  // don't modify directly, but patch calls to implement appropriate MagicItemEx override fields
    #ifdef OBLIVION        
    //_LOCAL bool                 HasIgnoredEffect(); // used to diable casting of spells with ignored effectsm replace call to override flag purpose ?
    //_LOCAL UInt32               GetSchoolAV(); // forward to MagicItemEx school method
    //_LOCAL BSStringT            SkillRequiredText(); // forward to MagicItemEx menu rendering methods, patch calls?
    //_LOCAL BSStringT            MagicSchoolText(); // forward to MagicItemEx menu rendering methods, patch calls?
    #endif

    // methods - serialization & reference management
    //_LOCAL void                 SaveEffects();
    //_LOCAL void                 LoadEffects(TESFile& file, const char* parentEditorID);
    //_LOCAL bool                 RequiresObmeMgefChunks(); // returns true if obme-specific chunks are required to serialize this effect
    _LOCAL void                 LinkEffects();      // converts type codes into pointers where needed, and increments ref counts in CS
    _LOCAL void                 UnlinkEffects();    // converts pointers into typecodes where needed, and decrements ref counts in CS
    _LOCAL void                 ReplaceMgefCodeRef(UInt32 oldMgefCode, UInt32 newMgefCode); // replace all uses of old code with new code
    _LOCAL void                 RemoveFormReference(TESForm& form);

    // methods - CD dialog
    #ifndef OBLIVION
    static const UINT WM_QUERYDROPITEM = WM_APP + 0x60; // dispatched to target window during item drag & drop to notify window of potential drop
                                                        // WParam = EffectItem* to drop, LParam = POINT* of drop
                                                        // LResult = {0 for 'no drop' icon, -1 for no icon, other for 'add' icon} 
    static const UINT WM_DROPITEM      = WM_APP + 0x61; // dispatched to target window during item drag & drop to notify window of drop
                                                        // WParam = EffectItem* to drop, LParam = POINT* of drop
    _LOCAL bool                 EffectListDlgMsgCallback(HWND dialog, int uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result); 
    _LOCAL void                 InitializeDialogEffectList(HWND listView); 
    _LOCAL void                 CleanupDialogEffectList(HWND listView);
    _LOCAL static void CALLBACK GetEffectDisplayInfo(void* displayInfo); 
    _LOCAL static int CALLBACK  EffectItemComparator(const EffectItem& itemA, const EffectItem& itemB, int compareBy); 
    #endif

    // hooks & debugging
    _LOCAL static void          InitHooks();

    // private, undefined constructor & destructor to guarantee this class in not instantiated or inherited from by accident
private:
    _NOUSE EffectItemList();
    _NOUSE ~EffectItemList();
};

}   // end namespace OBME