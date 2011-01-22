#include "OBME/EffectHandlers/ValueModifierEffect.h"
#include "OBME/EffectHandlers/EffectHandler.rc.h"
#include "OBME/EffectSetting.h"

#include "API/TESFiles/TESFile.h"
#include "API/CSDialogs/TESDialog.h"

// global method for returning small enumerations as booleans
inline bool BoolEx(UInt8 value) {return *(bool*)&value;}
// send in lieu of WM_COMMAND to avoid problems with TESFormIDListView::DlgProc 
static const UInt32 WM_USERCOMMAND =  WM_APP + 0x55; 

namespace OBME {

/*************************** ValueModifierMgefHandler ********************************/
ValueModifierMgefHandler::ValueModifierMgefHandler(EffectSetting& effect) 
 : MgefHandler(effect) , magIsPercentage(false), percentageAVPart(kAVPart_Base), avPart(kAVPart_Damage), detrimentalLimit(-1)
{
    // By default, recoverable effects target the Max modifier, with special behavior for abilities on the player
    if (effect.GetFlag(EffectSetting::kMgefFlagShift_Recovers)) avPart = kAVPart_MaxSpecial;
}
// serialization
bool ValueModifierMgefHandler::LoadHandlerChunk(TESFile& file, UInt32 RecordVersion)
{
    file.GetChunkData(&magIsPercentage,kEHNDChunkSize);
    return true;
}
void ValueModifierMgefHandler::SaveHandlerChunk()
{
    TESForm::PutFormRecordChunkData(Swap32('EHND'),&magIsPercentage,kEHNDChunkSize);
}
void ValueModifierMgefHandler::LinkHandler()
{
    // TODO - lookup AV token and register reference in CS
}
// copy/compare
void ValueModifierMgefHandler::CopyFrom(const MgefHandler& copyFrom)
{
    const ValueModifierMgefHandler* src = dynamic_cast<const ValueModifierMgefHandler*>(&copyFrom);
    if (!src) return; // wrong polymorphic type

    // TODO - CrossRef incr/decr for mgefParam AV token

    magIsPercentage = src->magIsPercentage;
    percentageAVPart = src->percentageAVPart;
    avPart = src->avPart;
    detrimentalLimit  = src->detrimentalLimit;
}
bool ValueModifierMgefHandler::CompareTo(const MgefHandler& compareTo)
{   
    // define failure codes - all of which must evaluate to boolean true
    enum
    {
        kCompareSuccess         = 0,
        kCompareFail_General    = 1,
        kCompareFail_Polymorphic,
        kCompareFail_MagIsPerc,
        kCompareFail_PercAVPart,
        kCompareFail_AVPart,
        kCompareFail_DetrimentalLimit,
    };

    const ValueModifierMgefHandler* src = dynamic_cast<const ValueModifierMgefHandler*>(&compareTo);
    if (!src) return BoolEx(kCompareFail_Polymorphic); // wrong polymorphic type

    // compare members
    if (magIsPercentage != src->magIsPercentage) return BoolEx(kCompareFail_MagIsPerc);
    if (percentageAVPart != src->percentageAVPart) return BoolEx(kCompareFail_PercAVPart);
    if (avPart != src->avPart) return BoolEx(kCompareFail_AVPart);
    if (detrimentalLimit != src->detrimentalLimit) return BoolEx(kCompareFail_DetrimentalLimit);

    // handlers are identical
    return BoolEx(kCompareSuccess);
}
#ifndef OBLIVION
// reference management in CS
void ValueModifierMgefHandler::RemoveFormReference(TESForm& form) 
{
    // TODO - clear mgefParam if it is an AV token
}
// child Dialog in CS
INT ValueModifierMgefHandler::DialogTemplateID() { return IDD_MDAV; }
void ValueModifierMgefHandler::InitializeDialog(HWND dialog)
{
    HWND ctl = 0;
    // AV Source
    ctl = GetDlgItem(dialog,IDC_MDAV_AVSOURCE);
    TESComboBox::Clear(ctl);
    TESComboBox::AddItem(ctl,"Single Actor Value",(void*)0x100);
    TESComboBox::AddItem(ctl,"All Skills",(void*)0x010);
    TESComboBox::AddItem(ctl,"All Attributes",(void*)0x001);
    // AV
    ctl = GetDlgItem(dialog,IDC_MDAV_ACTORVALUE);
    TESComboBox::PopulateWithActorValues(ctl,true,true);
    // AV Part
    ctl = GetDlgItem(dialog,IDC_MDAV_AVPART);
    TESComboBox::Clear(ctl);
    TESComboBox::AddItem(ctl,"Base Value",(void*)kAVPart_Base);
    TESComboBox::AddItem(ctl,"Max Modifier",(void*)kAVPart_Max);
    TESComboBox::AddItem(ctl,"Max Modifier (Base for Player Abilities)",(void*)kAVPart_MaxSpecial);
    TESComboBox::AddItem(ctl,"Script Modifier",(void*)kAVPart_Script);
    TESComboBox::AddItem(ctl,"Damage Modifier",(void*)kAVPart_Damage);
    // Percentage Part
    ctl = GetDlgItem(dialog,IDC_MDAV_PERCAVPART);
    TESComboBox::Clear(ctl);
    TESComboBox::AddItem(ctl,"Base Value",(void*)kAVPart_Base);
    TESComboBox::AddItem(ctl,"Max Modifier",(void*)kAVPart_Max);
    TESComboBox::AddItem(ctl,"Script Modifier",(void*)kAVPart_Script);
    TESComboBox::AddItem(ctl,"Damage Modifier",(void*)kAVPart_Damage);
    TESComboBox::AddItem(ctl,"Calculated Base Value",(void*)kAVPart_CalcBase);
    TESComboBox::AddItem(ctl,"Current Value",(void*)kAVPart_CurVal);

}
bool ValueModifierMgefHandler::DialogMessageCallback(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
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
        case IDC_MDAV_MAGISPERC:  // Magnitude Is Percentage checkbox
        {
            if (commandCode == BN_CLICKED)
            {
                _VMESSAGE("Magnitude Is Percentage check clicked");  
                ctl = GetDlgItem(dialog,IDC_MDAV_PERCAVPART);
                EnableWindow(ctl,IsDlgButtonChecked(dialog,IDC_MDAV_MAGISPERC));             
                result = 0; return true;   // retval = false signals command handled    
            }
            break;
        }
        case IDC_MDAV_AVSOURCE:  // AV source combo box
        {
            if (commandCode == CBN_SELCHANGE)
            {
                _VMESSAGE("AV source changed");
                bool useAV = TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_MDAV_AVSOURCE)) == (void*)0x100;  
                ctl = GetDlgItem(dialog,IDC_MDAV_ACTORVALUE);
                TESComboBox::SetCurSelByData(ctl,useAV ? (void*)parentEffect.mgefParam : 0);
                EnableWindow(ctl,useAV);
                result = 0; return true;  // retval = false signals command handled
            }
            break;
        }
        }
    }
    }
    return false;
}
void ValueModifierMgefHandler::SetInDialog(HWND dialog)
{
    HWND ctl = 0;

    // AV Source
    ctl = GetDlgItem(dialog,IDC_MDAV_AVSOURCE);
    if (parentEffect.GetFlag(EffectSetting::kMgefFlagShift_UseAttribute)) TESComboBox::SetCurSelByData(ctl,(void*)0x001);
    if (parentEffect.GetFlag(EffectSetting::kMgefFlagShift_UseSkill)) TESComboBox::SetCurSelByData(ctl,(void*)0x010);
    if (parentEffect.GetFlag(EffectSetting::kMgefFlagShift_UseActorValue)) TESComboBox::SetCurSelByData(ctl,(void*)0x100);
    SendNotifyMessage(dialog,WM_USERCOMMAND,MAKEWPARAM(IDC_MDAV_AVSOURCE,CBN_SELCHANGE),(LPARAM)GetDlgItem(dialog,IDC_MDAV_AVSOURCE)); // update AV combo
    // AV Part
    ctl = GetDlgItem(dialog,IDC_MDAV_AVPART);
    TESComboBox::SetCurSelByData(ctl,(void*)avPart);
    // Percentage
    Button_SetCheck(GetDlgItem(dialog,IDC_MDAV_MAGISPERC),magIsPercentage);
    SendNotifyMessage(dialog,WM_USERCOMMAND,MAKEWPARAM(IDC_MDAV_MAGISPERC,BN_CLICKED),(LPARAM)GetDlgItem(dialog,IDC_MDAV_MAGISPERC)); // update PercAVPart combo
    // Percentage Part
    ctl = GetDlgItem(dialog,IDC_MDAV_PERCAVPART);
    TESComboBox::SetCurSelByData(ctl,(void*)percentageAVPart); 
    // Detrimental option group
    bool det = parentEffect.GetFlag(EffectSetting::kMgefFlagShift_Detrimental);
    Button_SetCheck(GetDlgItem(dialog,IDC_MDAV_DETRIMENTALLIM0),det && detrimentalLimit == 0);
    Button_SetCheck(GetDlgItem(dialog,IDC_MDAV_DETRIMENTALLIM1),det && detrimentalLimit == 1);
    Button_SetCheck(GetDlgItem(dialog,IDC_MDAV_DETRIMENTAL),det && detrimentalLimit != 1 && detrimentalLimit != 0);
    Button_SetCheck(GetDlgItem(dialog,IDC_MDAV_INCREMENTAL),!det);
    // Recovers
    Button_SetCheck(GetDlgItem(dialog,IDC_MDAV_RECOVERS),parentEffect.GetFlag(EffectSetting::kMgefFlagShift_Recovers));

    return;
}
void ValueModifierMgefHandler::GetFromDialog(HWND dialog)
{
    HWND ctl = 0;

    // AV Source
    ctl = GetDlgItem(dialog,IDC_MDAV_AVSOURCE);
    UInt32 source = (UInt32)TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_MDAV_AVSOURCE));
    parentEffect.SetFlag(EffectSetting::kMgefFlagShift_UseAttribute,source == 0x001);
    parentEffect.SetFlag(EffectSetting::kMgefFlagShift_UseSkill,source == 0x010);
    parentEffect.SetFlag(EffectSetting::kMgefFlagShift_UseActorValue,source == 0x100);
    // Actor Value
    parentEffect.mgefParam = (UInt32)TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_MDAV_ACTORVALUE));
    // AV Part
    avPart = (UInt8)TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_MDAV_AVPART));
    // Percentage
    magIsPercentage = IsDlgButtonChecked(dialog,IDC_MDAV_MAGISPERC);
    // Percentage Part
    percentageAVPart = (UInt8)TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_MDAV_PERCAVPART));
    // Detrimental option group
    detrimentalLimit = -1;
    parentEffect.SetFlag(EffectSetting::kMgefFlagShift_Detrimental,!IsDlgButtonChecked(dialog,IDC_MDAV_INCREMENTAL));
    if (IsDlgButtonChecked(dialog,IDC_MDAV_DETRIMENTALLIM0)) detrimentalLimit = 0;
    if (IsDlgButtonChecked(dialog,IDC_MDAV_DETRIMENTALLIM1)) detrimentalLimit = 1;
    // Recovers
    parentEffect.SetFlag(EffectSetting::kMgefFlagShift_Recovers,IsDlgButtonChecked(dialog,IDC_MDAV_RECOVERS));

    return;
}
#endif

/*************************** ValueModifierEfitHandler ********************************/
/*************************** ValueModifierEffect ********************************/
#ifdef OBLIVION
float ValueModifierActvHandler::GetDefaultMagnitude() {return 0;} 
ValueModifierEffect::ValueModifierEffect(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem)
: ::ValueModifierEffect(caster,magicItem,(::EffectItem*)effectItem) 
{
}
::ActiveEffect* ValueModifierEffect::Clone() const
{
    ::ActiveEffect* neweff = Make(caster,magicItem,(OBME::EffectItem*)effectItem); 
    CopyTo(*neweff); 
    return neweff;
}
::ActiveEffect* ValueModifierEffect::Make(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem)
{
    return new ValueModifierEffect(caster,magicItem,effectItem);
}
#endif
}   //  end namespace OBME