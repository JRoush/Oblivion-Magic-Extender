#include "OBME/OBME_interface.h"
#include "OBME/OBME_version.h"
#include "OBME/EffectSetting.h"

#include "API/BSTypes/BSStringT.h"
#include "API/TESForms/TESObjectREFR.h"
#include "API/Magic/EffectSetting.h"
#include "API/TESFiles/TESFile.h"
#include "API/Actors/ActorValues.h"

/*
#include "rsh\GameForms\FormBase.h"
#include "rsh\GameForms\MagicEffects.h"
#include "rsh\EffectHandlers\EffectHandler.h"
#include "rsh\EffectHandlers\ActiveEffects.h"
#include "rsh\EffectHandlers\ModAVEffects.h"
#include "rsh\EffectHandlers\SummonEffects.h"
#include "rsh\GameObjects\Actor.h"
#include "rsh\GameObjects\TESSaveLoadGame.h"
#include "rsh\GameInternals\Oblivion_RTTI.h"
*/

/********************* EffectSetting Comand Interface ******************************/
/*
bool OBME_Interface::EffectSettingComandInterface::ResolveModMgefCode(UInt32& mgefCode, TESFile* file)
{
    return EffectSetting::ResolveModMgefCode(mgefCode,file);
}
bool OBME_Interface::EffectSettingComandInterface::ResolveSavedMgefCode(UInt32& mgefCode)
{
    return EffectSetting::ResolveSavedMgefCode(mgefCode);
}
EffectSetting* OBME_Interface::EffectSettingComandInterface::LookupMgef(UInt32 mgefCode)
{
    return EffectSetting::Lookup(mgefCode);
}
EffectSetting* OBME_Interface::EffectSettingComandInterface::CreateMgef(UInt32 newCode, EffectSetting* cloneFrom)
{
    if (EffectSetting::Lookup(newCode)) return NULL;                // provided new effect code is taken
    if (EffectSetting::IsMgefCodeDynamic(newCode)) return NULL;     // provided new effect code is dynamic
    // create effect
    EffectSetting* neweff;
    if (cloneFrom)
    {
        _VMESSAGE("OBME|CreateMgef: Cloning effect {%08X} '%s'/%08X", cloneFrom->mgefCode,cloneFrom->GetEditorID(),cloneFrom->formID);
        neweff = (EffectSetting*)cloneFrom->Clone(true,NULL);
    }
    else
    {
        _VMESSAGE("OBME|CreateMgef: Creating new effect");
        neweff = EffectSetting::Create();
    }
    // assign mgefCode & register
    if (EffectSetting::IsMgefCodeValid(newCode))
    {
        _DMESSAGE("OBME|CreateMgef: new code specified: {%08X}",newCode);
        neweff->mgefCode = newCode;
    }
    else
    {
        neweff->mgefCode = EffectSetting::GetUnusedDynamicCode();
        _DMESSAGE("OBME|CreateMgef: using dynamic code: {%08X}",neweff->mgefCode);
    }
    neweff->Register();
    // add to current mod/savegame
    #ifdef OBLIVION
     // in-game, add to created base object list   
     (**g_currentSaveLoad).AddCreatedBaseObject(neweff);
    #else
     // in CS, add to active mod file & update useage
     // TODO -test this !
     neweff->UpdateUsageInfo();
     neweff->SetFromActiveFile(true);
    #endif

    return neweff;
}
UInt32 OBME_Interface::EffectSettingComandInterface::GetHandler(EffectSetting* mgef)
{
    if (mgef && mgef->effectHandler) return mgef->effectHandler->GetEHCode();
    else return 0;    // invalid mgef or mgef has no handler    
}
void OBME_Interface::EffectSettingComandInterface::SetHandler(UInt32 newEHCode, EffectSetting* mgef)
{
    if (mgef) mgef->effectHandler = EffectHandler::Lookup(newEHCode);
}
UInt8 OBME_Interface::EffectSettingComandInterface::GetHostility(EffectSetting* mgef)
{
    if (mgef) return mgef->GetHostility();
    else return 0;    // invalid mgef
}
void OBME_Interface::EffectSettingComandInterface::SetHostility(UInt8 newHostility, EffectSetting* mgef)
{
    if (mgef) mgef->SetHostility(newHostility);
}
OBME_Interface::MixedType OBME_Interface::EffectSettingComandInterface::GetHandlerParam(const char* paramName, EffectSetting* mgef, UInt32& valueType)
{
    valueType = kType_Invalid;
    if (!mgef || !mgef->effectHandler) return NULL;

    switch (Swap32(mgef->effectHandler->GetEHCode()))
    {
    case 'SEFF':    // ScriptEffect
        if (_stricmp(paramName,         "Script") == 0) 
        {
            valueType = kType_FormID;
            return mgef->mgefParamA;
        }
        else if (_stricmp(paramName,    "UserParamIsFormID") == 0) 
        {
            valueType = kType_Numeric;
            return mgef->GetFlag(ScriptEffect::kMgefFlag_UserParamIsFormID);
        }
        else if (_stricmp(paramName,    "UserParamIsMgefCode") == 0) 
        {
            valueType = kType_Numeric;
            return mgef->GetFlag(ScriptEffect::kMgefFlag_UserParamIsMgefCode);
        }
        else return NULL;
    case 'SUMN':
    case 'SMAC':    
    case 'SMBO':    // SummonForm and children
        if (_stricmp(paramName,         "Summon") == 0) 
        {
            valueType = kType_FormID;
            return mgef->mgefParamA;
        }
        else if (_stricmp(paramName,    "SummonLimit") == 0) 
        {
            valueType = kType_FormID;
            return mgef->GetFlag(SummonEffect::kMgefFlag_UseVanillaLimit) ? 0x0 : mgef->mgefParamB;
        }
        else if (_stricmp(paramName,    "UseVanillaLimit") == 0) 
        {
            valueType = kType_Numeric;
            return mgef->GetFlag(SummonEffect::kMgefFlag_UseVanillaLimit);
        }
        else return NULL;
    case 'MDAV':    // ModAV
        if (_stricmp(paramName,         "DefaultAV") == 0)
        {
            valueType = kType_Numeric;
            return mgef->mgefParamA;
        }
        else if (_stricmp(paramName,    "SecondAV") == 0)
        { 
            valueType = kType_Numeric;
            return mgef->mgefParamB;
        }
        else if (_stricmp(paramName,    "AVPart") == 0)
        { 
            valueType = kType_Numeric;
            return (mgef->GetFlag(ModAVEffect::kMgefFlag_AVPartA) ? 1 : 0) + (mgef->GetFlag(ModAVEffect::kMgefFlag_AVPartB) ? 2 : 0);
        }
        else if (_stricmp(paramName,    "Detrimental") == 0)
        { 
            valueType = kType_Numeric;
            return mgef->GetFlag(ModAVEffect::kMgefFlag_Detrimental);
        }
        else if (_stricmp(paramName,    "AbilitySpecial") == 0)
        { 
            valueType = kType_Numeric;
            return mgef->GetFlag(ModAVEffect::kMgefFlag_AbilitySpecial);
        }
        else return NULL;
    case 'DSPL':    // Dispel
        if (_stricmp(paramName,         "mgefCode") == 0)
        { 
            valueType = kType_Numeric;
            return mgef->GetFlag(DispelEffect::kMgefFlag_ParamAIsEHCode) ? EffectSetting::kMgefCode_None : mgef->mgefParamA;
        }
        else if (_stricmp(paramName,    "ehCode") == 0)
        { 
            valueType = kType_Numeric;
            return mgef->GetFlag(DispelEffect::kMgefFlag_ParamAIsEHCode) ? mgef->mgefParamA : 0x0;
        }
        else if (_stricmp(paramName,    "AtomicDispel") == 0)
        { 
            valueType = kType_Numeric;
            return mgef->GetFlag(DispelEffect::kMgefFlag_AtomicDispel);
        }
        else if (_stricmp(paramName,    "DistributeMagicka") == 0)
        { 
            valueType = kType_Numeric;
            return mgef->GetFlag(DispelEffect::kMgefFlag_DistributeMagicka);
        }
        else if (_stricmp(paramName,    "AtomicMagicItem") == 0)
        { 
            valueType = kType_Numeric;
            return mgef->GetFlag(DispelEffect::kMgefFlag_AtomicMagicItem);
        }
        else if (_stricmp(paramName,    "DispelSpell") == 0)
        { 
            valueType = kType_Numeric;
            return mgef->mgefParamB & DispelEffect::kMagItm_Spell;
        }
        else if (_stricmp(paramName,    "DispelDisease") == 0)
        { 
            valueType = kType_Numeric;
            return mgef->mgefParamB & DispelEffect::kMagItm_Disease;
        }
        else if (_stricmp(paramName,    "DispelPower") == 0)
        { 
            valueType = kType_Numeric;
            return mgef->mgefParamB & DispelEffect::kMagItm_Power;
        }
        else if (_stricmp(paramName,    "DispelLesserPower") == 0)
        { 
            valueType = kType_Numeric;
            return mgef->mgefParamB & DispelEffect::kMagItm_LesserPower;
        }
        else if (_stricmp(paramName,    "DispelAbility") == 0)
        { 
            valueType = kType_Numeric;
            return mgef->mgefParamB & DispelEffect::kMagItm_Ability;
        }
        else if (_stricmp(paramName,    "DispelScrollEnchantment") == 0)
        { 
            valueType = kType_Numeric;
            return mgef->mgefParamB & DispelEffect::kMagItm_EnchScroll;
        }
        else if (_stricmp(paramName,    "DispelStaffEnchantment") == 0)
        { 
            valueType = kType_Numeric;
            return mgef->mgefParamB & DispelEffect::kMagItm_EnchStaff;
        }
        else if (_stricmp(paramName,    "DispelWeaponEnchantment") == 0)
        { 
            valueType = kType_Numeric;
            return mgef->mgefParamB & DispelEffect::kMagItm_EnchWeapon;
        }
        else if (_stricmp(paramName,    "DispelApparelEnchantment") == 0)
        { 
            valueType = kType_Numeric;
            return mgef->mgefParamB & DispelEffect::kMagItm_EnchApparel;
        }
        else if (_stricmp(paramName,    "DispelAlchemyItem") == 0)
        { 
            valueType = kType_Numeric;
            return mgef->mgefParamB & DispelEffect::kMagItm_AlchemyItem;
        }
        else if (_stricmp(paramName,    "DispelIngredient") == 0)
        { 
            valueType = kType_Numeric;
            return mgef->mgefParamB & DispelEffect::kMagItm_Ingredient;
        }
        else return NULL;
    }  

    return NULL;
}
bool OBME_Interface::EffectSettingComandInterface::SetHandlerParam(const char* paramName, EffectSetting* mgef, OBME_Interface::MixedType newValue)
{
    if (!mgef || !mgef->effectHandler) return false;

    switch (Swap32(mgef->effectHandler->GetEHCode()))
    {
    case 'SEFF':
        if (_stricmp(paramName,         "Script") == 0) 
            mgef->mgefParamA = newValue;
        else if (_stricmp(paramName,    "UserParamIsFormID") == 0) 
            mgef->SetFlag(ScriptEffect::kMgefFlag_UserParamIsFormID,newValue);
        else if (_stricmp(paramName,    "UserParamIsMgefCode") == 0) 
            mgef->SetFlag(ScriptEffect::kMgefFlag_UserParamIsMgefCode,newValue);
        else return false;
    case 'SUMN':
    case 'SMAC':
    case 'SMBO':
        if (_stricmp(paramName,         "Summon") == 0) 
            mgef->mgefParamA = newValue;
        else if (_stricmp(paramName,    "SummonLimit") == 0) 
            mgef->mgefParamB = newValue;
        else if (_stricmp(paramName,    "UseVanillaLimit") == 0) 
            mgef->SetFlag(SummonEffect::kMgefFlag_UseVanillaLimit,newValue);
        else return false;
    case 'MDAV':
        if (_stricmp(paramName,         "DefaultAV") == 0) 
            mgef->mgefParamA = newValue;
        else if (_stricmp(paramName,    "SecondAV") == 0) 
            mgef->mgefParamB = newValue;
        else if (_stricmp(paramName,    "AVPart") == 0) 
        {
            mgef->SetFlag(ModAVEffect::kMgefFlag_AVPartA, newValue & 1);
            mgef->SetFlag(ModAVEffect::kMgefFlag_AVPartB, newValue & 2);
        }
        else if (_stricmp(paramName,    "Detrimental") == 0) 
            mgef->SetFlag(ModAVEffect::kMgefFlag_Detrimental,newValue);
        else if (_stricmp(paramName,    "AbilitySpecial") == 0) 
            mgef->SetFlag(ModAVEffect::kMgefFlag_AbilitySpecial,newValue);
        else return false;
    case 'DSPL':
        if (_stricmp(paramName,         "mgefCode") == 0) 
        {
            mgef->SetFlag(DispelEffect::kMgefFlag_ParamAIsEHCode,false);
            mgef->mgefParamA = newValue;
        }
        else if (_stricmp(paramName,    "ehCode") == 0) 
        {
            mgef->SetFlag(DispelEffect::kMgefFlag_ParamAIsEHCode,true);
            mgef->mgefParamA = newValue;
        }
        else if (_stricmp(paramName,    "AtomicDispel") == 0) 
            mgef->SetFlag(DispelEffect::kMgefFlag_AtomicDispel, newValue);
        else if (_stricmp(paramName,    "DistributeMagicka") == 0) 
            mgef->SetFlag(DispelEffect::kMgefFlag_DistributeMagicka, newValue);
        else if (_stricmp(paramName,    "AtomicMagicItem") == 0) 
            mgef->SetFlag(DispelEffect::kMgefFlag_AtomicMagicItem, newValue);
        else if (_stricmp(paramName,    "DispelSpell") == 0) 
        {
            if (newValue) mgef->mgefParamB |= DispelEffect::kMagItm_Spell;
            else mgef->mgefParamB &= ~DispelEffect::kMagItm_Spell;
        }
        else if (_stricmp(paramName,    "DispelDisease") == 0) 
        {
            if (newValue) mgef->mgefParamB |= DispelEffect::kMagItm_Disease;
            else mgef->mgefParamB &= ~DispelEffect::kMagItm_Disease;
        }
        else if (_stricmp(paramName,    "DispelPower") == 0) 
        {
        if (newValue) mgef->mgefParamB |= DispelEffect::kMagItm_Power;
            else mgef->mgefParamB &= ~DispelEffect::kMagItm_Power;
        }
        else if (_stricmp(paramName,    "DispelLesserPower") == 0) 
        {
            if (newValue) mgef->mgefParamB |= DispelEffect::kMagItm_LesserPower;
            else mgef->mgefParamB &= ~DispelEffect::kMagItm_LesserPower;
        }
        else if (_stricmp(paramName,    "DispelAbility") == 0) 
        {
            if (newValue) mgef->mgefParamB |= DispelEffect::kMagItm_Ability;
            else mgef->mgefParamB &= ~DispelEffect::kMagItm_Ability;
        }
        else if (_stricmp(paramName,    "DispelScrollEnchantment") == 0) 
        {
            if (newValue) mgef->mgefParamB |= DispelEffect::kMagItm_EnchScroll;
            else mgef->mgefParamB &= ~DispelEffect::kMagItm_EnchScroll;
        }
        else if (_stricmp(paramName,    "DispelStaffEnchantment") == 0) 
        {
            if (newValue) mgef->mgefParamB |= DispelEffect::kMagItm_EnchStaff;
            else mgef->mgefParamB &= ~DispelEffect::kMagItm_EnchStaff;
        }
        else if (_stricmp(paramName,    "DispelWeaponEnchantment") == 0) 
        {
            if (newValue) mgef->mgefParamB |= DispelEffect::kMagItm_EnchWeapon;
            else mgef->mgefParamB &= ~DispelEffect::kMagItm_EnchWeapon;
        }
        else if (_stricmp(paramName,    "DispelApparelEnchantment") == 0) 
        {
            if (newValue) mgef->mgefParamB |= DispelEffect::kMagItm_EnchApparel;
            else mgef->mgefParamB &= ~DispelEffect::kMagItm_EnchApparel;
        }
        else if (_stricmp(paramName,    "DispelAlchemyItem") == 0) 
        {
            if (newValue) mgef->mgefParamB |= DispelEffect::kMagItm_AlchemyItem;
            else mgef->mgefParamB &= ~DispelEffect::kMagItm_AlchemyItem;
        }
        else if (_stricmp(paramName,    "DispelIngredient") == 0) 
        {
            if (newValue) mgef->mgefParamB |= DispelEffect::kMagItm_Ingredient;
            else mgef->mgefParamB &= ~DispelEffect::kMagItm_Ingredient;
        }
        else return false;
    default:
        return false;
    }  

    return true;
}
*/

