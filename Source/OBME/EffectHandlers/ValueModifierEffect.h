/*
    TODO
*/
#pragma once

// base classes
#include "OBME/EffectHandlers/EffectHandler.h"
#include "API/Magic/ValueModifierEffects.h"

namespace OBME {

class ValueModifierEffect;  // OBME version

class ValueModifierMgefHandler : public MgefHandler
{
public:
    // constructor
    ValueModifierMgefHandler(EffectSetting& effect);

    // serialization
    static const UInt32         kEHNDChunkSize = 0x8;
    virtual bool                LoadHandlerChunk(TESFile& file, UInt32 RecordVersion);
    virtual void                SaveHandlerChunk();
    virtual void                LinkHandler();
    virtual void                UnlinkHandler();

    // copy/compare
    virtual void                CopyFrom(const MgefHandler& copyFrom);
    virtual bool                CompareTo(const MgefHandler& compareTo);

    #ifndef OBLIVION
    // reference management in CS
    virtual void                RemoveFormReference(TESForm& form);
    // child Dialog in CS
    virtual INT                 DialogTemplateID();
    virtual void                InitializeDialog(HWND dialog);
    virtual bool                DialogMessageCallback(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result);
    virtual void                SetInDialog(HWND dialog);
    virtual void                GetFromDialog(HWND dialog);
    #endif 

    // AV Parts
    enum AVParts
    {
        kAVPart_Max             = 0x00,
        kAVPart_Script          = 0x01,
        kAVPart_Damage          = 0x02,
        kAVPart_Base            = 0x03,
        kAVPart_MaxSpecial      = 0x06,
        kAVPart_CurVal          = 0x10,
        kAVPart_CalcBase        = 0x11,
    };

    // members
    bool            magIsPercentage;  // magnitude is a percentage of 
    UInt8           percentageAVPart;   // AVPart: part of AV to take percentage of
    UInt8           avPart; // AVPart: modifier to target
    float           detrimentalLimit; // detrimental effects won't decrease current av value below this limit ( < 0 indicates no limit)
};

template <UInt32 ehCode> class GenericValueModifierMgefHandler : public ValueModifierMgefHandler
{
public:
    // constructor
    GenericValueModifierMgefHandler(EffectSetting& effect) : ValueModifierMgefHandler(effect) {}
    // creation 
    static MgefHandler*         Make(EffectSetting& effect) {return new GenericValueModifierMgefHandler(effect);}  // create instance of this type
    // handler code
    virtual UInt32              HandlerCode() const {return ehCode;}
};

class ValueModifierEfitHandler : public EfitHandler
{
public:
    // constructor, initializer
    ValueModifierEfitHandler(EffectItem& item);
    virtual void                SetParentItemDefaultFields(); // initialize actorVal, script formid on parent effect item & incr refs accordingly

    // serialization
    //virtual bool              LoadHandlerChunk(TESFile& file, UInt32 RecordVersion);
    //virtual void              SaveHandlerChunk();
    virtual void                LinkHandler();
    virtual void                UnlinkHandler();

    // copy/compare
    virtual void                CopyFrom(const EfitHandler& copyFrom); // must incr/decr CrossRefs for actorVal, scriptFormID if necessary
    virtual bool                CompareTo(const EfitHandler& compareTo); // returns false if equivalent
    virtual bool                Match(const EfitHandler& compareTo); // returns true if they match for purposes of alchemy

    // game/CS specific
    #ifdef OBLIVION
    // TODO: menu interaction methods, similar to the dialog interaction methods
    #else
    // reference management in CS
    virtual void                RemoveFormReference(TESForm& form);
    // child dialog in CS
    virtual INT                 DialogTemplateID();
    virtual void                InitializeDialog(HWND dialog);
    virtual bool                DialogMessageCallback(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result);
    virtual void                SetInDialog(HWND dialog);
    virtual void                GetFromDialog(HWND dialog);
    #endif

    // methods
    UInt32          GetActorValue();
    UInt8           GetAVPart();
    bool            GetDetrimental();
    bool            GetRecoverable();

    // override flags
    enum OverrideFlags
    {   
        kOverride_ActorValue    = 0x01,
        kOverride_AVPart        = 0x02,
        kOverride_Detrimental   = 0x04,
        kOverride_Recovers      = 0x08,
    };

    // members
    UInt8               overrideFlags;
    UInt8               avPart; // AVPart: modifier to target
    bool                detrimental;
    bool                recovers;
};

template <UInt32 ehCode> class GenericValueModifierEfitHandler : public ValueModifierEfitHandler
{
public:
    // constructor
    GenericValueModifierEfitHandler(EffectItem& item) : ValueModifierEfitHandler(item) {}
    // creation 
    static EfitHandler*         Make(EffectItem& item) {return new GenericValueModifierEfitHandler(item);}  // create instance of this type
    // handler code
    virtual UInt32              HandlerCode() const {return ehCode;}
};


#ifdef OBLIVION

class ValueModifierActvHandler
{
public:
    // ... Additional members and (pure) virtual methods common to value modifier effects
    virtual float               GetDefaultMagnitude(); // magnitude for 'No Magnitude' effects
};

class ValueModifierEffect : public ValueModifierActvHandler, public ActvHandler, public ::ValueModifierEffect
{
public:    
    // constructor
    ValueModifierEffect(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem);

    // creation 
    static ::ActiveEffect*      Make(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem);

    // Clone virtual function override (needs to be overriden by *all* OBME ActiveEffects)
    _LOCAL /*004*/ virtual ::ActiveEffect*      Clone() const;

    // use FormHeap for class new & delete
    USEFORMHEAP    
};

#endif

}   //  end namespace OBME