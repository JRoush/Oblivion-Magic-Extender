/*
    TODO
*/
#pragma once

// base classes
#include "OBME/EffectHandlers/AssociatedItemEffect.h"
#include "API/Magic/AssociatedItemEffects.h"

namespace OBME {

class BoundItemEffect;  // OBME version

class BoundItemMgefHandler : public AssociatedItemMgefHandler
{
public:
    // constructor
    BoundItemMgefHandler(EffectSetting& effect);
    // creation 
    static MgefHandler*         Make(EffectSetting& effect) {return new BoundItemMgefHandler(effect);}  // create instance of this type
    // handler code
    virtual UInt32              HandlerCode() const {return EffectHandler::kSMBO;}

    // serialization
    virtual void                LinkHandler();  // overriden to allow correction of flag values
    #ifndef OBLIVION    
    // child Dialog for CS editing
    virtual INT                 DialogTemplateID();
    virtual void                InitializeDialog(HWND dialog);
    virtual bool                DialogMessageCallback(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result);
    virtual void                SetInDialog(HWND dialog);
    virtual void                GetFromDialog(HWND dialog);
    #endif 

    // no additional members
};

class BoundItemEfitHandler : public AssociatedItemEfitHandler
{
public:
    // constructor, destructor
    BoundItemEfitHandler(EffectItem& item) : AssociatedItemEfitHandler(item) {}
    // creation 
    static EfitHandler*         Make(EffectItem& item) {return new BoundItemEfitHandler(item);}  // create instance of this type
    // handler code
    virtual UInt32              HandlerCode() const {return EffectHandler::kSMBO;}

    // ... TODO
};

#ifdef OBLIVION

class BoundItemEffect : public ActvHandler, public ::BoundItemEffect
{
public:    
    // constructor
    BoundItemEffect(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem);

    // creation 
    static ::ActiveEffect*      Make(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem);

    // Clone virtual function override (needs to be overriden by *all* OBME ActiveEffects)
    _LOCAL /*004*/ virtual ::ActiveEffect*      Clone() const;

    // use FormHeap for class new & delete
    USEFORMHEAP    
};

#endif

}   //  end namespace OBME