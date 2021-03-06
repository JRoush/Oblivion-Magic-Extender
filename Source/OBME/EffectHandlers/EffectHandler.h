/*
    EffectHandler is a common base providing a typecode and display name to compliment the polymorphic structure of magic effects.

    OBME::ActiveEffect [Game only]
        compound class derived from vanilla ActiveEffect and EffectHandler, used to attach additional information & virtual methods to ActiveEffects.
    
    MgefHandler 
        container object attached to an EffectSetting to hold handler-specific parameters and provide a dialog interface in the CS.
        All effectsettings must have a valid handler field, but the field may be changed by scripts or in the CS

    EfitHandler
        container object attached to an EffectItem to hold handler-specific parameters and provide a dialog interface in the CS.
        May also provide a menu interface for editing in the Enchanting/Spellmakin/Alchemy menus in game.
        EfitHandler, like EffectItemExtra, must maintain a state mask indicating which fields are 'active' and which are unused.
        An effect item may not have a valid EfitHandler attached, which is the same as having one with all fields unused.
*/
#pragma once

// base classes
#include "API/TES/MemoryHeap.h"
#include "API/Magic/ActiveEffect.h"

// argument classes
class   TESFile;
class   TESForm;
class   Actor;

namespace OBME {

// argument classes from OBME
class   EffectSetting;
class   EffectItem;
class   ActiveEffect;

class EffectHandler
{
public:

    enum EffectHandlerCodes
    {
        k__NONE     = 0,
        kACTV       = 'VTCA',
        kSEFF       = 'FFES',
        kSUDG       = 'GDUS',
        kDEMO       = 'OMED',
        kCMND       = 'DNMC',
        kCOCR       = 'RCOC',
        kCOHU       = 'UHOC',
        kREAN       = 'NAER',
        kTURN       = 'NRUT',
        kVAMP       = 'PMAV',
        kLGHT       = 'THGL',
        kDSPL       = 'LPSD',
        kCURE       = 'ERUC',
        kDIAR       = 'RAID',
        kDIWE       = 'EWID',
        kLOCK       = 'KCOL',
        kOPEN       = 'NEPO',
        kSTRP       = 'PRTS',
        kMDAV       = 'VADM',
        kABSB       = 'BSBA',
        kPARA       = 'ARAP',
        kSHLD       = 'DLHS',
        kCALM       = 'MLAC',
        kFRNZ       = 'ZNRF',
        kCHML       = 'LMHC',
        kINVI       = 'IVNI',
        kDTCT       = 'TCTD',
        kNEYE       = 'EYEN',
        kDARK       = 'KRAD',
        kTELE       = 'ELET',
        kSUMN       = 'NMUS',
        kSMAC       = 'CAMS',
        kSMBO       = 'OBMS',
    };

    // constructor, destructor
    EffectHandler() {}
    virtual ~EffectHandler() {}

    // handler code and name
    virtual UInt32              HandlerCode() const = 0;
    virtual const char*         HandlerName() const; 

    // handler enumeration
    static bool     GetNextHandler(UInt32& ehCode, const char*& ehName); // pass ehCode == 0 to get first handler

    // use FormHeap for class new & delete
    USEFORMHEAP    

    // initialization
    static void     Initialize();
};

class MgefHandler : public EffectHandler
{
public:
    // constructor, destructor
    MgefHandler(EffectSetting& effect) : parentEffect(effect) {}  // initialize fields based on parent object.  
                                        // NOTE: Members initialized as linked, but refs not managed.  Do not initialize mgefParam.
    virtual ~MgefHandler() {} 

    // creation 
    static MgefHandler*    Create(UInt32 handlerCode, EffectSetting& effect); // create polymorphic instance based on handler code    

    // serialization
    virtual bool                LoadHandlerChunk(TESFile& file, UInt32 RecordVersion);
    virtual void                SaveHandlerChunk();
    virtual void                LinkHandler(); // must link mgefParam if necessary
    virtual void                UnlinkHandler(); // must unlink mgefParam if necessary

    // copy/compare
    virtual void                CopyFrom(const MgefHandler& copyFrom);  // must incr/decr CrossRefs for mgefParam if necessary
    virtual bool                CompareTo(const MgefHandler& compareTo); // returns false if equivalent
    
    // reference management
    virtual void                RemoveFormReference(TESForm& form); // removes ref to given form.  must handle mgefParam if necessary
    virtual void                ReplaceMgefCodeRef(UInt32 oldMgefCode, UInt32 newMgefCode); // replace all uses of old code with new code

