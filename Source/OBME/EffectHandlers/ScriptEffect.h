/*
    TODO
*/
#pragma once

// base classes
#include "OBME/EffectHandlers/EffectHandler.h"
#include "API/Magic/ActiveEffects.h"

// argument classes
class   Script;

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

    // reference management
    virtual void                RemoveFormReference(TESForm& form);
    virtual void                ReplaceMgefCodeRef(UInt32 oldMgefCode, UInt32 newMgefCode);

    #ifndef OBLIVION
    // child Dialog in CS
    virtual INT                 DialogTemplateID();
    virtual void                InitializeDialog(HWND dialog);
    virtual bool                DialogMessageCallback(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result);
    virtual void                SetInDialog(HWND dialog);
    virtual void                GetFromDialog(HWND dialog);
    #endif 

    // members
    UInt32          scriptFormID;   
    bool            useSEFFAlwaysApplies;  // anded with flag on magic items
    UInt8           efitAVResType;      // TESFileFormats::ResolutionTypes, for EffectItem::ActorValue field - present for compatibility with v1
    UInt8           customParamResType; // TESFileFormats::ResolutionTypes, for the custom param
    UInt32          customParam;        // user-specified parameter
};

class ScriptEfitHandler : public EfitHandler
{
public:
    // constructor, destructor, creation
    ScriptEfitHandler(EffectItem& item);
    static EfitHandler*         Make(EffectItem& item) {return new ScriptEfitHandler(item);}  // create instance of this type

    // handler code
    virtual UInt32              HandlerCode() const {return EffectHandler::kSEFF;}

    // serialization
    //virtual bool                LoadHandlerChunk(TESFile& file, UInt32 RecordVersion);
    //virtual void                SaveHandlerChunk();
    virtual void                LinkHandler(); 
    virtual void                UnlinkHandler(); 

    // copy/compare
    virtual void                CopyFrom(const EfitHandler& copyFrom);
    virtual bool                CompareTo(const EfitHandler& compareTo);
    virtual bool                Match(const EfitHandler& compareTo);

    // reference management
    virtual void                RemoveFormReference(TESForm& form);
    virtual void                ReplaceMgefCodeRef(UInt32 oldMgefCode, UInt32 newMgefCode);

    // game/CS specific
    #ifndef OBLIVION
    // child dialog in CS
    virtual INT                 DialogTemplateID();
    virtual void                InitializeDialog(HWND dialog);
    virtual bool                DialogMessageCallback(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result);
    virtual void                SetInDialog(HWND dialog);
    virtual void                GetFromDialog(HWND dialog);
    #endif

    // methods
    Script*         GetScript();    // determine script from effect item & effect setting handlers

    // members
    UInt8           customParamResType; // TESFileFormats::ResolutionTypes, for the custom param
    UInt32          customParam;        // additional user-specified parameter
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