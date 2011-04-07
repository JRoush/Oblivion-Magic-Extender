#include "OBME/EffectItem.h"
#include "OBME/EffectSetting.h"
#include "OBME/EffectHandlers/EffectHandler.h"
#include "OBME/Magic.h"

#include "API/Actors/ActorValues.h"
#include "Components/TESFileFormats.h"

namespace OBME {

// global method for returning small enumerations as booleans
inline bool BoolEx(UInt8 value) {return *(bool*)&value;}

// methods - other effect items
bool EffectItem::CompareTo(const EffectItem& compareTo) const
{
    _VMESSAGE("Comparing <%p> using %s to %p",this,GetEffectSetting()->GetDebugDescEx().c_str(),&compareTo);
    // NOTE - parent forms are explicity not compared

    // effect
    if (GetEffectSetting() != compareTo.GetEffectSetting()) return BoolEx(0x10);

    // range, projrange, area, duration, magnitude
    if (GetRange() != compareTo.GetRange()) return BoolEx(0x20);
    if (GetProjectileRange() != compareTo.GetProjectileRange()) return BoolEx(0x21);
    if (GetArea() != compareTo.GetArea()) return BoolEx(0x22);
    if (GetDuration() != compareTo.GetDuration()) return BoolEx(0x23);
    if (GetMagnitude() != compareTo.GetMagnitude()) return BoolEx(0x24);

    // handler & handler-specific fields
    if (effectHandler)
    {
        if (!compareTo.effectHandler) return BoolEx(0x30);
        else if (effectHandler->CompareTo(*compareTo.effectHandler)) return BoolEx(0x30);
    }
    else if (compareTo.effectHandler) return BoolEx(0x30);
    if (actorValue != compareTo.actorValue) return BoolEx(0x31);

    // overrides
    EffectItemExtra* extra = (EffectItemExtra*)scriptInfo;
    EffectItemExtra* compareExtra = (EffectItemExtra*)compareTo.scriptInfo;    
    if (extra)
    {
        // compare override flags
        if (!compareExtra) return BoolEx(0x40);
        if (extra->mgefFlagMask != compareExtra->mgefFlagMask) return BoolEx(0x41); 
        if (extra->efitFlagMask != compareExtra->efitFlagMask) return BoolEx(0x42); 
        // compare overridden fields
        if (IsEfitFieldOverridden(kEfitFlagShift_ScriptFormID) && extra->scriptFormID != compareExtra->scriptFormID) return BoolEx(0x50);   // script
        if (IsEfitFieldOverridden(kEfitFlagShift_School) && extra->school != compareExtra->school)                  return BoolEx(0x51);    // school
        if (IsEfitFieldOverridden(kEfitFlagShift_Name) && strcmp(extra->name.c_str(),compareExtra->name.c_str()) != 0) return BoolEx(0x52); // name
        if (IsEfitFieldOverridden(kEfitFlagShift_FXMgef) && extra->fxMgefCode != compareExtra->fxMgefCode)          return BoolEx(0x53);    // fx effect
        if (IsMgefFlagOverridden(EffectSetting::kMgefFlagShift_Hostile) && extra->hostile != compareExtra->hostile) return BoolEx(0x54);    // hostile flag
        if ((extra->mgefFlagMask & extra->mgefFlags) != (compareExtra->mgefFlagMask & compareExtra->mgefFlags))     return BoolEx(0x55);    // mgef flags
        if ((extra->efitFlagMask & extra->efitFlags) != (compareExtra->efitFlagMask & compareExtra->efitFlags))     return BoolEx(0x56);    // efit flags
        if (IsEfitFieldOverridden(kEfitFlagShift_Cost) && extra->baseCost != compareExtra->baseCost)                return BoolEx(0x57);    // cost
        if (IsEfitFieldOverridden(kEfitFlagShift_ResistAV) && extra->resistAV != compareExtra->resistAV)            return BoolEx(0x58);    // resistance
        if (IsEfitFieldOverridden(kEfitFlagShift_Icon) && strcmp(extra->iconPath.c_str(),compareExtra->iconPath.c_str()) != 0) return BoolEx(0x59); // icon
    }
    else if (compareExtra) return BoolEx(0x40);
   
    // effect items are equivalent
    return false;
}
void EffectItem::CopyFrom(const EffectItem& copyFrom)
{
    _VMESSAGE("Copying <%p> using %s to %p",&copyFrom,copyFrom.GetEffectSetting()->GetDebugDescEx().c_str(),this);

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
    range = copyFrom.range;
    projectileRange = copyFrom.projectileRange;
    area = copyFrom.area;
    duration = copyFrom.duration;
    magnitude = copyFrom.magnitude;

    // handler & handler-specific parameters
    if (effectHandler && copyFrom.effectHandler) effectHandler->CopyFrom(*copyFrom.effectHandler);
    actorValue = copyFrom.actorValue;   // handler only copies it if handler uses it

    // overrides
    EffectItemExtra* extra = (EffectItemExtra*)scriptInfo;
    EffectItemExtra* copyExtra = (EffectItemExtra*)copyFrom.scriptInfo;    
    #ifndef OBLIVION // remove current form refs in overrides
    if (IsEfitFieldOverridden(kEfitFlagShift_School)) // School
    { 
        /* TODO - decr school form ref */ 
    }
    if (IsEfitFieldOverridden(kEfitFlagShift_FXMgef)) // FX EffectSetting
    { 
        if (EffectSetting* mgef = GetFXEffect()) mgef->RemoveCrossReference(parent);
    }
    if (IsEfitFieldOverridden(kEfitFlagShift_ResistAV)) // Resistance AV
    {
        if (TESForm* form = TESFileFormats::GetFormFromCode(GetResistAV(),TESFileFormats::kResType_ActorValue)) form->RemoveCrossReference(parent);
    }
    #endif
    if (copyExtra)
    {
        // create extra obj if necessary
        if (!extra) extra = new EffectItemExtra;
        scriptInfo = extra;
        // copy all fields
        extra->scriptFormID = copyExtra->scriptFormID;    // handler only copies it if handler uses it
        extra->school = copyExtra->school;
        extra->name = copyExtra->name;
        extra->fxMgefCode = copyExtra->fxMgefCode;
        extra->hostile = copyExtra->hostile;
        extra->mgefFlags = copyExtra->mgefFlags;
        extra->mgefFlagMask = copyExtra->mgefFlagMask;
        extra->efitFlags = copyExtra->efitFlags;
        extra->efitFlagMask = copyExtra->efitFlagMask;
        extra->baseCost = copyExtra->baseCost;
        extra->resistAV = copyExtra->resistAV;
        extra->iconPath = copyExtra->iconPath;   
        // increment form refs in overrides
        #ifndef OBLIVION 
        if (IsEfitFieldOverridden(kEfitFlagShift_School)) // School
        { 
            /* TODO - decr school form ref */ 
        }
        if (IsEfitFieldOverridden(kEfitFlagShift_FXMgef)) // FX EffectSetting
        { 
            if (EffectSetting* mgef = GetFXEffect()) mgef->AddCrossReference(parent);
        }
        if (IsEfitFieldOverridden(kEfitFlagShift_ResistAV)) // Resistance AV
        {
            if (TESForm* form = TESFileFormats::GetFormFromCode(GetResistAV(),TESFileFormats::kResType_ActorValue)) form->AddCrossReference(parent);
        }
        #endif
    }
    else if (extra)
    {
        // destroy extra obj, no longer needed
        delete extra;
        scriptInfo = 0;
    }

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
    if (GetEffectSetting() == &nEffect) return; // this item already has the specified effect setting

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
    actorValue = 0; // handler may provide alternate init
    if (scriptInfo) scriptInfo->scriptFormID = 0;  // handler may provide alternate init
    effectHandler = EfitHandler::Create(mgef.GetHandler().HandlerCode(),*this);
    if (effectHandler) effectHandler->SetParentItemDefaultFields();
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
    scriptInfo->name = name ? name : ""; // set name
}
UInt32 EffectItem::GetSchool() const
{
    if (IsEfitFieldOverridden(kEfitFlagShift_School)) return scriptInfo->school; // return school override
    else return GetEffectSetting()->school; // return school of parent effect setting
}
void EffectItem::SetSchool(UInt32 schoolCode)
{
    SetEfitFieldOverride(kEfitFlagShift_School,true); // set override flag
    scriptInfo->school = schoolCode; // set school
}
EffectSetting* EffectItem::GetFXEffect() const
{
    if (IsEfitFieldOverridden(kEfitFlagShift_FXMgef)) return EffectSetting::LookupByCode(scriptInfo->fxMgefCode); // return fx effect override
    else return GetEffectSetting(); // return code of parent effect setting
}
void EffectItem::SetFXEffect(UInt32 mgefCode)
{
    SetEfitFieldOverride(kEfitFlagShift_FXMgef,true); // set override flag
    scriptInfo->fxMgefCode = mgefCode; // set fx mgefCode
}
UInt8 EffectItem::GetHostility() const
{
    if (GetMgefFlag(EffectSetting::kMgefFlagShift_Hostile)) return Magic::kHostility_Hostile;
    else if (GetMgefFlag(EffectSetting::kMgefFlagShift_Beneficial)) return Magic::kHostility_Beneficial;
    else return Magic::kHostility_Neutral;
}
void EffectItem::SetHostility(UInt8 hostility)
{
    switch(hostility)
    {
    case Magic::kHostility_Hostile:
        SetMgefFlag(EffectSetting::kMgefFlagShift_Hostile,true);
        SetMgefFlag(EffectSetting::kMgefFlagShift_Beneficial,false);
        break;
    case Magic::kHostility_Beneficial:
        SetMgefFlag(EffectSetting::kMgefFlagShift_Hostile,false);
        SetMgefFlag(EffectSetting::kMgefFlagShift_Beneficial,true);
        break;
    default:
        SetMgefFlag(EffectSetting::kMgefFlagShift_Hostile,false);
        SetMgefFlag(EffectSetting::kMgefFlagShift_Beneficial,false);
        break;
    }
}
bool EffectItem::GetMgefFlag(UInt32 flagShift) const
{
    if (flagShift >= 64) return false; // invalid flag - return default value of false
    EffectItemExtra* extra = (EffectItemExtra*)scriptInfo;
    if (extra && extra->mgefFlagMask & (1i64 << flagShift)) 
    {
        if (flagShift == EffectSetting::kMgefFlagShift_Hostile) return extra->hostile;  // hostile flag stored seperately
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
        if (flagShift == EffectSetting::kMgefFlagShift_Hostile) extra->hostile = value; // hostile flag stored seperately
        else if (value) extra->mgefFlags |= (1i64 << flagShift);    // set flag
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
    if (IsEfitFieldOverridden(kEfitFlagShift_Cost) && !GetEfitFlag(kEfitFlagShift_Cost)) 
        return ((EffectItemExtra*)scriptInfo)->baseCost; // return base cost override
    else return GetEffectSetting()->baseCost; // return base cost of parent effect setting
}
void EffectItem::SetBaseCost(float nCost)
{
    SetEfitFieldOverride(kEfitFlagShift_Cost,true); // set override flag
    SetEfitFlag(kEfitFlagShift_Cost,false); // clear 'no auto cost calc' flag
    ((EffectItemExtra*)scriptInfo)->baseCost = nCost; // set base cost
    cost = -1;  // clear cached cost
}
UInt32 EffectItem::GetResistAV() const
{
    if (IsEfitFieldOverridden(kEfitFlagShift_ResistAV)) return ((EffectItemExtra*)scriptInfo)->resistAV; // return resistance override
    else return GetEffectSetting()->GetResistAV(); // return resistance of parent effect setting
}
void EffectItem::SetResistAV(UInt32 avCode)
{
    SetEfitFieldOverride(kEfitFlagShift_ResistAV,true); // set override flag
    ((EffectItemExtra*)scriptInfo)->resistAV = avCode; // set resistance
}
const char* EffectItem::GetEffectIcon() const
{
    if (IsEfitFieldOverridden(kEfitFlagShift_Icon)) return ((EffectItemExtra*)scriptInfo)->iconPath; // return effect name override
    else return GetEffectSetting()->texturePath; // return icon path of parent effect setting
}
void EffectItem::SetEffectIcon(const char* iconPath)
{
    SetEfitFieldOverride(kEfitFlagShift_Icon,true); // set override flag
    ((EffectItemExtra*)scriptInfo)->iconPath = iconPath ? iconPath : ""; // set icon path
}

// methods - calculated properties
BSStringT EffectItem::GetDisplayText() const
{
    if (IsEfitFieldOverridden(kEfitFlagShift_Name) && GetEfitFlag(kEfitFlagShift_Name)) return scriptInfo->name; // return effect descriptor override
    
    // TODO - reproduce full descriptor construction
    return "[DESCRIPTOR - TODO]";
}
void  EffectItem::SetDisplayText(const char* text)
{
    SetEfitFieldOverride(kEfitFlagShift_Name,true); // set override flag
    SetEfitFlag(kEfitFlagShift_Name,true); // set 'whole descriptor override' flag
    scriptInfo->name = text ? text : ""; // set name
}
float EffectItem::MagickaCost()
{ 
    return MagickaCostForCaster(0); // get no-caster cost
}
float EffectItem::MagickaCostForCaster(Actor* caster)
{
    float skillFactor = 1.0;    
    if (caster)
    {
        // todo - compute skill factor for valid caster args
    }
    
    // use cached cost if valid
    if (cost >= 0) return cost * skillFactor;
    
    // check override field
    if (IsEfitFieldOverridden(kEfitFlagShift_Cost) && GetEfitFlag(kEfitFlagShift_Cost)) 
    {
        cost = ((EffectItemExtra*)scriptInfo)->baseCost; // use whole cost override
        return cost* skillFactor;
    }

    // TODO - compute base field factors, run through mgef cost callback if present, then multiply together
    cost = 5;
   
    return cost * skillFactor;
}
void EffectItem::SetMagickaCost(float nCost)
{
    SetEfitFieldOverride(kEfitFlagShift_Cost,true); // set override flag
    SetEfitFlag(kEfitFlagShift_Cost,true); // set 'no auto cost calc' flag
    ((EffectItemExtra*)scriptInfo)->baseCost = nCost; // set base cost
    cost = -1;  // clear cached cost
}
// methods - serialization
void EffectItem::Link()
{
    // get parent form, for reference counting.
    // NOTE - if parent is NULL, Add/Remove FormReference will do nothing
    TESForm* parent = parentList ? dynamic_cast<TESForm*>(parentList) : 0;

    // effect
    #ifndef OBLIVION
    GetEffectSetting()->AddCrossReference(parent);
    #endif

    // handler
    if (effectHandler) effectHandler->LinkHandler();    // links actorValue, ScriptFormID as necessary

    // override form refs
    #ifndef OBLIVION 
    if (IsEfitFieldOverridden(kEfitFlagShift_School)) // School
    { 
        /* TODO - decr school form ref */ 
    }
    if (IsEfitFieldOverridden(kEfitFlagShift_FXMgef)) // FX EffectSetting
    { 
        if (EffectSetting* mgef = GetFXEffect()) mgef->AddCrossReference(parent);
    }
    if (IsEfitFieldOverridden(kEfitFlagShift_ResistAV)) // Resistance AV
    {
        if (TESForm* form = TESFileFormats::GetFormFromCode(GetResistAV(),TESFileFormats::kResType_ActorValue)) form->AddCrossReference(parent);
    }
    #endif
}
void EffectItem::Unlink()
{
    // get parent form, for reference counting.
    // NOTE - if parent is NULL, Add/Remove FormReference will do nothing
    TESForm* parent = parentList ? dynamic_cast<TESForm*>(parentList) : 0;

    // effect
    #ifndef OBLIVION
    GetEffectSetting()->RemoveCrossReference(parent);
    #endif

    // handler
    if (effectHandler) effectHandler->UnlinkHandler();    // links actorValue, ScriptFormID as necessary

    // override form refs
    #ifndef OBLIVION 
    if (IsEfitFieldOverridden(kEfitFlagShift_School)) // School
    { 
        /* TODO - decr school form ref */ 
    }
    if (IsEfitFieldOverridden(kEfitFlagShift_FXMgef)) // FX EffectSetting
    { 
        if (EffectSetting* mgef = GetFXEffect()) mgef->RemoveCrossReference(parent);
    }
    if (IsEfitFieldOverridden(kEfitFlagShift_ResistAV)) // Resistance AV
    {
        if (TESForm* form = TESFileFormats::GetFormFromCode(GetResistAV(),TESFileFormats::kResType_ActorValue)) form->RemoveCrossReference(parent);
    }
    #endif
}
void EffectItem::ReplaceMgefCodeRef(UInt32 oldMgefCode, UInt32 newMgefCode)
{
    // effect
    if (mgefCode == oldMgefCode) mgefCode = newMgefCode;

    // override form refs
    if (IsEfitFieldOverridden(kEfitFlagShift_FXMgef)) // FX EffectSetting
    { 
        if (GetFXEffect() && GetFXEffect()->mgefCode == oldMgefCode) SetFXEffect(newMgefCode);
    }
}
#ifndef OBLIVION
void EffectItem::RemoveFormReference(TESForm& form)
{
    // NOTE - can't remove main EffectSetting, this must be done in the corresponding EffectItemList method
    
    // handler
    if (effectHandler) effectHandler->RemoveFormReference(form);

    // override form refs
    if (IsEfitFieldOverridden(kEfitFlagShift_School)) // School
    { 
        /* TODO - clear school form ref if found */ 
    }
    if (IsEfitFieldOverridden(kEfitFlagShift_FXMgef)) // FX EffectSetting
    { 
        if (&form == GetFXEffect()) SetEfitFieldOverride(kEfitFlagShift_FXMgef,false);
    }
    if (IsEfitFieldOverridden(kEfitFlagShift_ResistAV)) // Resistance AV
    {
        if (&form == TESFileFormats::GetFormFromCode(GetResistAV(),TESFileFormats::kResType_ActorValue)) SetEfitFieldOverride(kEfitFlagShift_ResistAV,false);
    }
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

    // overrides - none
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
    actorValue = 0; // handler may provide alternate initialization
    effectHandler = EfitHandler::Create(mgef.GetHandler().HandlerCode(),*this);
    if (effectHandler) effectHandler->SetParentItemDefaultFields();
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
        _MESSAGE("Hostility = %i (h=%i,b=%i)",GetHostility(), extra->hostile, (extra->mgefFlags & (1i64 << EffectSetting::kMgefFlagShift_Beneficial)) >0);
        _MESSAGE("EfitFlags = %08X",extra->efitFlags);
        _MESSAGE("MgefFlags = %08X %08X",UInt32(extra->mgefFlags >> 32), UInt32(extra->mgefFlags));
        _MESSAGE("Base Cost = %f (%f)",GetBaseCost(), extra->baseCost);
        _MESSAGE("Resistance = %02X (%02X)",GetResistAV(), extra->resistAV);
        _MESSAGE("Icon = %s (%s)",GetEffectIcon(), extra->iconPath.c_str());
        gLog.Outdent();
    }
    else _MESSAGE("[No Overrides]");
    _MESSAGE("Handler <%p> = ...",effectHandler);
    _MESSAGE("Cost (No Caster) = %f",MagickaCost());
    gLog.Outdent();
}

}   //  end namespace OBME