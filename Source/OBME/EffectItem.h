/* 
    OBME expansion of the EffectItem

    OBME::EffectItem will replace all instances ::EffectItem, but OBME::EffectItemList will be a purely
    noninstantiated class, used only for new methods.  Because MagicItem & SigilStone will still use
    the original EffectItemList, all OBME code will have to explicitly cast to OBME::EffectItem where
    necessary.
*/
#pragma once

// base classes
#include "API/Magic/EffectItem.h"
#include "API/BSTypes/BSStringT.h"

namespace OBME {

// argument classes from OBME
class   EfitHandler;
class   EffectSetting;
class   EfitHandler;

class EffectItem : public ::EffectItem
{
public:

    // EffectItem flag shifts
    enum EffectItemFlagShifts
    {     
        kEfitFlagShift_ScriptFormID         = 0x00, // Mask: use script formID
        kEfitFlagShift_School               = 0x01, // Mask: use school override
        kEfitFlagShift_Name                 = 0x02, // Mask: override effect name   Flags: override whole effect descriptor
        kEfitFlagShift_FXMgef               = 0x03, // Mask: use FX override
        kEfitFlagShift_Cost                 = 0x04, // Mask: override base cost     Flags: override entire magicka autocalc for effect item   
        kEfitFlagShift_ResistAV             = 0x05, // Mask: use resistAV  
        kEfitFlagShift_Icon                 = 0x06, // Mask: use icon  
    };

    // extension of ScriptEffectInfo struct for additional fields
    class EffectItemExtra : public ScriptEffectInfo
    {
    public:
        // members - covered by override masks
        //     /*00/00*/ UInt32             scriptFormID; // handler specific
        //     /*04/04*/ UInt32             school; 
        //     /*08/08*/ BSStringT          name;   // effect name, or complete effect descriptor if flag set
        //     /*10/10*/ UInt32             fxMgefCode;
        //     /*14/14*/ bool               hostile; // masked by Hostile flag in mgefFlagMask
        //     /*15/15*/ UInt8              pad15[3];
        MEMBER /*++/++*/ UInt64             mgefFlags; 
        MEMBER /*++/++*/ UInt64             mgefFlagMask; 
        MEMBER /*++/++*/ UInt16             efitFlags;
        MEMBER /*++/++*/ UInt16             efitFlagMask;
        MEMBER /*++/++*/ float              baseCost; // base cost, or total cost is flag set
        MEMBER /*++/++*/ UInt32             resistAV; // resistance avCode.
        MEMBER /*++/++*/ BSStringT          iconPath; // overrides effectsetting icon

        // members - nonoverride fields
        MEMBER /*++/++*/ ::EffectItemList*  parentList; // pointer to parent effect item list
        MEMBER /*++/++*/ EfitHandler*       effectHandler;  // handler object
        MEMBER /*++/++*/ SInt32             projectileRange;  // projectile range

        // constructor, destructor, validator
        _LOCAL bool         IsEmpty(); // returns true if all fields/override masks are set to default values
        _LOCAL EffectItemExtra();  // zero initialize, with no overrides enabled
        _LOCAL ~EffectItemExtra(); // destroy name+icon strings and handler object
    };

    // methods - other effect items
    _LOCAL bool                 CompareTo(const EffectItem& compareTo) const;
    _LOCAL void                 CopyFrom(const EffectItem& copyFrom); 
    #ifdef OBLIVION
    //_LOCAL bool                 Match(const EffectItem& compareTo);
    #endif

    // methods - non-override parameters
    INLINE EffectSetting*       GetEffectSetting() const {return (OBME::EffectSetting*)effect;} // for convenience
    _LOCAL void                 SetEffectSetting(const EffectSetting& mgef);
    _LOCAL ::EffectItemList*    GetParentList() const;
    _LOCAL void                 SetParentList(::EffectItemList* list);
    _LOCAL EfitHandler*         GetHandler() const;
    _LOCAL void                 SetHandler(EfitHandler* handler);
    _LOCAL SInt32               GetProjectileRange() const;             // returns -1 if effect cannot or does not have projectile range
    _LOCAL bool                 SetProjectileRange(SInt32 value);       // returns false on failure 