/********************* EffectItem Comand Interface ******************************/
/*
EffectItem* OBME_Interface::EffectItemComandInterface::GetNthEffectItem(TESForm* magicObject, SInt32 efitIndex)
{
    if (EffectItemList* effects = (EffectItemList*)RTTI::DynamicCast(magicObject, 0, RTTI::TESForm, RTTI::EffectItemList, 0)) 
        return effects->GetItemByIndex(efitIndex);
    else return NULL;
}
void OBME_Interface::EffectItemComandInterface::GetName(EffectItem* item, String& name)
{
    if (item) item->GetName((::String*)&name);
}
void OBME_Interface::EffectItemComandInterface::SetName(EffectItem* item, const char* newName)
{
    if (item) item->SetEfixName(newName);
}
void OBME_Interface::EffectItemComandInterface::ClearName(EffectItem* item)
{
    if (item) item->SetEfixOverride(EffectItem::kEfixOvrd_Name,false);
}
const char* OBME_Interface::EffectItemComandInterface::GetIconPath(EffectItem* item)
{
    if (item) return item->GetIconPath();
    else return NULL;
}
void OBME_Interface::EffectItemComandInterface::SetIconPath(EffectItem* item, const char* newPath)
{
    if (item) item->SetEfixIconPath(newPath);
}
void OBME_Interface::EffectItemComandInterface::ClearIconPath(EffectItem* item)
{
    if (item) item->SetEfixOverride(EffectItem::kEfixOvrd_Icon,false);
}
UInt8 OBME_Interface::EffectItemComandInterface::GetHostility(EffectItem* item)
{
    if (item) return item->GetHostility();
    else return 0;
}
void OBME_Interface::EffectItemComandInterface::SetHostility(EffectItem* item, UInt8 newHostility)
{
    if (item) item->SetEfixHostility(newHostility);
}
void OBME_Interface::EffectItemComandInterface::ClearHostility(EffectItem* item)
{
    if (item) item->SetEfixHostility(EffectSetting::kMgefHost__NONE);
}   
float OBME_Interface::EffectItemComandInterface::GetBaseCost(EffectItem* item)
{
    if (item) return item->GetBaseCost();
    else return -1;
}
void OBME_Interface::EffectItemComandInterface::SetBaseCost(EffectItem* item, float newBaseCost)
{
    if (item) item->SetEfixBaseCost(newBaseCost);
}
void OBME_Interface::EffectItemComandInterface::ClearBaseCost(EffectItem* item)
{
    if (item) item->SetEfixOverride(EffectItem::kEfixOvrd_BaseCost,false);
}
UInt32 OBME_Interface::EffectItemComandInterface::GetResistAV(EffectItem* item)
{
    if (item) return item->GetResistAV();
    else return Actor::kActorVal__NONE;
}
void OBME_Interface::EffectItemComandInterface::SetResistAV(EffectItem* item, UInt32 newResistAV)
{
    if (item) item->SetEfixResistAV(newResistAV);
}
void OBME_Interface::EffectItemComandInterface::ClearResistAV(EffectItem* item)
{
    if (item) item->SetEfixOverride(EffectItem::kEfixOvrd_ResistAV,false);
}
UInt32 OBME_Interface::EffectItemComandInterface::GetSchool(EffectItem* item)
{
    if (item) return item->GetSchool();
    else return EffectSetting::kSchool_MAX;
}
void OBME_Interface::EffectItemComandInterface::SetSchool(EffectItem* item, UInt32 newSchool)
{
    if (item) item->SetEfixSchool(newSchool);
}
void OBME_Interface::EffectItemComandInterface::ClearSchool(EffectItem* item)
{
    if (item) item->SetEfixOverride(EffectItem::kEfixOvrd_School,false);
}
UInt32 OBME_Interface::EffectItemComandInterface::GetVFXCode(EffectItem* item)
{
    if (item) return item->GetVFXCode();
    else return EffectSetting::kMgefCode_None;
}
void OBME_Interface::EffectItemComandInterface::SetVFXCode(EffectItem* item, UInt32 newVFXCode)
{
    if (item) item->SetEfixVFXCode(newVFXCode);
}
void OBME_Interface::EffectItemComandInterface::ClearVFXCode(EffectItem* item)
{
    if (item) item->SetEfixOverride(EffectItem::kEfixOvrd_VFXCode,false);
}
OBME_Interface::MixedType OBME_Interface::EffectItemComandInterface::GetHandlerParam(const char* paramName, EffectItem* item, UInt32& valueType)
{   
    valueType = kType_Invalid;
    if (!item || !item->mgef || !item->mgef->effectHandler) return NULL;

    switch (Swap32(item->mgef->effectHandler->GetEHCode()))
    {
    case 'SEFF':
        if (_stricmp(paramName,         "Script") == 0) 
        {
            valueType = kType_FormID;
            return item->GetEfixOverride(EffectItem::kEfixOvrd_EfixParam) ? item->GetEfixParam() : item->mgef->mgefParamA;
        }
        else if (_stricmp(paramName,    "UserParam") == 0) 
        {
            valueType = kType_Numeric;
            return item->GetEfitParam();
        }
        else if (_stricmp(paramName,    "UserParamIsFormID") == 0)
        { 
            valueType = kType_Numeric;
            return item->GetFlag(ScriptEffect::kMgefFlag_UserParamIsFormID);
        }
        else if (_stricmp(paramName,    "UserParamIsMgefCode") == 0)
        { 
            valueType = kType_Numeric;
            return item->GetFlag(ScriptEffect::kMgefFlag_UserParamIsMgefCode);
        }
        else return NULL;
    case 'MDAV':
        if (_stricmp(paramName,         "ActorValue") == 0)
        { 
            valueType = kType_Numeric;
            return item->GetEfitParam();
        }
        else if (_stricmp(paramName,    "AVPart") == 0)
        { 
            valueType = kType_Numeric;
            return (item->GetFlag(ModAVEffect::kMgefFlag_AVPartA) ? 1 : 0) + (item->GetFlag(ModAVEffect::kMgefFlag_AVPartB) ? 2 : 0);
        }
        else if (_stricmp(paramName,    "Detrimental") == 0)
        { 
            valueType = kType_Numeric;
            return item->GetFlag(ModAVEffect::kMgefFlag_Detrimental);
        }
        else if (_stricmp(paramName,    "AbilitySpecial") == 0)
        { 
            valueType = kType_Numeric;
            return item->GetFlag(ModAVEffect::kMgefFlag_AbilitySpecial);
        }
        else return NULL;
    }    

    return NULL;
}
bool OBME_Interface::EffectItemComandInterface::SetHandlerParam(const char* paramName, EffectItem* item, OBME_Interface::MixedType newValue)
{
    if (!item || !item->mgef || !item->mgef->effectHandler) return false;

    switch (Swap32(item->mgef->effectHandler->GetEHCode()))
    {
    case 'SEFF':
        if (_stricmp(paramName,         "Script") == 0) 
            item->SetEfixParam(newValue);
        else if (_stricmp(paramName,    "UserParam") == 0) 
            item->SetEfitParam(newValue);
        else if (_stricmp(paramName,    "UserParamIsFormID") == 0) 
            item->SetEfixFlag(ScriptEffect::kMgefFlag_UserParamIsFormID, newValue);
        else if (_stricmp(paramName,    "UserParamIsMgefCode") == 0) 
            item->SetEfixFlag(ScriptEffect::kMgefFlag_UserParamIsMgefCode, newValue);
        else return false;
    case 'MDAV':
        if (_stricmp(paramName,         "ActorValue") == 0) 
            item->SetEfitParam(newValue);
        else if (_stricmp(paramName,    "AVPart") == 0) 
        {
            item->SetEfixFlag(ModAVEffect::kMgefFlag_AVPartA, newValue & 1);
            item->SetEfixFlag(ModAVEffect::kMgefFlag_AVPartB, newValue & 2);
        }
        else if (_stricmp(paramName,    "Detrimental") == 0) 
            item->SetEfixFlag(ModAVEffect::kMgefFlag_Detrimental, newValue);
        else if (_stricmp(paramName,    "AbilitySpecial") == 0) 
            item->SetEfixFlag(ModAVEffect::kMgefFlag_AbilitySpecial, newValue);
        else return false;
    default:
        return false;
    } 

    return true;
}
void OBME_Interface::EffectItemComandInterface::ClearHandlerParam(const char* paramName, EffectItem* item)
{
    if (!item || !item->mgef || !item->mgef->effectHandler) return;

    switch (Swap32(item->mgef->effectHandler->GetEHCode()))
    {
    case 'SEFF':
        if (_stricmp(paramName,         "Script") == 0) 
            item->SetEfixOverride(EffectItem::kEfixOvrd_EfixParam, false);
        else if (_stricmp(paramName,    "UserParamIsFormID") == 0) 
            item->SetEfixOverride(ScriptEffect::kMgefFlag_UserParamIsFormID, false);
        else if (_stricmp(paramName,    "UserParamIsMgefCode") == 0) 
            item->SetEfixOverride(ScriptEffect::kMgefFlag_UserParamIsMgefCode, false);
        break;
    case 'MDAV':
        if (_stricmp(paramName,         "AVPart") == 0) 
            item->SetEfixOverride(ModAVEffect::kMgefFlag_AVPartA | ModAVEffect::kMgefFlag_AVPartB, false);
        else if (_stricmp(paramName,    "Detrimental") == 0) 
            item->SetEfixOverride(ModAVEffect::kMgefFlag_Detrimental, false);
        else if (_stricmp(paramName,    "AbilitySpecial") == 0) 
            item->SetEfixOverride(ModAVEffect::kMgefFlag_AbilitySpecial, false);
        break;
    }            
}
*/

