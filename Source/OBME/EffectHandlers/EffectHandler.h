/*
    EffectHandlerBase is a common base providing a typecode and display name to compliment the polymorphic structure of magic effects.

    ActiveEffectHandler [Game only]
        compound class derived from ActiveEffect and EffectHandlerBase, used to attach additional information & virtual methods to ActiveEffects.
    
    EffectSettingHandler 
        container object attached to an EffectSetting to hold handler-specific parameters and provide a dialog interface in the CS.
        All effectsettings must have a valid handler field, but the field may be changed by scripts or in the CS

    EffectItemHandler
        container object attached to an EffectItem to hold handler-specific parameters and provide a dialog interface in the CS.
        May also provide a menu interface for editing in the Enchanting/Spellmakin/Alchemy menus in game.
        EffectItemHandler, like EffectItemExtra, must maintain a state mask indicating which fields are 'active' and which are unused.
        An effect item may not have a valid EffectItemHandler attached, which is the same as having one with all fields unused.
*/
#pragma once

// base classes
#include "API/TES/MemoryHeap.h"
#include "API/Magic/ActiveEffect.h"

// argument classes
class   TESFile;

namespace OBME {

// argument classes from OBME
class   EffectSetting;
class   EffectItem;

static const UInt32 EffectHandlerCode = 'VTCA';  // 'ACTV'

class EffectHandlerBase
{
public:

    // constructor, destructor
    EffectHandlerBase() {}
    virtual ~EffectHandlerBase() {}

    // handler code and name
    virtual UInt32              HandlerCode() const {return EffectHandlerCode;}
    virtual const char*         HandlerName() const; 

    // conversion
    static UInt32   GetDefaultHandlerCode(UInt32 mgefCode); // returns vanilla handler for vanilla effect codes, ACTV for new effects

    // handler enumeration
    static bool     GetNextHandler(UInt32& ehCode, const char*& ehName); // pass ehCode == 0 to get first handler

    // use FormHeap for class new & delete
    USEFORMHEAP    

    // initialization
    static void     Initialize();
};

class EffectSettingHandler : public EffectHandlerBase
{
public:
    // constructor, destructor
    EffectSettingHandler(EffectSetting& effect) : parentEffect(effect) {}  // initialize fields based on parent object
    virtual ~EffectSettingHandler() {}

    // creation 
    static EffectSettingHandler*    Create(UInt32 handlerCode, EffectSetting& effect); // create polymorphic instance based on handler code    
    static EffectSettingHandler*    Make(EffectSetting& effect) {return new EffectSettingHandler(effect);}

    // serialization
    virtual bool                LoadHandlerChunk(TESFile& file);
    virtual void                SaveHandlerChunk();
    virtual void                LinkHandler();

    // copy/compare
    virtual void                CopyFrom(const EffectSettingHandler& copyFrom);
    virtual bool                CompareTo(const EffectSettingHandler& compareTo); // returns false is equivalent

    // child Dialog for CS editing
    #ifndef OBLIVION
    virtual INT                 DialogTemplateID(); // ID of dialog template resource 
    virtual bool                DialogMessageCallback(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result);
    virtual void                SetInDialog(HWND dialog);
    virtual void                GetFromDialog(HWND dialog);
    virtual void                CleanupDialog(HWND dialog);
    #endif

protected:
    // parent object
    EffectSetting&      parentEffect;  
};

class EffectItemHandler : public EffectHandlerBase
{
public:
    // constructor, destructor
    EffectItemHandler(EffectItem& item) : parentItem(item) {}
    virtual ~EffectItemHandler() {}

    // creation
    static EffectItemHandler*   Create(UInt32 handlerCode, EffectItem& item); // create polymorphic instance based on handler code      
    static EffectItemHandler*   Make(EffectItem& item) {return new EffectItemHandler(item);}

    // serialization
    virtual bool                LoadHandlerChunk(TESFile& file) {return 0;}
    virtual bool                SaveHandlerChunk() {return 0;}
    virtual void                LinkHandler() {}

    // copy/compare
    virtual void                CopyFrom(const EffectItemHandler& copyFrom) {}
    virtual bool                CompareTo(const EffectItemHandler& compareTo) {return 0;} // returns false is equivalent

    // child Menu/Dialog for Game/CS editing
    #ifdef OBLIVION
    // TODO: menu interaction methods, similar to the dialog interaction methods
    #else
    virtual INT                 DialogTemplateID() {return 0;} // ID of dialog template resource 
    virtual bool                DialogMessageCallback(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result) {return 0;}
    virtual void                SetInDialog(HWND dialog) {}
    virtual void                GetFromDialog(HWND dialog) {}
    virtual void                CleanupDialog(HWND dialog) {}
    #endif

protected:
    // parent object
    EffectItem&     parentItem;
};

#ifdef OBLIVION

class ActiveEffectHandlerBase : public EffectHandlerBase
{
public:
    // ... Additional members and (pure) virtual methods common to all OBME active effects
    virtual bool                FullyLocalized();  // returns true if effect does not require external information to safely remove itself
};

class ActiveEffectHandler : public ActiveEffectHandlerBase, public ActiveEffect
{
public:    
    // constructor
    ActiveEffectHandler(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem);

    // creation
    static ActiveEffect*        Create(UInt32 handlerCode, MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem, TESBoundObject* boundObject); // 
                                // create polymorphic instance based on handler code      
    static ActiveEffectHandler* Make(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem);

    // Clone virtual function override (needs to be overriden by *all* OBME AE handlers)
    _LOCAL /*004*/ virtual ActiveEffect*    Clone() const;

    // use FormHeap for class new & delete
    USEFORMHEAP    
};

#endif

}   //  end namespace OBME