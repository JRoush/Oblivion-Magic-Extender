#include "OBME/EffectItem.h"
#include "OBME/EffectSetting.h"
#include "OBME/EffectHandlers/EffectHandler.h"
#include "OBME/EffectItemList.h"
#include "OBME/Magic.h"

#include "API/Actors/ActorValues.h"
#include "Components/TESFileFormats.h"
#include "Components/FormRefCounter.h"

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
    if (GetHandler())
    {
        if (!compareTo.GetHandler()) return BoolEx(0x30);
        else if (GetHandler()->CompareTo(*compareTo.GetHandler())) return BoolEx(0x30);
    }
    else if (compareTo.GetHandler()) return BoolEx(0x30);
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
    // get parent form, for reference counting.  Note that parent ref is explicitly not copied.
    // NOTE - if parent is NULL, Add/Remove FormReference will do nothing
    TESForm* parent = dynamic_cast<TESForm*>(GetParentList());
    _VMESSAGE("Copying <%p> on {%p} from <%p>",this,parent ? parent->formID : 0,&copyFrom);

    // effect (automatically generates correct default handler item)
    FormRefCounter::RemoveReference(parent,GetEffectSetting());
    FormRefCounter::AddReference(parent,copyFrom.GetEffectSetting());
    if (GetEffectSetting() != copyFrom.GetEffectSetting())
    {        
        if (GetHandler()) 
        {
            GetHandler()->UnlinkHandler(); // unlink handler to decr form refs
            SetHandler(0);  // destroy current handler
        }
        SetEffectSetting(*copyFrom.GetEffectSetting());
    }
    
    // basic fields
    range = copyFrom.range;
    area = copyFrom.area;
    duration = copyFrom.duration;
    magnitude = copyFrom.magnitude;
    SetProjectileRange(copyFrom.GetProjectileRange());  // don't copy literal values b/c it's stored on the extra obj

    // handler & handler-specific parameters
    if (GetHandler() && copyFrom.GetHandler()) GetHandler()->CopyFrom(*copyFrom.GetHandler());
    actorValue = copyFrom.actorValue;   // handler only copies it if handler uses it

    // overrides
    EffectItemExtra* extra = (EffectItemExtra*)scriptInfo;
    EffectItemExtra* copyExtra = (EffectItemExtra*)copyFrom.scriptInfo;    
    if (IsEfitFieldOverridden(kEfitFlagShift_School))   // School override form ref
    { 
        /* TODO - decr school form ref */ 
    }
    if (IsEfitFieldOverridden(kEfitFlagShift_FXMgef))   // FX EffectSetting override form ref
    { 
        if (EffectSetting* mgef = GetFXEffect()) FormRefCounter::RemoveReference(parent,mgef);
    }
    if (IsEfitFieldOverridden(kEfitFlagShift_ResistAV)) // Resistance AV override form ref
    {
        if (TESForm* form = TESFileFormats::GetFormFromCode(GetResistAV(),TESFileFormats::kResType_ActorValue)) 
            FormRefCounter::RemoveReference(parent,form);
    }
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
        if (IsEfitFieldOverridden(kEfitFlagShift_School)) // School
        { 
            /* TODO - decr school form ref */ 
        }
        if (IsEfitFieldOverridden(kEfitFlagShift_FXMgef)) // FX EffectSetting
        { 
            if (EffectSetting* mgef = GetFXEffect()) FormRefCounter::AddReference(parent,mgef);
        }
        if (IsEfitFieldOverridden(kEfitFlagShift_ResistAV)) // Resistance AV
        {
            if (TESForm* form = TESFileFormats::GetFormFromCode(GetResistAV(),TESFileFormats::kResType_ActorValue)) 
                FormRefCounter::AddReference(parent,form);
        }
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

    // handler - actorValue param
    if (mgef.GetFlag(EffectSetting::kMgefFlagShift_UseSkill)) actorValue = ActorValues::kActorVal_Armorer;  
    else if (mgef.GetFlag(EffectSetting::kMgefFlagShift_UseAttribute)) actorValue = ActorValues::kActorVal_Strength;
    else actorValue = 0;
    // handler - scriptFormID param
    SetEfitFieldOverride(kEfitFlagShift_ScriptFormID,false);
    // handler - efithandler
    SetHandler(EfitHandler::Create(mgef.GetHandler().HandlerCode(),*this));
}
EffectItemList* EffectItem::GetParentList() const
{
    if (EffectItemExtra* extra = (EffectItemExtra*)scriptInfo) return extra->parentList;
    else return 0;
}
void EffectItem::SetParentList(EffectItemList* list)
{
    EffectItemExtra* extra = (EffectItemExtra*)scriptInfo;
    if (list)
    {
        // default construct extra obj
        if (!extra) extra = new EffectItemExtra;    
        scriptInfo = extra;
        extra->parentList = list; // set new parent
    }
    else if (extra)
    {
        extra->parentList = 0; // clear parent list
        if (extra->IsEmpty() && mgefCode != Swap32('SEFF')) // all overrides clear, and not SEFF
        {
            delete extra; // destroy unnecessary handler object
            scriptInfo = 0;
        }
    }
}
TESForm* EffectItem::GetParentForm() const
{
    EffectItemList* list = GetParentList();
    if (list) return list->GetParentForm();
    else return 0;
}
EfitHandler* EffectItem::GetHandler() const
{
    if (EffectItemExtra* extra = (EffectItemExtra*)scriptInfo) return extra->effectHandler;
    else return 0;
}
void EffectItem::SetHandler(EfitHandler* handler)
{
    EffectItemExtra* extra = (EffectItemExtra*)scriptInfo;
    if (handler)
    {
        // default construct extra obj
        if (!extra) extra = new EffectItemExtra;    
        scriptInfo = extra;
        // detsroy old handler
        if (extra->effectHandler && extra->effectHandler != handler) delete extra->effectHandler;
        extra->effectHandler = handler; // set new handler
    }
    else if (extra)
    {
        if (extra->effectHandler) delete extra->effectHandler;  // destroy existing handler
        extra->effectHandler = 0; // clear handler field
        if (extra->IsEmpty() && mgefCode != Swap32('SEFF')) // all overrides clear, and not SEFF
        {
            delete extra; // destroy unnecessary handler object
            scriptInfo = 0;
        }
    }
}
SInt32 EffectItem::GetProjectileRange() const
{
    if (!GetEffectSetting()->GetFlag(EffectSetting::kMgefFlagShift_HasProjRange)) return -1; // base effect cannot have projectile ranges
    else if (GetRange() != Magic::kRange_Target) return -1; // effect is not targeted, no projectile
    else if (EffectItemExtra* extra = (EffectItemExtra*)scriptInfo) return extra->projectileRange;  // return stored projectile range
    else return -1; // no extra obj
}
bool EffectItem::SetProjectileRange(SInt32 value)
{
    EffectItemExtra* extra = (EffectItemExtra*)scriptInfo;
    if (value >= 0)
    {
        // check if this item can have a valid proj range
        if (!GetEffectSetting()->GetFlag(EffectSetting::kMgefFlagShift_HasProjRange) || GetRange() != Magic::kRange_Target) return false;
        // default construct extra obj
        if (!extra) extra = new EffectItemExtra;    
        scriptInfo = extra;
        // set new proj range
        extra->projectileRange = value;
    }
    else if (extra)
    {
        extra->projectileRange = -1;    // clear projectile range
        if (extra->IsEmpty() && mgefCode != Swap32('SEFF')) // all overrides clear, and not SEFF
        {
            delete extra; // destroy unnecessary handler object
            scriptInfo = 0;
        }
    }
    cost = -1;  // clear cached cost
    return true;
}