    #ifndef OBLIVION
    // child Dialog in CS
    virtual INT                 DialogTemplateID(); // ID of dialog template resource 
    virtual void                InitializeDialog(HWND dialog);
    virtual bool                DialogMessageCallback(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result);
    virtual void                SetInDialog(HWND dialog);   // must set mgefParam and handler-specific mgefFlags if necessary
    virtual void                GetFromDialog(HWND dialog);   // must get mgefParam and handler-specific mgefFlags if necessary
    virtual void                CleanupDialog(HWND dialog);
    #endif

protected:
    // parent object
    EffectSetting&      parentEffect;  
};

template <UInt32 ehCode> class GenericMgefHandler : public MgefHandler
{
public:
    // constructor
    GenericMgefHandler(EffectSetting& effect) : MgefHandler(effect) {}
    // creation 
    static MgefHandler*         Make(EffectSetting& effect) {return new GenericMgefHandler(effect);}  // create instance of this type
    // handler code
    virtual UInt32              HandlerCode() const {return ehCode;}
};

class EfitHandler : public EffectHandler
{
public:
    // constructor, destructor
    EfitHandler(EffectItem& item) : parentItem(item) {}  // initialize fields based on parent object.  
                                        // NOTE: Members initialized as linked, but refs not managed.  Do not initialize actorVal/scriptFormID.
    virtual ~EfitHandler() {}

    // creation
    static EfitHandler*         Create(UInt32 handlerCode, EffectItem& item); // create polymorphic instance based on handler code. 

    // serialization
    virtual bool                LoadHandlerChunk(TESFile& file, UInt32 RecordVersion);
    virtual void                SaveHandlerChunk();
    virtual void                LinkHandler();   // must link actorValue, scriptFormID if necessary
    virtual void                UnlinkHandler();   // must unlink actorValue, scriptFormID if necessary

    // copy/compare
    virtual void                CopyFrom(const EfitHandler& copyFrom); // must incr/decr CrossRefs for actorVal, scriptFormID if necessary
    virtual bool                CompareTo(const EfitHandler& compareTo); // returns false if equivalent
    virtual bool                Match(const EfitHandler& compareTo); // returns true if they match for purposes of alchemy

    // calculated properties
    virtual float               GetCostFactor(Actor* caster); // get multiplicative factor to effect item magicka cost
    
    // reference management
    virtual void                RemoveFormReference(TESForm& form); // removes ref to given form.  must handle actorValue/scriptFormID if necessary
    virtual void                ReplaceMgefCodeRef(UInt32 oldMgefCode, UInt32 newMgefCode); // replace all uses of old code with new code

    // game/CS specific
    #ifdef OBLIVION
    // TODO: menu interaction methods, similar to the dialog interaction methods
    #else
    // child dialog in CS
    virtual INT                 DialogTemplateID(); // ID of dialog template resource 
    virtual void                InitializeDialog(HWND dialog);
    virtual bool                DialogMessageCallback(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result);
    virtual void                SetInDialog(HWND dialog);
    virtual void                GetFromDialog(HWND dialog);
    virtual void                CleanupDialog(HWND dialog);
    #endif

protected:
    // parent object
    EffectItem&     parentItem;
};

template <UInt32 ehCode> class NullEfitHandler : public EfitHandler
{
public:
    // constructor
    NullEfitHandler(EffectItem& item) : EfitHandler(item) {}
    // creation 
    static EfitHandler*         Make(EffectItem& item) {return 0;}  // generic Efit handler is none at all
    // handler code
    virtual UInt32              HandlerCode() const {return ehCode;}
};

#ifdef OBLIVION

class ActvHandler : public EffectHandler
{
public:
    // handler code
    virtual UInt32              HandlerCode() const {return 'VTCA';}
    // ... Additional members and (pure) virtual methods common to all OBME active effects
    virtual bool                FullyLocalized();  // returns true if effect does not require external information to safely remove itself
};

class ActiveEffect : public ActvHandler, public ::ActiveEffect
{
public:    
    // constructor
    ActiveEffect(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem);

    // creation
    static ::ActiveEffect*      Create(UInt32 handlerCode, MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem, TESBoundObject* boundObject); // 
                                // create polymorphic instance based on handler code      
    static ::ActiveEffect*      Make(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem);

    // Clone virtual function override (needs to be overriden by *all* OBME::ActiveEffects)
    _LOCAL /*004*/ virtual ::ActiveEffect*      Clone() const;

    // use FormHeap for class new & delete
    USEFORMHEAP    
};

#endif

}   //  end namespace OBME