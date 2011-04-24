#include "OBME/EffectItem.h"
#include "OBME/EffectSetting.h"
#include "OBME/EffectHandlers/EffectHandler.h"
#include "OBME/EffectHandlers/ScriptEffect.h"
#include "OBME/Magic.h"

#include "API/Actors/ActorValues.h"
#include "Utilities/Memaddr.h"

// local declaration of module handle.
extern HMODULE hModule;

namespace OBME {

// methods - construction,destruction
EffectItem* EffectItem::Create(const EffectSetting& effect) { return new EffectItem(effect); }
EffectItem* EffectItem::CreateCopy(const EffectItem& source)  { return new EffectItem(source); }
EffectItem* EffectItem::CreateFromVanilla(::EffectItem* source, bool destroyOriginal)
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

    // copy ActorVal
    item->actorValue = source->actorValue;

    // copy SCIT overrides (this item scriptInfo* will already be initialized if necessary by constructor)
    if (source->scriptInfo && source->mgefCode == Swap32('SEFF'))
    {
        item->SetEfitFieldOverride(EffectItem::kEfitFlagShift_ScriptFormID,true);  
        item->scriptInfo->scriptFormID = source->scriptInfo->scriptFormID;
        item->SetEffectName(source->scriptInfo->name.c_str());
        item->SetSchool(source->scriptInfo->school);
        item->SetFXEffect(source->scriptInfo->fxMgefCode);
        item->SetHostility(source->scriptInfo->hostile ? Magic::kHostility_Hostile : Magic::kHostility_Neutral);
    }

    // destroy original effect if requested
    // There is no other way to destroy a vanilla EffectItem, since the vanilla destructor is patched by OBME.
    if (destroyOriginal)
    {
        // reproduce original destructor from game/CS code
        if (source->scriptInfo) delete ((::EffectItem::ScriptEffectInfo*)source->scriptInfo);  // should automatically invoke destructor for name string
        MemoryHeap::FormHeapFree(source);   // deallocate vanilla effectitem without invoking destructor, since it has been hooked
    }

    // done
    return item;
}
void EffectItem::Initialize(const EffectSetting& nEffect)
{
    // effect
    EffectSetting& mgef = const_cast<EffectSetting&>(nEffect);
    effect = &mgef;
    mgefCode = nEffect.mgefCode;

    // overrides - none
    scriptInfo = 0;
    
    // basic fields
    if (!SetRange(Magic::kRange_Self) && !SetRange(Magic::kRange_Touch) && !SetRange(Magic::kRange_Target)) 
    {
        _ERROR("No valid range available for EffectItem using %s",nEffect.GetDebugDescEx().c_str());
    }
    area = 0;
    duration = 0;
    magnitude = 0;

    // cost cache
    cost = -1;

    // dialog editing fields
    #ifndef OBLIVION
    filterMgef = 0;
    origBaseMagicka = 0;
    origItemMagicka = 0;
    origItemMastery = 0;
    #endif
    
    // handler - actorValue param
    if (mgef.GetFlag(EffectSetting::kMgefFlagShift_UseSkill)) actorValue = ActorValues::kActorVal_Armorer;  
    else if (mgef.GetFlag(EffectSetting::kMgefFlagShift_UseAttribute)) actorValue = ActorValues::kActorVal_Strength;
    else actorValue = 0;
    // handler - scriptFormID param
    if (mgef.mgefCode == Swap32('SEFF'))
    {
        // enable script,name,school,fxeffect,hostility overrides for SEFF effects
        // this maintains with non-OBME-aware scripts that create SEFF items and expect these fields to be enabled
        SetEfitFieldOverride(EffectItem::kEfitFlagShift_ScriptFormID,true);  
        scriptInfo->scriptFormID = 0;                   // default script override = 0
        SetEffectName(mgef.name.c_str());               // default name override = base effect name 
        SetSchool(Magic::kSchool_Alteration);           // default school code = 0    
        SetFXEffect(EffectSetting::kMgefCode_None);     // default FX effect = none
        SetHostility(Magic::kHostility_Neutral);        // default hostility = neutral
    }
    // handler - efithandler
    SetHandler(EfitHandler::Create(mgef.GetHandler().HandlerCode(),*this));
}
EffectItem::EffectItem(const EffectSetting& effect) : ::EffectItem(effect) {}
EffectItem::EffectItem(const EffectItem& source) : ::EffectItem(source) {}
void EffectItem::Destruct()
{
    // EffectItemExtra
    if (EffectItemExtra* extra = (EffectItemExtra*)scriptInfo)
    {
        delete extra;   // destroys name+icon strings and effect handler object
        scriptInfo = 0;
    }
}
// methods - debugging
void EffectItem::Dump()
{
    _MESSAGE("EffectItem <%p> using {%08X} %s",this,mgefCode,GetEffectSetting()->GetDebugDescEx().c_str());
    gLog.Indent();
    _MESSAGE("Magnitude = %i (%i)",GetMagnitude(),magnitude);
    _MESSAGE("Duration = %i (%i)",GetDuration(),duration);
    _MESSAGE("Area = %i (%i)",GetArea(),area);
    _MESSAGE("Range = %i (%i)",GetRange(),range);
    _MESSAGE("Proj. Range = %i (%i)",GetProjectileRange(), scriptInfo ? ((EffectItemExtra*)scriptInfo)->projectileRange : -1);
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
    _MESSAGE("Handler <%p> = ...",GetHandler());
    _MESSAGE("Cost (No Caster) = %f",MagickaCost());
    gLog.Outdent();
}

