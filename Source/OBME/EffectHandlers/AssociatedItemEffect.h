/*
    TODO
*/
#pragma once

// base classes
#include "OBME/EffectHandlers/EffectHandler.h"
#include "API/Magic/AssociatedItemEffects.h"

namespace OBME {

class AssociatedItemEffect;  // OBME version

class AssociatedItemMgefHandler : public MgefHandler
{
public:
    // constructor
    AssociatedItemMgefHandler(EffectSetting& effect);

    // serialization
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
    virtual void                InitializeDialog(HWND dialog) = 0;
    virtual void                SetInDialog(HWND dialog);
    virtual void                GetFromDialog(HWND dialog);
    #endif 

    // no additional members
};

class AssociatedItemEfitHandler : public EfitHandler
{
public:
    // constructor, destructor
    AssociatedItemEfitHandler(EffectItem& item) : EfitHandler(item) {}

    // ... TODO
};

#ifdef OBLIVION

class AssociatedItemEffect : public ActvHandler, public ::AssociatedItemEffect
{
public:    
    // constructor
    AssociatedItemEffect(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem);

    // creation 
    static ::ActiveEffect*      Make(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem);

    // Clone virtual function override (needs to be overriden by *all* OBME ActiveEffects)
    _LOCAL /*004*/ virtual ::ActiveEffect*      Clone() const;

    // use FormHeap for class new & delete
    USEFORMHEAP    
};

#endif

}   //  end namespace OBME