#include "OBME/EffectItem.h"
#include "OBME/EffectSetting.h"
#include "OBME/EffectHandlers/EffectHandler.h"

#include "API/Actors/ActorValues.h"

namespace OBME {

// methods - other effect items
void EffectItem::CopyFrom(const EffectItem& copyFrom)
{
    // get parent form, for reference counting.  Note that parent ref is explicitly not copied.
    // NOTE - if parent is NULL, Add/Remove FormReference will do nothing
    TESForm* parent = parentList ? dynamic_cast<TESForm*>(parentList) : 0;

    // effect (automatically generates correct default handler item)
    #ifndef OBLIVION
    GetEffectSetting()->RemoveCrossReference(parent);
    copyFrom.GetEffectSetting()->AddCrossReference(parent);
    #endif
    SetEffectSetting(*copyFrom.GetEffectSetting());
    
    // basic fields
    SetRange(copyFrom.GetRange());
    SetProjectileRange(copyFrom.GetProjectileRange());
    _MESSAGE("Proj range %i -> %i",copyFrom.projectileRange,projectileRange);
    SetArea(copyFrom.GetArea());
    SetDuration(copyFrom.GetDuration());
    SetMagnitude(copyFrom.GetMagnitude());

    // handler & handler-specific parameters
    if (effectHandler && copyFrom.effectHandler) effectHandler->CopyFrom(*copyFrom.effectHandler);
    actorValue = copyFrom.actorValue;   // handler only copies it if handler uses it

    // overrides
        // TODO

    // CS dialog fields
    #ifndef OBLIVION
    filterMgef = copyFrom.filterMgef;   // don't track refs to filter effects; they're temporary and incomplete
    origBaseMagicka = copyFrom.origBaseMagicka;
    origItemMagicka = copyFrom.origItemMagicka;
    origItemMastery = copyFrom.origItemMastery;
    #endif

    // cost cache
    cost = -1;
}

// methods - basic parameters
void EffectItem::SetEffectSetting(const EffectSetting& nEffect)
{
    // set effect
    EffectSetting& mgef = const_cast<EffectSetting&>(nEffect);
    effect = &mgef;
    mgefCode = mgef.mgefCode;

    // validate range
    if (!SetRange(GetRange()) && !SetRange(Magic::kRange_Self) && !SetRange(Magic::kRange_Touch) && !SetRange(Magic::kRange_Target)) 
    {
        _ERROR("No valid range available for EffectItem using %s",mgef.GetDebugDescEx().c_str());
    }

    // reset handler
    if (effectHandler) 
    {
        effectHandler->UnlinkHandler(); // remove any references made by handler
        delete effectHandler;
    }
    effectHandler = EfitHandler::Create(mgef.GetHandler().HandlerCode(),*this);
    if (effectHandler) effectHandler->SetParentItemDefaultFields();
        // actorValue - initialized by Handler
        // script formid - initialized by handler
}
SInt32 EffectItem::GetProjectileRange() const
{
    if (!GetEffectSetting()->GetFlag(EffectSetting::kMgefFlagShift_HasProjRange)) return -1; // base effect cannot have projectile ranges
    else if (GetRange() != Magic::kRange_Target) return -1; // effect is not targeted, no projectile
    else return projectileRange;
}
bool EffectItem::SetProjectileRange(SInt32 value)
{
    bool noRange = !GetEffectSetting()->GetFlag(EffectSetting::kMgefFlagShift_HasProjRange) || (GetRange() != Magic::kRange_Target);
    if (noRange && value >= 0) return false;    // valid proj range not allowed for this effect
    
    projectileRange = value;
    cost = -1;  // clear cached cost
    return true;
}

// methods - overrides
EffectItem::EffectItemExtra::EffectItemExtra()
{
    scriptFormID = 0;
    school = Magic::kSchool_Alteration; // as in game/CS code - init to 0
    name.Clear();
    fxMgefCode = EffectSetting::kMgefCode_None;
    hostile = false;
    pad15[2] = pad15[1] = pad15[0] = 0; // unnecessary, but fixes a comparison bug in v20- obse
    mgefFlags = 0;
    mgefFlagMask = 0; 
    efitFlags = 0;
    efitFlagMask = 0;
    baseCost = 0;
    resistAV = ActorValues::kActorVal__UBOUND;   // used as a 'No resistance' value
    iconPath.Clear();
}
bool EffectItem::IsEfitFieldOverridden(UInt32 flagShift) const
{
    EffectItemExtra* extra = (EffectItemExtra*)scriptInfo;
    if (!extra || flagShift >= 16) return false;    // no overrides obj, or invalid flag
    return (extra->efitFlagMask & (1 << flagShift)) > 0;
}
void EffectItem::SetEfitFieldOverride(UInt32 flagShift, bool value)
{
    if (flagShift >= 16) return;  // invalid flag
    EffectItemExtra* extra = (EffectItemExtra*)scriptInfo;
    if (value)
    {
        if (!extra) extra = new EffectItemExtra;    // default construct extra obj
        scriptInfo = extra;
        extra->efitFlagMask |= (1 << flagShift);    // set override flag
    }
    else if (extra) 
    {
        extra->efitFlagMask &= ~(1 << flagShift);    // clear override flag
        if (extra->efitFlagMask == 0 && extra->mgefFlagMask == 0 && mgefCode != Swap32('SEFF')) // all overrides clear, and not SEFF
        {
            delete extra; // default destruct (cleans up name+icon strings)
            scriptInfo = 0;
        }
    }    
}
bool EffectItem::IsMgefFlagOverridden(UInt32 flagShift) const
{
    EffectItemExtra* extra = (EffectItemExtra*)scriptInfo;
    if (!extra || flagShift >= 64) return false;    // no overrides obj, or invalid flag
    return (extra->mgefFlagMask & (1i64 << flagShift)) > 0;
}
void EffectItem::SetMgefFlagOverride(UInt32 flagShift, bool value)
{
    if (flagShift >= 64) return;  // invalid flag
    EffectItemExtra* extra = (EffectItemExtra*)scriptInfo;
    if (value)
    {
        if (!extra) extra = new EffectItemExtra;    // default construct extra obj
        scriptInfo = extra;
        extra->mgefFlagMask |= (1i64 << flagShift); // set override flag
    }
    else if (extra) 
    {
        extra->mgefFlagMask &= ~(1i64 << flagShift);    // clear override flag
        if (extra->efitFlagMask == 0 && extra->mgefFlagMask == 0 && mgefCode != Swap32('SEFF')) // all overrides clear, and not SEFF
        {
            delete extra; // default destruct (cleans up name+icon strings)
            scriptInfo = 0;
        }
    }  
}
BSStringT EffectItem::GetEffectName() const
{
    if (IsEfitFieldOverridden(kEfitFlagShift_Name) && !GetEfitFlag(kEfitFlagShift_Name)) return scriptInfo->name; // return effect name override
    else return GetEffectSetting()->Name(); // return name of parent effect setting
}
void EffectItem::SetEffectName(const char* name)
{
    SetEfitFieldOverride(kEfitFlagShift_Name,true); // set override flag
    SetEfitFlag(kEfitFlagShift_Name,false); // clear 'whole descriptor override' flag
    scriptInfo->name = name; // set name
}
UInt32 EffectItem::GetSchool() const
{
    if (IsEfitFieldOverridden(kEfitFlagShift_School)) return scriptInfo->school; // return school override
    else return GetEffectSetting()->school; // return name of parent effect setting
}
void EffectItem::SetSchool(UInt32 schoolCode)
{
    SetEfitFieldOverride(kEfitFlagShift_School,true); // set override flag
    scriptInfo->school = schoolCode; // set name
}
EffectSetting* EffectItem::GetFXEffect() const
{
    if (IsEfitFieldOverridden(kEfitFlagShift_FXMgef)) return EffectSetting::LookupByCode(scriptInfo->fxMgefCode); // return fx effect override
    else return GetEffectSetting(); // return name of parent effect setting
}
void EffectItem::SetFXEffect(UInt32 mgefCode)
{
    SetEfitFieldOverride(kEfitFlagShift_FXMgef,true); // set override flag
    scriptInfo->fxMgefCode = mgefCode; // set name
}
UInt8 EffectItem::GetHostility() const
{
    return 0;
}
void EffectItem::SetHostility(UInt8 hostility)
{
}
bool EffectItem::GetMgefFlag(UInt32 flagShift) const
{
    if (flagShift >= 64) return false; // invalid flag - return default value of false
    EffectItemExtra* extra = (EffectItemExtra*)scriptInfo;
    if (extra && extra->mgefFlagMask & (1i64 << flagShift)) 
    {
        _MESSAGE("Flag masked, %i",extra->mgefFlags & (1i64 << flagShift));
        return (extra->mgefFlags & (1i64 << flagShift)) > 0; // return override flag
    }
    else return GetEffectSetting()->GetFlag(flagShift); // return flag value from base effect
}
void EffectItem::SetMgefFlag(UInt32 flagShift, bool value)
{
    if (flagShift >= 64) return;    // invalid flag
    SetMgefFlagOverride(flagShift,true);   // set override flag, constructs extra obj if necessary
    if (EffectItemExtra* extra = (EffectItemExtra*)scriptInfo)
    {
        if (value) extra->mgefFlags |= (1i64 << flagShift);    // set flag
        else extra->mgefFlags &= ~(1i64 << flagShift);         // clear flag
    }
}
bool EffectItem::GetEfitFlag(UInt32 flagShift) const
{
    EffectItemExtra* extra = (EffectItemExtra*)scriptInfo;
    if (!extra || flagShift >= 16) return false;    // no overrides obj, or invalid flag - return default of false
    return (extra->efitFlagMask & (1 << flagShift) & extra->efitFlags) > 0;
}
void EffectItem::SetEfitFlag(UInt32 flagShift, bool value)
{
    if (flagShift >= 16) return;    // invalid flag
    SetEfitFieldOverride(flagShift,true);   // set override flag, constructs extra obj if necessary
    if (EffectItemExtra* extra = (EffectItemExtra*)scriptInfo)
    {
        if (value) extra->efitFlags |= (1 << flagShift);    // set flag
        else extra->efitFlags &= ~(1 << flagShift);         // clear flag
    }
}
float EffectItem::GetBaseCost() const
{
    return 0;
}
void EffectItem::SetBaseCost(float nCost)
{
}
UInt32 EffectItem::GetResistAV() const
{
    return 0;
}
void EffectItem::SetResistAV(UInt32 avCode)
{
}
const char* EffectItem::GetEffectIcon() const
{
    return 0;
}
void EffectItem::SetEffectIcon(const char* iconPath)
{
}

// methods - magicka cost
float EffectItem::MagickaCost()
{
    return 0;
}
void EffectItem::SetMagickaCost(float nCost)
{
}
#ifdef OBLIVION
float EffectItem::MagickaCostForCaster(Actor* caster = 0)
{
    return 0;
}
#endif

// methods - construction,destruction
EffectItem* EffectItem::Create(const EffectSetting& effect) { return new EffectItem(effect); }
EffectItem* EffectItem::CreateCopy(const EffectItem& source)  { return new EffectItem(source); }
EffectItem* EffectItem::CreateCopyFromVanilla(::EffectItem* source, bool destroyOriginal)
{
    if (!source) return 0;
    EffectSetting* mgef = EffectSetting::LookupByCode(source->mgefCode); // lookup mgef, in case it has been rebuilt
    if (!mgef) return 0;

    // construct new item with proper effect
    EffectItem* item = new EffectItem(*mgef);

    // copy magnitude, area, range, duration
    item->SetMagnitude(source->GetMagnitude());
    item->SetArea(source->GetArea());
    item->SetDuration(source->GetDuration());
    item->SetRange(source->GetRange());

    // TODO - copy ActorVal
    // TODO - copy SCIT overrides

    // destroy original effect if requested
    if (destroyOriginal)
    {
        // reproduce original destructor from game/CS code
        if (source->scriptInfo) delete ((::EffectItem::ScriptEffectInfo*)source->scriptInfo);  // should automatically invoke destructor for name string
        MemoryHeap::FormHeapFree(source);   // deallocate vanilla effectitem without invoking destructor, since it has been hooked
    }

    return item;
}
void EffectItem::Initialize(const EffectSetting& nEffect)
{
    // effect
    EffectSetting& mgef = const_cast<EffectSetting&>(nEffect);
    effect = &mgef;
    mgefCode = nEffect.mgefCode;
    
    // basic fields
    if (!SetRange(Magic::kRange_Self) && !SetRange(Magic::kRange_Touch) && !SetRange(Magic::kRange_Target)) 
    {
        _ERROR("No valid range available for EffectItem using %s",nEffect.GetDebugDescEx().c_str());
    }
    area = 0;
    duration = 0;
    magnitude = 0;
    projectileRange = -1;   // default to inf range, for compatibility

    // overrides
    scriptInfo = 0;

    // cost cache
    cost = -1;

    // dialog editing fields
    #ifndef OBLIVION
    filterMgef = 0;
    origBaseMagicka = 0;
    origItemMagicka = 0;
    origItemMastery = 0;
    #endif

    // parent
    parentList = 0;
    
    // handler
    effectHandler = EfitHandler::Create(mgef.GetHandler().HandlerCode(),*this);
    if (effectHandler) effectHandler->SetParentItemDefaultFields();
        // actorValue - initialized by Handler
        // script formid - initialized by handler
}
EffectItem::EffectItem(const EffectSetting& effect) : ::EffectItem(effect) {}
EffectItem::EffectItem(const EffectItem& source) : ::EffectItem(source) {}
void EffectItem::Destruct()
{
    // EffectItemExtra
    if (EffectItemExtra* extra = (EffectItemExtra*)scriptInfo)
    {
        delete extra;   // default destructor will call BSStringT destructors for name & icon path
        scriptInfo = 0;
    }

    // Handler
    if (effectHandler) 
    {
        delete effectHandler;
        effectHandler = 0;
    }
}
// methods - debugging
void EffectItem::Dump()
{
    BSStringT desc = "[NO PARENT]";
    if (TESForm* form = dynamic_cast<TESForm*>(parentList)) form->GetDebugDescription(desc);
    _MESSAGE("EffectItem <%p> on %s using %s",this,desc.c_str(),GetEffectSetting()->GetDebugDescEx().c_str());
    gLog.Indent();
    _MESSAGE("Magnitude = %i (%i)",GetMagnitude(),magnitude);
    _MESSAGE("Duration = %i (%i)",GetDuration(),duration);
    _MESSAGE("Area = %i (%i)",GetArea(),area);
    _MESSAGE("Range = %i (%i)",GetRange(),range);
    _MESSAGE("Proj. Range = %i (%i)",GetProjectileRange(),projectileRange);
    if (EffectItemExtra* extra = (EffectItemExtra*)scriptInfo)
    {
        _MESSAGE("Overrides (Efit Mask = %08X, Mgef Mask = %08X %08X):",extra->efitFlagMask,UInt32(extra->mgefFlagMask >> 32), UInt32(extra->mgefFlagMask));
        gLog.Indent();
        _MESSAGE("Name = %s (%s)",GetEffectName().c_str(),extra->name.c_str());
        _MESSAGE("School = %i (%i)",GetSchool(),extra->school);
        _MESSAGE("FX Effect = %p (%p)",GetFXEffect() ? GetFXEffect()->mgefCode : 0,extra->fxMgefCode);
        _MESSAGE("EfitFlags = %08X",extra->efitFlags);
        _MESSAGE("MgefFlags = %08X %08X",UInt32(extra->mgefFlags >> 32), UInt32(extra->mgefFlags));
        gLog.Outdent();
    }
    else _MESSAGE("[No Overrides]");
    _MESSAGE("Handler <%p> = ...",effectHandler);
    _MESSAGE("Cost (No Caster) = %f",MagickaCost());
    gLog.Outdent();
}

}   //  end namespace OBME