// memory patch addresses - construction, destruction
memaddr Initialize_Hook                         (0x00414790,0x00566A10);    // replace indirect constructor
memaddr Destruct_Hook                           (0x00413BB0,0x00566690);    // replace destructor
// memory patch addresses - general methods
memaddr CopyFrom_Hook                           (0x00413FC0,0x005666E0);
memaddr CompareTo_Hook                          (0x00414190,0x00566150);
memaddr SetEffectSetting_Hook                   (0x00413710,0x00566050);
memaddr GetMagickaCost_Hook                     (0x00413810,0x00565730);
memaddr GetMagickaCostForCaster_Hook            (0x00413890,0x0);
// memory path addresses - overrides
memaddr GetEffectName_Hook                      (0x004139F0,0x00565D00);
memaddr SetEffectName_Hook                      (0x00413960,0x00565C70);    // only call not overridden elsewhere is CS Import Names function
memaddr GetSchool_Hook                          (0x00412F20,0x00564E80);
memaddr SetSchool_Hook                          (0x0       ,0x00564E60);    // all calls overridden, included only for completeness
memaddr GetFXEffectCode_Hook                    (0x00412CB0,0x00564DF0);
memaddr SetFXEffectCode_Hook                    (0x0       ,0x00564E00);    // all calls overridden, included only for completeness
memaddr GetIsHostile_Hook                       (0x00413470,0x00565280);    
memaddr SetIsHostile_Hook                       (0x0       ,0x00564EA0);    // all calls overridden, included only for completeness
memaddr GetScript_Hook                          (0x006A4420,0x0);           // used in Dispel script command to determine if calling script needs to be terminated
// memory patch addresses - Spellmaking & Enchanting menus
    // ... yikes, there are a lot of these
// memory patch addresses - CS Dialogs
memaddr TESCS_CreateEfitDialog_PatchA           (0x0       ,0x00567F92);    // double click & edit
memaddr TESCS_CreateEfitDialog_PatchB           (0x0       ,0x0056821F);    // insert & new
memaddr BuildSchoolList_Hook                    (0x0       ,0x005654E0);
memaddr InitializeDialog_Hook                   (0x0       ,0x005664C0);
memaddr SetInDialog_Hook                        (0x0       ,0x00565DA0);
memaddr GetFromDialog_Hook                      (0x0       ,0x005657E0);
memaddr DialogMsgCallback_Hook                  (0x0       ,0x00566340);
memaddr CleanupDialog_Hook                      (0x0       ,0x00566979);    // just before destruction of temp working item in EffectItem dlg proc

