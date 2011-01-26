#include "OBME/EffectHandlers/DispelEffect.h"
#include "OBME/EffectHandlers/EffectHandler.rc.h"
#include "OBME/EffectSetting.h"
#include "OBME/Magic.h"

#include "API/TESFiles/TESFile.h"
#include "API/CSDialogs/TESDialog.h"
#include "API/Actors/ActorValues.h"
#include "Components/TESFileFormats.h"

// global method for returning small enumerations as booleans
inline bool BoolEx(UInt8 value) {return *(bool*)&value;}
// send in lieu of WM_COMMAND to avoid problems with TESFormIDListView::DlgProc 
static const UInt32 WM_USERCOMMAND =  WM_APP + 0x55; 
static const char* kNoneEntry = " NONE ";

namespace OBME {

/*************************** DispelMgefHandler ********************************/
DispelMgefHandler::DispelMgefHandler(EffectSetting& effect) 
: MgefHandler(effect) , ehCode(0), mgefCode(0), magicTypes(1 << Magic::kMagicType_Spell), castTypes(0), 
    magicitemFormID(0), atomicItems(true), atomicDispel(true), distributeMag(false), useCastingCost(false)
{
    if (effect.mgefCode == Swap32('DSPL')) useCastingCost = true;   // vanilla dispel compares magnitude to casting cost
}
// serialization
bool DispelMgefHandler::LoadHandlerChunk(TESFile& file, UInt32 RecordVersion)
{
    if (file.currentChunk.chunkLength != kEHNDChunkSize) return false;  // wrong chunk size
    file.GetChunkData(&ehCode,kEHNDChunkSize);
    TESFileFormats::ResolveModValue(mgefCode,file,TESFileFormats::kResType_MgefCode);  // resolve targeted mgefCode  
    TESFileFormats::ResolveModValue(magicitemFormID,file,TESFileFormats::kResType_FormID);  // resolve targeted magic item
    return true;
}
void DispelMgefHandler::SaveHandlerChunk()
{
    TESForm::PutFormRecordChunkData(Swap32('EHND'),&ehCode,kEHNDChunkSize);
}
void DispelMgefHandler::LinkHandler()
{
    // add cross references
    #ifndef OBLIVION
    if (::EffectSetting* mgef = EffectSettingCollection::LookupByCode(mgefCode)) mgef->AddCrossReference(&parentEffect);
    if (TESForm* form = TESForm::LookupByFormID(magicitemFormID)) form->AddCrossReference(&parentEffect);
    #endif
}
void DispelMgefHandler::UnlinkHandler() 
{
    // remove cross references
    #ifndef OBLIVION
    if (::EffectSetting* mgef = EffectSettingCollection::LookupByCode(mgefCode)) mgef->RemoveCrossReference(&parentEffect);
    if (TESForm* form = TESForm::LookupByFormID(magicitemFormID)) form->RemoveCrossReference(&parentEffect);
    #endif
}
// copy/compare
void DispelMgefHandler::CopyFrom(const MgefHandler& copyFrom)
{
    const DispelMgefHandler* src = dynamic_cast<const DispelMgefHandler*>(&copyFrom);
    if (!src) return; // wrong polymorphic type

    // modify references
    #ifndef OBLIVION
    if (::EffectSetting* mgef = EffectSettingCollection::LookupByCode(mgefCode)) mgef->RemoveCrossReference(&parentEffect);
    if (TESForm* form = TESForm::LookupByFormID(magicitemFormID)) form->RemoveCrossReference(&parentEffect);
    if (::EffectSetting* mgef = EffectSettingCollection::LookupByCode(src->mgefCode)) mgef->AddCrossReference(&parentEffect);
    if (TESForm* form = TESForm::LookupByFormID(src->magicitemFormID)) form->AddCrossReference(&parentEffect);
    #endif

    // copy members
    ehCode = src->ehCode;
    mgefCode = src->mgefCode;
    magicTypes = src->magicTypes;
    castTypes = src->castTypes;
    magicitemFormID = src->magicitemFormID;
    atomicItems = src->atomicItems;
    atomicDispel = src->atomicDispel;
    distributeMag = src->distributeMag;
    useCastingCost = src->useCastingCost;
}
bool DispelMgefHandler::CompareTo(const MgefHandler& compareTo)
{   
    if (MgefHandler::CompareTo(compareTo)) return BoolEx(2);
    const DispelMgefHandler* src = dynamic_cast<const DispelMgefHandler*>(&compareTo);
    if (!src) return BoolEx(4); // wrong polymorphic type

    // compare members
    if (ehCode != src->ehCode) return BoolEx(10);
    if (mgefCode != src->mgefCode) return BoolEx(20);
    if (magicTypes != src->magicTypes) return BoolEx(30);
    if (castTypes != src->castTypes) return BoolEx(10);
    if (magicitemFormID != src->magicitemFormID) return BoolEx(40);
    if (atomicItems != src->atomicItems) return BoolEx(50);
    if (atomicDispel != src->atomicDispel) return BoolEx(60);
    if (distributeMag != src->distributeMag) return BoolEx(70);
    if (useCastingCost != src->useCastingCost) return BoolEx(80);

    // handlers are identical
    return false;
}
#ifndef OBLIVION
// reference management in CS
void DispelMgefHandler::RemoveFormReference(TESForm& form) 
{
    if (&form == (TESForm*)EffectSettingCollection::LookupByCode(mgefCode)) mgefCode = 0;
    if (form.formID == magicitemFormID) magicitemFormID = 0;
}
// child Dialog in CS
INT DispelMgefHandler::DialogTemplateID() { return IDD_MGEF_DSPL; }
void DispelMgefHandler::InitializeDialog(HWND dialog)
{
    HWND ctl = 0;
    // Effect Handler
    ctl = GetDlgItem(dialog,IDC_DSPL_EFFECTHANDLER);
    TESComboBox::Clear(ctl);
    TESComboBox::AddItem(ctl,kNoneEntry,0); // 'None' entry
    UInt32 ehCode = 0;
    const char* ehName = 0;
    while (EffectHandler::GetNextHandler(ehCode,ehName))
    {
        TESComboBox::AddItem(ctl,ehName,(void*)ehCode);
    }
    // Effect Setting
    ctl = GetDlgItem(dialog,IDC_DSPL_EFFECTSETTING);
    TESComboBox::PopulateWithForms(ctl,TESForm::kFormType_EffectSetting,true,true);
    // Magic Item Type
    ctl = GetDlgItem(dialog,IDC_DSPL_MAGICITEMTYPE);
    TESComboBox::Clear(ctl);
    TESComboBox::AddItem(ctl,kNoneEntry,0); // 'None' entry
    TESComboBox::AddItem(ctl,TESForm::GetFormTypeName(TESForm::kFormType_Spell),(void*)TESForm::kFormType_Spell);
    TESComboBox::AddItem(ctl,TESForm::GetFormTypeName(TESForm::kFormType_Enchantment),(void*)TESForm::kFormType_Enchantment);
    TESComboBox::AddItem(ctl,TESForm::GetFormTypeName(TESForm::kFormType_AlchemyItem),(void*)TESForm::kFormType_AlchemyItem);
    TESComboBox::AddItem(ctl,TESForm::GetFormTypeName(TESForm::kFormType_Ingredient),(void*)TESForm::kFormType_Ingredient);
    TESComboBox::AddItem(ctl,TESForm::GetFormTypeName(TESForm::kFormType_LeveledSpell),(void*)TESForm::kFormType_LeveledSpell);
}
bool DispelMgefHandler::DialogMessageCallback(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    // parse message
    switch (uMsg)
    {
    case WM_USERCOMMAND:
    case WM_COMMAND:
    {
        if (LOWORD(wParam) != IDC_DSPL_MAGICITEMTYPE || HIWORD(wParam) != CBN_SELCHANGE) break; // only process changes in item type combo
        _VMESSAGE("Magic Item Type changed");
        HWND ctl = GetDlgItem(dialog,IDC_DSPL_MAGICITEM);
        UInt8 type = (UInt8)TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_DSPL_MAGICITEMTYPE));  // get form type
        if (type) 
        {
            EnableWindow(ctl,true);
            TESComboBox::PopulateWithForms(ctl,type,true,false);
            TESComboBox::SetCurSelByData(ctl,TESForm::LookupByFormID(magicitemFormID));
        }
        else 
        {
            TESComboBox::Clear(ctl); 
            EnableWindow(ctl,false);
        }
        result = 0; return true;  // retval = false signals command handled
    }
    }
    return false;
}
void DispelMgefHandler::SetInDialog(HWND dialog)
{
    HWND ctl = 0;
    // Effect Handler
    TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_DSPL_EFFECTHANDLER),(void*)ehCode);
    // Effect Setting
    ::EffectSetting* mgef = EffectSettingCollection::LookupByCode(mgefCode);
    TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_DSPL_EFFECTSETTING),mgef);
    // Magic Type Checkboxes
    CheckDlgButton(dialog,IDC_DSPL_SPELLS,magicTypes & (1 << Magic::kMagicType_Spell));
    CheckDlgButton(dialog,IDC_DSPL_DISEASES,magicTypes & (1 << Magic::kMagicType_Disease));
    CheckDlgButton(dialog,IDC_DSPL_POWERS,magicTypes & (1 << Magic::kMagicType_Power));
    CheckDlgButton(dialog,IDC_DSPL_LPOWERS,magicTypes & (1 << Magic::kMagicType_LesserPower));
    CheckDlgButton(dialog,IDC_DSPL_ABILITIES,magicTypes & (1 << Magic::kMagicType_Ability));
    CheckDlgButton(dialog,IDC_DSPL_POISONS,magicTypes & (1 << Magic::kMagicType_Poison));
    CheckDlgButton(dialog,IDC_DSPL_POTIONS,magicTypes & (1 << Magic::kMagicType_AlchemyItem));
    CheckDlgButton(dialog,IDC_DSPL_INGREDIENTS,magicTypes & (1 << Magic::kMagicType_Ingredient));
    // Cast Type Checkboxes    
    CheckDlgButton(dialog,IDC_DSPL_SCROLLENCH,castTypes & (1 << ::Magic::kCast_Once));
    CheckDlgButton(dialog,IDC_DSPL_STAFFENCH,castTypes & (1 << ::Magic::kCast_WhenUsed));
    CheckDlgButton(dialog,IDC_DSPL_WEAPENCH,castTypes & (1 << ::Magic::kCast_WhenStrikes));
    CheckDlgButton(dialog,IDC_DSPL_APPARELENCH,castTypes & (1 << ::Magic::kCast_Constant));
    // Magic Item Type Select
    TESForm* form = TESForm::LookupByFormID(magicitemFormID);
    TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_DSPL_MAGICITEMTYPE),form ? (void*)form->GetFormType() : 0);
    SendNotifyMessage(dialog, WM_USERCOMMAND, MAKEWPARAM(IDC_DSPL_MAGICITEMTYPE,CBN_SELCHANGE),(LPARAM)GetDlgItem(dialog,IDC_DSPL_MAGICITEMTYPE));
    // behaviors    
    CheckDlgButton(dialog,IDC_DSPL_ATOMICITEM,atomicItems);
    CheckDlgButton(dialog,IDC_DSPL_ATOMICDISPEL,atomicDispel);
    CheckDlgButton(dialog,IDC_DSPL_DISTRIBUTEMAG,distributeMag);
    CheckDlgButton(dialog,IDC_DSPL_USECASTINGCOST,useCastingCost);
}
void DispelMgefHandler::GetFromDialog(HWND dialog)
{
    HWND ctl = 0;
    // Effect Handler
    ehCode = (UInt32)TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_DSPL_EFFECTHANDLER));
    // Effect Setting
    EffectSetting* mgef = (EffectSetting*)TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_DSPL_EFFECTSETTING));
    mgefCode = mgef ? mgef->mgefCode : 0;    
    // Magic Item Type Checkboxes
    magicTypes = 0;
    if (IsDlgButtonChecked(dialog,IDC_DSPL_SPELLS)) magicTypes |= (1 << Magic::kMagicType_Spell);
    if (IsDlgButtonChecked(dialog,IDC_DSPL_DISEASES)) magicTypes |= (1 << Magic::kMagicType_Disease);
    if (IsDlgButtonChecked(dialog,IDC_DSPL_POWERS)) magicTypes |= (1 << Magic::kMagicType_Power);
    if (IsDlgButtonChecked(dialog,IDC_DSPL_LPOWERS)) magicTypes |= (1 << Magic::kMagicType_LesserPower);
    if (IsDlgButtonChecked(dialog,IDC_DSPL_ABILITIES)) magicTypes |= (1 << Magic::kMagicType_Ability);
    if (IsDlgButtonChecked(dialog,IDC_DSPL_POISONS)) magicTypes |= (1 << Magic::kMagicType_Poison);
    if (IsDlgButtonChecked(dialog,IDC_DSPL_POTIONS)) magicTypes |= (1 << Magic::kMagicType_AlchemyItem);
    if (IsDlgButtonChecked(dialog,IDC_DSPL_INGREDIENTS)) magicTypes |= (1 << Magic::kMagicType_Ingredient);
    // Cast Type Checkboxes    
    castTypes = 0;    
    if (IsDlgButtonChecked(dialog,IDC_DSPL_SCROLLENCH)) castTypes |= (1 << Magic::kCast_Once);
    if (IsDlgButtonChecked(dialog,IDC_DSPL_STAFFENCH)) castTypes |= (1 << Magic::kCast_WhenUsed);
    if (IsDlgButtonChecked(dialog,IDC_DSPL_WEAPENCH)) castTypes |= (1 << Magic::kCast_WhenStrikes);
    if (IsDlgButtonChecked(dialog,IDC_DSPL_APPARELENCH)) castTypes |= (1 << Magic::kCast_Constant);
    // Magic Item
    if (0 == TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_DSPL_MAGICITEMTYPE))) magicitemFormID = 0;
    else
    {
        TESForm* form = (TESForm*)TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_DSPL_MAGICITEM));
        magicitemFormID = form ? form->formID : 0;
    }
    // behaviors    
    atomicItems = IsDlgButtonChecked(dialog,IDC_DSPL_ATOMICITEM);
    atomicDispel = IsDlgButtonChecked(dialog,IDC_DSPL_ATOMICDISPEL);
    distributeMag = IsDlgButtonChecked(dialog,IDC_DSPL_DISTRIBUTEMAG);
    useCastingCost = IsDlgButtonChecked(dialog,IDC_DSPL_USECASTINGCOST);
}
#endif
/*************************** DispelEfitHandler ********************************/
/*************************** DispelEffect ********************************/
#ifdef OBLIVION 
DispelEffect::DispelEffect(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem)
: ::DispelEffect(caster,magicItem,(::EffectItem*)effectItem) 
{
}
::ActiveEffect* DispelEffect::Clone() const
{
    ::ActiveEffect* neweff = Make(caster,magicItem,(OBME::EffectItem*)effectItem); 
    CopyTo(*neweff); 
    return neweff;
}
::ActiveEffect* DispelEffect::Make(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem)
{
    return new DispelEffect(caster,magicItem,effectItem);
}
#endif
}   //  end namespace OBME