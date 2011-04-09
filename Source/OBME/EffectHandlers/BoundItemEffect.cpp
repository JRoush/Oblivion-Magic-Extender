#include "OBME/EffectHandlers/BoundItemEffect.h"
#include "OBME/EffectHandlers/EffectHandler.rc.h"
#include "OBME/EffectSetting.h"

#include "API/TESFiles/TESFile.h"
#include "API/CSDialogs/TESDialog.h"
#include "Components/FormRefCounter.h"

// global method for returning small enumerations as booleans
inline bool BoolEx(UInt8 value) {return *(bool*)&value;}
// send in lieu of WM_COMMAND to avoid problems with TESFormIDListView::DlgProc 
static const UInt32 WM_USERCOMMAND =  WM_APP + 0x55; 
static const char* kNoneEntry = " NONE ";

namespace OBME {

/*************************** BoundItemMgefHandler ********************************/
BoundItemMgefHandler::BoundItemMgefHandler(EffectSetting& effect) 
 : AssociatedItemMgefHandler(effect)
{}
// serialization
void BoundItemMgefHandler::LinkHandler()
{
    TESForm* assocItem = TESForm::LookupByFormID(parentEffect.mgefParam);
    #ifndef OBLIVION        
    if (assocItem)  FormRefCounter::AddReference(&parentEffect,assocItem);   // add a cross ref for the associated form
    #endif
    if (!parentEffect.GetFlag(EffectSetting::kMgefFlagShift_UseArmor) && !parentEffect.GetFlag(EffectSetting::kMgefFlagShift_UseWeapon))
    {
        // Use* flags are not set properly, attempt to guess value from assoc item and/or defaults
        if (assocItem && assocItem->GetFormType() == TESForm::kFormType_Weapon) // assoc item is a weapon
            parentEffect.SetFlag(EffectSetting::kMgefFlagShift_UseWeapon,true);
        else if (assocItem && assocItem->GetFormType() == TESForm::kFormType_Armor) // assoc item is armor
            parentEffect.SetFlag(EffectSetting::kMgefFlagShift_UseArmor,true);
        else if (EffectSetting::GetDefaultMgefFlags(parentEffect.mgefCode) & EffectSetting::kMgefFlag_UseWeapon) // default flag is 'Use Weapon'
            parentEffect.SetFlag(EffectSetting::kMgefFlagShift_UseWeapon,true);
        else // default flag is 'Use Armor' OR there is no default flag setting
            parentEffect.SetFlag(EffectSetting::kMgefFlagShift_UseArmor,true);
        // check vanilla bound item group & switch groups if necessary
        if (parentEffect.MagicGroupList::GetMagicGroup(MagicGroup::g_BoundWeaponLimit) && parentEffect.GetFlag(EffectSetting::kMgefFlagShift_UseArmor))
        {
            // TODO - implement switch over armor slot to choose appripriate limit
            parentEffect.RemoveMagicGroup(MagicGroup::g_BoundWeaponLimit);
            parentEffect.AddMagicGroup(MagicGroup::g_BoundHelmLimit,1);
        }
    }
}
#ifndef OBLIVION
// child Dialog for CS editing
INT BoundItemMgefHandler::DialogTemplateID() { return IDD_MGEF_SMBO; }   // default handler has no dialog
void BoundItemMgefHandler::InitializeDialog(HWND dialog)
{
    HWND ctl = 0;
    // Weapons
    ctl = GetDlgItem(dialog,IDC_SMBO_WEAPON);
    TESComboBox::PopulateWithForms(ctl,TESForm::kFormType_Weapon,true,true);
    // Armor
    ctl = GetDlgItem(dialog,IDC_SMBO_ARMOR);
    TESComboBox::PopulateWithForms(ctl,TESForm::kFormType_Armor,true,true);
}
bool BoundItemMgefHandler::DialogMessageCallback(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{ 
    HWND ctl;

    // parse message
    switch (uMsg)
    {
    case WM_USERCOMMAND:
    case WM_COMMAND:
    {
        UInt32 commandCode = HIWORD(wParam);
        switch (LOWORD(wParam))  // switch on control id
        {
        case IDC_SMBO_USEWEAPON:  // Armor/Weapon toggle
        case IDC_SMBO_USEARMOR:
        {
            if (commandCode == BN_CLICKED)
            {
                _VMESSAGE("Weapon/Armor toggle clicked"); 
                bool clicked = false;
                // weapon 
                ctl = GetDlgItem(dialog,IDC_SMBO_WEAPON);
                clicked = IsDlgButtonChecked(dialog,IDC_SMBO_USEWEAPON);
                EnableWindow(ctl,clicked); 
                _DMESSAGE("Setting Weap sel to %08X", clicked ? parentEffect.mgefParam : 0);
                TESComboBox::SetCurSelByData(ctl, clicked ? TESForm::LookupByFormID(parentEffect.mgefParam) : 0);
                // armor
                ctl = GetDlgItem(dialog,IDC_SMBO_ARMOR);
                clicked = IsDlgButtonChecked(dialog,IDC_SMBO_USEARMOR);
                EnableWindow(ctl,clicked); 
                _DMESSAGE("Setting Armo sel to %08X", clicked ? parentEffect.mgefParam : 0);
                TESComboBox::SetCurSelByData(ctl, clicked ? TESForm::LookupByFormID(parentEffect.mgefParam) : 0);
                //
                result = 0; return true;   // retval = false signals command handled    
            }
            break;
        }
        }
    }
    }
    return false;
}
void BoundItemMgefHandler::SetInDialog(HWND dialog) 
{
    bool weapon = parentEffect.GetFlag(EffectSetting::kMgefFlagShift_UseWeapon);
    CheckDlgButton(dialog,IDC_SMBO_USEWEAPON,weapon);    // weapon
    CheckDlgButton(dialog,IDC_SMBO_USEARMOR,!weapon);    // armor
    SendNotifyMessage(dialog,WM_USERCOMMAND,MAKEWPARAM(IDC_SMBO_USEWEAPON,BN_CLICKED),(LPARAM)GetDlgItem(dialog,IDC_SMBO_USEWEAPON)); // update dialog state
}
void BoundItemMgefHandler::GetFromDialog(HWND dialog)
{
    // flags
    bool weapon = IsDlgButtonChecked(dialog,IDC_SMBO_USEWEAPON);
    parentEffect.SetFlag(EffectSetting::kMgefFlagShift_UseWeapon,weapon);
    parentEffect.SetFlag(EffectSetting::kMgefFlagShift_UseArmor,!weapon);
    // assoc item
    TESForm* assocItem = (TESForm*)TESComboBox::GetCurSelData(GetDlgItem(dialog, weapon ? IDC_SMBO_WEAPON : IDC_SMBO_ARMOR));
    parentEffect.mgefParam = assocItem ? assocItem->formID : 0;
}
#endif

/*************************** BoundItemEfitHandler ********************************/
/*************************** BoundItemEffect ********************************/
#ifdef OBLIVION
BoundItemEffect::BoundItemEffect(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem)
: ::BoundItemEffect(caster,magicItem,(::EffectItem*)effectItem) 
{
}
::ActiveEffect* BoundItemEffect::Clone() const
{
    ::ActiveEffect* neweff = Make(caster,magicItem,(OBME::EffectItem*)effectItem); 
    CopyTo(*neweff); 
    return neweff;
}
::ActiveEffect* BoundItemEffect::Make(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem)
{
    return new BoundItemEffect(caster,magicItem,effectItem);
}
#endif
}   //  end namespace OBME