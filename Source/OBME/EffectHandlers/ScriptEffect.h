/*
    TODO
*/
#pragma once

// base classes
#include "OBME/EffectHandlers/EffectHandler.h"
#include "API/Magic/ActiveEffects.h"

namespace OBME {

class ScriptEffect;  // OBME version

class ScriptMgefHandler : public MgefHandler
{
public:
    // constructor
    ScriptMgefHandler(EffectSetting& effect);
    // creation 
    static MgefHandler*         Make(EffectSetting& effect) {return new ScriptMgefHandler(effect);}  // create instance of this type
    // handler code
    virtual UInt32              HandlerCode() const {return EffectHandler::kSEFF;}

    // serialization
    static const UInt32         kEHNDChunkSize = 0xC;
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
    UInt32          scriptFormID;   
    bool            useSEFFAlwaysApplies;  // magnitude is a percentage of 
    UInt8           efitAVResType;   // TESFileFormats::ResolutionTypes, for EffectItem::ActorValue field
    UInt8           customParamResType; // TESFileFormats::ResolutionTypes, for the custom param
    UInt32          customParam;
};

class ScriptEfitHandler : public EfitHandler
{
public:
    // constructor, destructor
    ScriptEfitHandler(EffectItem& item) : EfitHandler(item) {}
    // creation 
    static EfitHandler*         Make(EffectItem& item) {return new ScriptEfitHandler(item);}  // create instance of this type
    // handler code
    virtual UInt32              HandlerCode() const {return EffectHandler::kSEFF;}

    // ... TODO
};

#ifdef OBLIVION

class ScriptEffect : public ActvHandler, public ::ScriptEffect
{
public:    
    // constructor
    ScriptEffect(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem);

    // creation 
    static ::ActiveEffect*      Make(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem);

    // Clone virtual function override (needs to be overriden by *all* OBME ActiveEffects)
    _LOCAL /*004*/ virtual ::ActiveEffect*      Clone() const;

    // use FormHeap for class new & delete
    USEFORMHEAP    
};

#endif

}   //  end namespace OBME