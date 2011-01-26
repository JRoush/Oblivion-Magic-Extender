/*
    TODO
*/
#pragma once

// base classes
#include "OBME/EffectHandlers/EffectHandler.h"
#include "API/Magic/ActiveEffects.h"

namespace OBME {

class DispelEffect;  // OBME version

class DispelMgefHandler : public MgefHandler
{
public:
    // constructor
    DispelMgefHandler(EffectSetting& effect);
    // creation 
    static MgefHandler*         Make(EffectSetting& effect) {return new DispelMgefHandler(effect);}  // create instance of this type
    // handler code
    virtual UInt32              HandlerCode() const {return EffectHandler::kDSPL;}

    // serialization
    static const UInt32         kEHNDChunkSize = 0x14;
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

    // members
    UInt32      ehCode;
    UInt32      mgefCode;
    UInt16      magicTypes; // bits ordered as in Magic::MagicTypes, indicating allowable magic item types
    UInt16      castTypes; // bits ordered as in Magic::CastTypes, indicating allowable enchantment types
    UInt32      magicitemFormID; // magic item or leveled spell
    bool        atomicItems;
    bool        atomicDispel;
    bool        distributeMag;
    bool        useCastingCost;
};

class DispelEfitHandler : public EfitHandler
{
public:
    // constructor, destructor
    DispelEfitHandler(EffectItem& item) : EfitHandler(item) {}
    // creation 
    static EfitHandler*         Make(EffectItem& item) {return new DispelEfitHandler(item);}  // create instance of this type
    // handler code
    virtual UInt32              HandlerCode() const {return EffectHandler::kDSPL;}

    // ... TODO
};

#ifdef OBLIVION

class DispelEffect : public ActvHandler, public ::DispelEffect
{
public:    
    // constructor
    DispelEffect(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem);

    // creation 
    static ::ActiveEffect*      Make(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem);

    // Clone virtual function override (needs to be overriden by *all* OBME ActiveEffects)
    _LOCAL /*004*/ virtual ::ActiveEffect*      Clone() const;

    // use FormHeap for class new & delete
    USEFORMHEAP    
};

#endif

}   //  end namespace OBME