/*
    TODO
*/
#pragma once

// base classes
#include "OBME/EffectHandlers/AssociatedItemEffect.h"
#include "API/Magic/AssociatedItemEffects.h"

namespace OBME {

class SummonCreatureEffect;  // OBME version

class SummonCreatureMgefHandler : public AssociatedItemMgefHandler
{
public:
    // constructor
    SummonCreatureMgefHandler(EffectSetting& effect);
    // creation 
    static MgefHandler*         Make(EffectSetting& effect) {return new SummonCreatureMgefHandler(effect);}  // create instance of this type
    // handler code
    virtual UInt32              HandlerCode() const {return EffectHandler::kSMAC;}

    // child Dialog for CS editing
    #ifndef OBLIVION
    virtual void                InitializeDialog(HWND dialog);
    #endif 

    // no additional members
};

class SummonCreatureEfitHandler : public AssociatedItemEfitHandler
{
public:
    // constructor, destructor
    SummonCreatureEfitHandler(EffectItem& item) : AssociatedItemEfitHandler(item) {}
    // creation 
    static EfitHandler*         Make(EffectItem& item) {return new SummonCreatureEfitHandler(item);}  // create instance of this type
    // handler code
    virtual UInt32              HandlerCode() const {return EffectHandler::kSMAC;}

    // ... TODO
};

#ifdef OBLIVION

class SummonCreatureEffect : public ActvHandler, public ::SummonCreatureEffect
{
public:    
    // constructor
    SummonCreatureEffect(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem);

    // creation 
    static ::ActiveEffect*      Make(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem);

    // Clone virtual function override (needs to be overriden by *all* OBME ActiveEffects)
    _LOCAL /*004*/ virtual ::ActiveEffect*      Clone() const;

    // use FormHeap for class new & delete
    USEFORMHEAP    
};

#endif

}   //  end namespace OBME