// methods - overrides
EffectItem::EffectItemExtra::EffectItemExtra()
{
    // override members
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
    // non-override members
    parentList = 0;
    effectHandler = 0;
    projectileRange = -1;
}
EffectItem::EffectItemExtra::~EffectItemExtra()
{
    // automatically invokes destructor for name + icon strings
    if (effectHandler) delete effectHandler;    // destroy handler object
}
bool EffectItem::EffectItemExtra::IsEmpty()
{
    return (efitFlagMask == 0 && mgefFlagMask == 0 && parentList == 0 && effectHandler == 0 && projectileRange < 0);
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
        if (extra->IsEmpty() && mgefCode != Swap32('SEFF')) // all overrides clear, and not SEFF
        {
            delete extra; // destroy unnecessary handler object
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
        if (extra->IsEmpty() && mgefCode != Swap32('SEFF')) // all overrides clear, and not SEFF
        {
            delete extra; // destroy unnecessary handler object
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
    TESForm* parent = dynamic_cast<TESForm*>(GetParentList());

    // effect
    FormRefCounter::AddReference(parent,GetEffectSetting());

    // handler
    if (GetHandler()) GetHandler()->LinkHandler();    // links actorValue, ScriptFormID as necessary

    // override form refs
    if (IsEfitFieldOverridden(kEfitFlagShift_School)) // School
    { 
        /* TODO - decr school form ref */ 
    }
    if (IsEfitFieldOverridden(kEfitFlagShift_FXMgef)) // FX EffectSetting
    { 
        if (EffectSetting* mgef = GetFXEffect()) FormRefCounter::AddReference(parent,mgef);
    }
    if (IsEfitFieldOverridden(kEfitFlagShift_ResistAV)) // Resistance AV
    {
        if (TESForm* form = TESFileFormats::GetFormFromCode(GetResistAV(),TESFileFormats::kResType_ActorValue)) FormRefCounter::AddReference(parent,form);
    }
}
void EffectItem::Unlink()
{
    // get parent form, for reference counting.
    // NOTE - if parent is NULL, Add/Remove FormReference will do nothing
    TESForm* parent = dynamic_cast<TESForm*>(GetParentList());

    // effect
    FormRefCounter::RemoveReference(parent,GetEffectSetting());

    // handler
    if (GetHandler()) GetHandler()->UnlinkHandler();    // links actorValue, ScriptFormID as necessary

    // override form refs
    if (IsEfitFieldOverridden(kEfitFlagShift_School)) // School
    { 
        /* TODO - decr school form ref */ 
    }
    if (IsEfitFieldOverridden(kEfitFlagShift_FXMgef)) // FX EffectSetting
    { 
        if (EffectSetting* mgef = GetFXEffect()) FormRefCounter::RemoveReference(parent,mgef);
    }
    if (IsEfitFieldOverridden(kEfitFlagShift_ResistAV)) // Resistance AV
    {
        if (TESForm* form = TESFileFormats::GetFormFromCode(GetResistAV(),TESFileFormats::kResType_ActorValue)) FormRefCounter::RemoveReference(parent,form);
    }
}
void EffectItem::ReplaceMgefCodeRef(UInt32 oldMgefCode, UInt32 newMgefCode)
{
    // effect
    if (mgefCode == oldMgefCode) mgefCode = newMgefCode;

    // handler
    if (GetHandler()) GetHandler()->ReplaceMgefCodeRef(oldMgefCode,newMgefCode);

    // override form refs
    if (IsEfitFieldOverridden(kEfitFlagShift_FXMgef)) // FX EffectSetting
    { 
        if (GetFXEffect() && GetFXEffect()->mgefCode == oldMgefCode) SetFXEffect(newMgefCode);
    }
}
void EffectItem::RemoveFormReference(TESForm& form)
{
    // NOTE - can't remove main EffectSetting, this must be done in the corresponding EffectItemList method
    
    // handler
    if (GetHandler()) GetHandler()->RemoveFormReference(form);

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

}   //  end namespace OBME