    // methods - overrides
    _LOCAL bool                 IsEfitFieldOverridden(UInt32 flagShift) const;
    _LOCAL void                 SetEfitFieldOverride(UInt32 flagShift, bool value);
    _LOCAL bool                 IsMgefFlagOverridden(UInt32 flagShift) const;
    _LOCAL void                 SetMgefFlagOverride(UInt32 flagShift, bool value);
    _LOCAL BSStringT            GetEffectName() const;  // returns effect name, or override name if not overriding whole descriptor
    _LOCAL void                 SetEffectName(const char* name);        // set effect name override 
    _LOCAL UInt32               GetSchool() const;
    _LOCAL void                 SetSchool(UInt32 schoolCode);           // set school override
    _LOCAL EffectSetting*       GetFXEffect() const;
    _LOCAL void                 SetFXEffect(UInt32 mgefCode);           // set fx effect override
    _LOCAL UInt8                GetHostility() const;
    _LOCAL void                 SetHostility(UInt8 hostility);          // set hostility (both hostile and beneficial flag) overrides
    _LOCAL bool                 GetMgefFlag(UInt32 flagShift) const;
    _LOCAL void                 SetMgefFlag(UInt32 flagShift, bool value);  // set mgef flag override
    _LOCAL bool                 GetEfitFlag(UInt32 flagShift) const;
    _LOCAL void                 SetEfitFlag(UInt32 flagShift, bool value);  // set efit flag override
    _LOCAL float                GetBaseCost() const;
    _LOCAL void                 SetBaseCost(float nCost);               // set base cost override
    _LOCAL UInt32               GetResistAV() const;
    _LOCAL void                 SetResistAV(UInt32 avCode);             // set resistAV override
    _LOCAL const char*          GetEffectIcon() const;
    _LOCAL void                 SetEffectIcon(const char* iconPath);    // set icon override 

    // methods - calculated properties
    _LOCAL BSStringT            GetDisplayText() const;
    _LOCAL void                 SetDisplayText(const char* text);       // set effect descriptor override
    #ifdef OBLIVION    
    //_LOCAL BSStringT            GetDisplayText(UInt32 magicType, float effectiveness, bool noDuration, bool noRange, bool onStrikeRange) const; 
    //_LOCAL BSStringT            GetDisplayText(const MagicItem& parentItem, float effectiveness = 1.0) const; 
    //_LOCAL void                 GetAVQualifiedName(char* buffer) const;     
    #endif
    _LOCAL float                MagickaCost(); // get no-caster magicka cost
    _LOCAL float                MagickaCostForCaster(Actor* caster = 0); // magicka cost modified by caster skill
    _LOCAL void                 SetMagickaCost(float nCost); // set overall no-caster cost override

    // methods - serialization & reference management
    //_LOCAL bool                 Load(TESFile& file, const char* parentEditorID);
    //_LOCAL void                 Save(); 
    _LOCAL void                 Link(); // converts type codes into pointers where needed, and increments ref counts in CS
    _LOCAL void                 Unlink(); // converts pointers to typecodes where needed, and decrements ref counts in CS
    _LOCAL void                 ReplaceMgefCodeRef(UInt32 oldMgefCode, UInt32 newMgefCode); // replace all uses of old code with new code
    #ifndef OBLIVION
    _LOCAL void                 RemoveFormReference(TESForm& form);
    #endif

    // methods - dialog interface, reference management for CS
    #ifndef OBLIVION   
    _LOCAL void                 BuildSchoolList(HWND comboList);
    _LOCAL void                 InitializeDialog(HWND dialog);
    _LOCAL void                 SetInDialog(HWND dialog);
    _LOCAL void                 GetFromDialog(HWND dialog); 
    _LOCAL void                 CleanupDialog(HWND dialog);
    _LOCAL bool                 HandleDialogEvent(HWND dialog, int uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result); 
    #endif

    // methods: hooks & debugging
    _LOCAL static void          InitHooks();
    _LOCAL void                 Dump(); // dump to output

    // construction, destruction
    _LOCAL static EffectItem*   Create(const EffectSetting& effect);
    _LOCAL static EffectItem*   CreateCopy(const EffectItem& source);
    _LOCAL static EffectItem*   CreateCopyFromVanilla(::EffectItem* source, bool destroyOriginal); // for replacing existing ::EffectItems
    _LOCAL void                 Initialize(const EffectSetting& effect);
    _LOCAL                      EffectItem(const EffectSetting& effect);
    _LOCAL                      EffectItem(const EffectItem& source);
    _LOCAL void                 Destruct(); // indirect destructor, can be hooked and called from OBME destructor
    INLINE                      ~EffectItem() {Destruct();}
};

}   // end namespace OBME