/********************* EffectHandler Comand Interface ******************************/  
/*
EffectHandler* OBME_Interface::EffectHandlerComandInterface::LookupEffectHandler(UInt32 ehCode)
{
    return EffectHandler::Lookup(ehCode);
}
void OBME_Interface::EffectHandlerComandInterface::RegisterEffectHandler(EffectHandler* eh)
{
    if (eh) eh->Register();
}
*/

/********************* Private Comand Interface ******************************/  
const char* OBME_Interface::PrivateInterface::Description()
{
    static char buffer[0x100];
    sprintf_s(buffer,sizeof(buffer),"OBME by JRoush, v%i.%i beta%i",OBME_MAJOR_VERSION,OBME_MINOR_VERSION,OBME_BETA_VERSION);
    return buffer;
}
void OBME_Interface::PrivateInterface::OBMETest(TESObjectREFR* thisObj, const char* argA, const char* argB, const char* argC)
{
    BSStringT desc;
    if (thisObj) thisObj->GetDebugDescription(desc);
    _DMESSAGE("Test command on <%p> '%s' w/ args A='%s' B='%s' C='%s'",thisObj,desc.c_str(),argA,argB,argC);

}

// constructor
OBME_Interface::OBME_Interface()
: version(kInterfaceVersion)
{}