// hooks
void _declspec(naked) SetEffectItemName_Hndl(void)
{
    _asm
    {
        push    [esp + 4]   // push string buffer of BSStringT
        call    EffectItem::SetEffectName   // set name via string buffer
        retn    8
    }
}
void _declspec(naked) GetFXEffectCode_Hndl(void)
{
    EffectItem* item;
    UInt32*     code;
    _asm
    {
        pushad
        mov     ebp, esp
        sub     esp, __LOCAL_SIZE
        mov     item, ecx
        lea     eax, [ebp + 0x1C]    // address of eax stored on stack
        mov     code, eax
   }
    if (EffectSetting* mgef = item->GetFXEffect()) *code = mgef->mgefCode;
    else *code = EffectSetting::kMgefCode_None;
    _asm
    {
        mov     esp, ebp
        popad
        retn
    }
}
void _declspec(naked) GetIsHostile_Hndl(void)
{
    EffectItem* item;
    bool*       host;
    _asm
    {
        pushad
        mov     ebp, esp
        sub     esp, __LOCAL_SIZE
        mov     item, ecx
        lea     eax, [ebp + 0x1C]    // address of eax stored on stack
        mov     host, eax
    }
    *host = (item->GetHostility() == Magic::kHostility_Hostile);
    _asm
    {
        mov     esp, ebp
        popad
        retn
    }
}
void _declspec(naked) SetIsHostile_Hndl(void)
{
    EffectItem* item;
    bool        host;
    _asm
    {
        pushad
        mov     ebp, esp
        sub     esp, __LOCAL_SIZE
        mov     item, ecx
        mov     eax, [ebp + 0x24]    // address of first argument
        mov     host, al
    }
    item->SetHostility(host ? Magic::kHostility_Hostile : Magic::kHostility_Neutral);
    _asm
    {
        mov     esp, ebp
        popad
        retn    4
    }
}
void _declspec(naked) GetScript_Hndl(void)
{
    EffectItem* item;
    Script**    result;
    _asm
    {
        pushad
        mov     ebp, esp
        sub     esp, __LOCAL_SIZE
        mov     item, ecx
        mov     eax, [ebp + 0x1C]    // address of stored eax on stack
        mov     result, eax
    }
    if (ScriptEfitHandler* scriptHandler = dynamic_cast<ScriptEfitHandler*>(item->GetHandler()))
    {
        *result = scriptHandler->GetScript();
    }
    else *result = 0;
    _asm
    {
        mov     esp, ebp
        popad
        retn
    }
}
#ifndef OBLIVION
void _declspec(naked) CleanupDialog_Hndl(void)
{
    EffectItem* item;
    HWND dialog;
    _asm
    {
        pushad
        mov     ebp, esp
        sub     esp, __LOCAL_SIZE
        mov     item, ecx
        mov     dialog, edi
    }
    item->CleanupDialog(dialog);
    item->Destruct();   // overwritten code: destruct temp item
    _asm
    {
        mov     esp, ebp
        popad
        retn
    }
}
#endif

// init hooks
void EffectItem::InitHooks()
{
    _MESSAGE("Initializing ...");

    // hook construction & destruction methods
    Initialize_Hook.WriteRelJump(memaddr::GetPointerToMember(&Initialize));
    Destruct_Hook.WriteRelJump(memaddr::GetPointerToMember(&Destruct));

    // hook basic parameter methods
    CopyFrom_Hook.WriteRelJump(memaddr::GetPointerToMember(&CopyFrom));
    CompareTo_Hook.WriteRelJump(memaddr::GetPointerToMember(&CompareTo));
    SetEffectSetting_Hook.WriteRelJump(memaddr::GetPointerToMember(&SetEffectSetting));

    // hook override methods
    GetEffectName_Hook.WriteRelJump(memaddr::GetPointerToMember(&GetEffectName));
    SetEffectName_Hook.WriteRelJump(&SetEffectItemName_Hndl);
    GetSchool_Hook.WriteRelJump(memaddr::GetPointerToMember(&GetSchool));
    SetSchool_Hook.WriteRelJump(memaddr::GetPointerToMember(&SetSchool));
    GetFXEffectCode_Hook.WriteRelJump(&GetFXEffectCode_Hndl);
    SetFXEffectCode_Hook.WriteRelJump(memaddr::GetPointerToMember(&SetFXEffect));
    GetIsHostile_Hook.WriteRelJump(&GetIsHostile_Hndl);
    SetIsHostile_Hook.WriteRelJump(&SetIsHostile_Hndl);
        // TODO - hook GetScript to return script for all scripted effects, not just SEFF

    // hook calculated property methods
    GetMagickaCost_Hook.WriteRelJump(memaddr::GetPointerToMember(&MagickaCost));
    GetMagickaCostForCaster_Hook.WriteRelJump(memaddr::GetPointerToMember(&MagickaCostForCaster));

    // patch creation of EFIT dialog in CS - replace CS module instance with local module instance
    TESCS_CreateEfitDialog_PatchA.WriteData32((UInt32)&hModule);
    TESCS_CreateEfitDialog_PatchB.WriteData32((UInt32)&hModule);

    // hook CS dialog methods
    #ifndef OBLIVION
    BuildSchoolList_Hook.WriteRelJump(memaddr::GetPointerToMember(&BuildSchoolList));
    InitializeDialog_Hook.WriteRelJump(memaddr::GetPointerToMember(&InitializeDialog));
    SetInDialog_Hook.WriteRelJump(memaddr::GetPointerToMember(&SetInDialog));
    GetFromDialog_Hook.WriteRelJump(memaddr::GetPointerToMember(&GetFromDialog));
    DialogMsgCallback_Hook.WriteRelJump(memaddr::GetPointerToMember(&HandleDialogEvent));
    CleanupDialog_Hook.WriteRelCall(&CleanupDialog_Hndl);
    #endif

}

}   //  end namespace OBME