/*
    TODO
*/
#pragma once

// base classes
#include "OBME/EffectHandlers/EffectHandler.h"
#include "API/Magic/ValueModifierEffects.h"

namespace OBME {

static const UInt32 ValueModifierEHCode = 'VADM';  // 'MDAV'

class ValueModifierESHandler : public EffectSettingHandler
{
public:
    // constructor
    ValueModifierESHandler(EffectSetting& effect) : EffectSettingHandler(effect) {}

    // creation 
    static ValueModifierESHandler*  Make(EffectSetting& effect) {return new ValueModifierESHandler(effect);}

    // handler code and name
    virtual UInt32              HandlerCode() const {return ValueModifierEHCode;}  

    // serialization
    virtual bool                LoadHandlerChunk(TESFile& file);
    virtual void                SaveHandlerChunk();
    virtual void                LinkHandler();

    // copy/compare
    virtual void                CopyFrom(const EffectSettingHandler& copyFrom);
    virtual bool                CompareTo(const EffectSettingHandler& compareTo);

    // child Dialog for CS editing
    #ifndef OBLIVION
    virtual INT                 DialogTemplateID();
    virtual bool                DialogMessageCallback(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result);
    virtual void                SetInDialog(HWND dialog);
    virtual void                GetFromDialog(HWND dialog);
    #endif 
};

class ValueModifierEIHandler : public EffectItemHandler
{
public:
    // constructor, destructor
    ValueModifierEIHandler(EffectItem& item) : EffectItemHandler(item) {}

    // creation    
    static ValueModifierEIHandler*   Make(EffectItem& item) {return new ValueModifierEIHandler(item);}

    // handler code and name
    virtual UInt32              HandlerCode() const {return ValueModifierEHCode;}  

};

#ifdef OBLIVION

class ValueModifierAEHandlerBase : public ActiveEffectHandlerBase
{
public:
    // ... Additional members and (pure) virtual methods common to value modifier effects
    virtual UInt8               GetAVModifier(); // AV part to modify
};

class ValueModifierEffectHandler : public ValueModifierAEHandlerBase, public ActiveEffectHandlerBase, public ValueModifierEffect
{
public:    
    // constructor
    ValueModifierEffectHandler(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem);

    // creation 
    static ActiveEffectHandler* Make(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem);

    // Clone virtual function override (needs to be overriden by *all* OBME AE handlers)
    _LOCAL /*004*/ virtual ActiveEffect*    Clone() const;

    // use FormHeap for class new & delete
    USEFORMHEAP    
};

#endif

}   //  end namespace OBME