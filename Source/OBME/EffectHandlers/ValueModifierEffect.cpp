#include "OBME/EffectHandlers/ValueModifierEffect.h"
#include "OBME/EffectHandlers/EffectHandler.rc.h"
#include "OBME/EffectSetting.h"
#include "OBME/EffectItem.h"

#include "API/TESFiles/TESFile.h"
#include "API/CSDialogs/TESDialog.h"
#include "API/Actors/ActorValues.h"
#include "Components/TESFileFormats.h"

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
    if (file.currentChunk.chunkLength != kEHNDChunkSize) return false;  // wrong chunk size
    file.GetChunkData(&magIsPercentage,kEHNDChunkSize);
    return true;
}
void ValueModifierMgefHandler::SaveHandlerChunk()
{
    TESForm::PutFormRecordChunkData(Swap32('EHND'),&magIsPercentage,kEHNDChunkSize);
}
void ValueModifierMgefHandler::LinkHandler()
{
    // add cross reference for mgefParam
    #ifndef OBLIVION
    if (TESForm* form = TESFileFormats::GetFormFromCode(parentEffect.mgefParam,TESFileFormats::kResType_ActorValue)) 
        form->AddCrossReference(&parentEffect);
    #endif
}
void ValueModifierMgefHandler::UnlinkHandler() 
{
    // remove cross reference for mgefParam
    #ifndef OBLIVION
    if (TESForm* form = TESFileFormats::GetFormFromCode(parentEffect.mgefParam,TESFileFormats::kResType_ActorValue)) 
        form->RemoveCrossReference(&parentEffect);
    #endif
}
// copy/compare
void ValueModifierMgefHandler::CopyFrom(const MgefHandler& copyFrom)
{
    const ValueModifierMgefHandler* src = dynamic_cast<const ValueModifierMgefHandler*>(&copyFrom);
    if (!src) return; // wrong polymorphic type

    // incr/decr cross refs for mgefParam
    #ifndef OBLIVION
    if (TESForm* form = TESFileFormats::GetFormFromCode(parentEffect.mgefParam,TESFileFormats::kResType_ActorValue)) 
        form->RemoveCrossReference(&parentEffect);
    if (TESForm* form = TESFileFormats::GetFormFromCode(src->parentEffect.mgefParam,TESFileFormats::kResType_ActorValue)) 
        form->AddCrossReference(&parentEffect);
    #endif

    magIsPercentage = src->magIsPercentage;
    percentageAVPart = src->percentageAVPart;
    avPart = src->avPart;
    detrimentalLimit  = src->detrimentalLimit;
}
bool ValueModifierMgefHandler::CompareTo(const MgefHandler& compareTo)
{   
    if (MgefHandler::CompareTo(compareTo)) return BoolEx(02);
    const ValueModifierMgefHandler* src = dynamic_cast<const ValueModifierMgefHandler*>(&compareTo);
    if (!src) return BoolEx(04); // wrong polymorphic type

    // compare members
    if (magIsPercentage != src->magIsPercentage) return BoolEx(10);
    if (percentageAVPart != src->percentageAVPart) return BoolEx(20);
    if (avPart != src->avPart) return BoolEx(30);
    if (detrimentalLimit != src->detrimentalLimit) return BoolEx(40);

    // handlers are identical
    return false;
}
#ifndef OBLIVION
// reference management in CS
void ValueModifierMgefHandler::RemoveFormReference(TESForm& form) 
{
    // clear mgefParam if it references AV token form
    if (&form == TESFileFormats::GetFormFromCode(parentEffect.mgefParam,TESFileFormats::kResType_ActorValue)) 
        parentEffect.mgefParam = ActorValues::kActorVal__UBOUND;
}
// child Dialog in CS
INT ValueModifierMgefHandler::DialogTemplateID() { return IDD_MGEF_MDAV; }
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
    int src = 0;
    if (parentEffect.GetFlag(EffectSetting::kMgefFlagShift_UseAttribute)) src = 0x001;
    if (parentEffect.GetFlag(EffectSetting::kMgefFlagShift_UseSkill)) src =  0x010;
    if (!src || parentEffect.GetFlag(EffectSetting::kMgefFlagShift_UseActorValue)) src = 0x100; // default
    TESComboBox::SetCurSelByData(ctl,(void*)src);
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
ValueModifierEfitHandler::ValueModifierEfitHandler(EffectItem& item) 
: EfitHandler(item), overrideFlags(0), avPart(ValueModifierMgefHandler::kAVPart_Damage), detrimental(false), recovers(false)
{
    // default value of ActorValue override flag is set by SetParentItemDefaultFields(), at the same time it sets the initial actorValue code
}
void ValueModifierEfitHandler::SetParentItemDefaultFields()
{
    // set default avCode for effects that use explicit ev ranges
    overrideFlags |= kOverride_ActorValue;
    if (parentItem.GetMgefFlag(EffectSetting::kMgefFlagShift_UseSkill)) parentItem.actorValue = ActorValues::kActorVal_Armorer;
    else if (parentItem.GetMgefFlag(EffectSetting::kMgefFlagShift_UseAttribute)) parentItem.actorValue = ActorValues::kActorVal_Strength;
    else overrideFlags &= !kOverride_ActorValue; // doesn't use local AV
}
// serialization
void ValueModifierEfitHandler::LinkHandler()
{
    // add cross reference for actorValue
    #ifndef OBLIVION
    if (overrideFlags & kOverride_ActorValue)
    {
        TESForm* parentForm = dynamic_cast<TESForm*>(parentItem.GetParentList());
        TESForm* form = TESFileFormats::GetFormFromCode(parentItem.actorValue,TESFileFormats::kResType_ActorValue);
        if (parentForm && form) form->AddCrossReference(parentForm);
    }
    #endif
}
void ValueModifierEfitHandler::UnlinkHandler()
{
    // remove cross reference for actorValue
    #ifndef OBLIVION
    if (overrideFlags & kOverride_ActorValue)
    {
        TESForm* parentForm = dynamic_cast<TESForm*>(parentItem.GetParentList());
        TESForm* form = TESFileFormats::GetFormFromCode(parentItem.actorValue,TESFileFormats::kResType_ActorValue);
        if (parentForm && form) form->RemoveCrossReference(parentForm);
    }
    #endif
}
// copy/compare
void ValueModifierEfitHandler::CopyFrom(const EfitHandler& copyFrom)
{
    const ValueModifierEfitHandler* src = dynamic_cast<const ValueModifierEfitHandler*>(&copyFrom);
    if (!src) return; // wrong polymorphic type

    // actorValue
    #ifndef OBLIVION
    TESForm* parentForm = dynamic_cast<TESForm*>(parentItem.GetParentList());
    if (overrideFlags & kOverride_ActorValue)
    {        
        TESForm* form = TESFileFormats::GetFormFromCode(parentItem.actorValue,TESFileFormats::kResType_ActorValue);
        if (parentForm && form) form->RemoveCrossReference(parentForm);
    }
    if (src->overrideFlags & kOverride_ActorValue)
    {        
        TESForm* form = TESFileFormats::GetFormFromCode(src->parentItem.actorValue,TESFileFormats::kResType_ActorValue);
        if (parentForm && form) form->AddCrossReference(parentForm);
    }
    #endif
    parentItem.actorValue = src->parentItem.actorValue;

    // members
    overrideFlags = src->overrideFlags;
    avPart = src->avPart;
    detrimental = src->detrimental;
    recovers = src->recovers;
}
bool ValueModifierEfitHandler::CompareTo(const EfitHandler& compareTo)
{
    if (EfitHandler::CompareTo(compareTo)) return BoolEx(02);
    const ValueModifierEfitHandler* src = dynamic_cast<const ValueModifierEfitHandler*>(&compareTo);
    if (!src) return BoolEx(04); // wrong polymorphic type

    // compare override masks
    if (overrideFlags != src->overrideFlags) return BoolEx(10);

    // compare actor values
    if ((overrideFlags & kOverride_ActorValue) && parentItem.actorValue != src->parentItem.actorValue) return BoolEx(20);
    
    // compare members
    if ((overrideFlags & kOverride_AVPart) && avPart != src->avPart) return BoolEx(21);
    if ((overrideFlags & kOverride_Detrimental) && detrimental != src->detrimental) return BoolEx(22);
    if ((overrideFlags & kOverride_Recovers) && recovers != src->recovers) return BoolEx(23);

    // identical
    return false;
}
bool ValueModifierEfitHandler::Match(const EfitHandler& compareTo)
{
    if (!EfitHandler::Match(compareTo)) return false;
    const ValueModifierEfitHandler* src = dynamic_cast<const ValueModifierEfitHandler*>(&compareTo);
    if (!src) return BoolEx(04); // wrong polymorphic type

    // return true if doesn't override AV, or if avs are identical
    return (~overrideFlags & kOverride_ActorValue) || (parentItem.actorValue == src->parentItem.actorValue);
}
// game/CS specific
#ifndef OBLIVION
// reference management in CS
void ValueModifierEfitHandler::RemoveFormReference(TESForm& form)
{
    // clear actorValue if it is in use and it references AV token form
    if (overrideFlags & kOverride_ActorValue)
    {
        if (&form == TESFileFormats::GetFormFromCode(parentItem.actorValue,TESFileFormats::kResType_ActorValue))
            parentItem.actorValue = ActorValues::kActorVal__UBOUND;    // reset value to 'none'
    }    
}
// child dialog in CS
INT ValueModifierEfitHandler::DialogTemplateID() { return IDD_EFIT_MDAV; }
void ValueModifierEfitHandler::InitializeDialog(HWND dialog)
{
    HWND ctl = 0;
    // AV
    TESComboBox::PopulateWithActorValues(GetDlgItem(dialog,IDC_MDAV_ACTORVALUE),true,true);
    // AV Part
    ctl = GetDlgItem(dialog,IDC_MDAV_AVPART);
    TESComboBox::Clear(ctl);
    TESComboBox::AddItem(ctl,"Base Value",(void*)ValueModifierMgefHandler::kAVPart_Base);
    TESComboBox::AddItem(ctl,"Max Modifier",(void*)ValueModifierMgefHandler::kAVPart_Max);
    TESComboBox::AddItem(ctl,"Max Modifier (Base for Player Abilities)",(void*)ValueModifierMgefHandler::kAVPart_MaxSpecial);
    TESComboBox::AddItem(ctl,"Script Modifier",(void*)ValueModifierMgefHandler::kAVPart_Script);
    TESComboBox::AddItem(ctl,"Damage Modifier",(void*)ValueModifierMgefHandler::kAVPart_Damage);
}
bool ValueModifierEfitHandler::DialogMessageCallback(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    if (uMsg == WM_USERCOMMAND || uMsg == WM_COMMAND)
    {
        UInt32 commandCode = HIWORD(wParam);
        switch (LOWORD(wParam)) // switch on control id
        {            
            case IDC_MDAV_OVR_ACTORVALUE:
            {
                bool over = Button_GetCheck(GetDlgItem(dialog,IDC_MDAV_OVR_ACTORVALUE));  
                EnableWindow(GetDlgItem(dialog,IDC_MDAV_ACTORVALUE),over);                
                result = 0; return true; // result = 0 to mark command message handled
            }
            case IDC_MDAV_OVR_AVPART:
            {
                bool over = Button_GetCheck(GetDlgItem(dialog,IDC_MDAV_OVR_AVPART));
                EnableWindow(GetDlgItem(dialog,IDC_MDAV_AVPART),over);
                result = 0; return true; // result = 0 to mark command message handled
            }
            case IDC_MDAV_OVR_RAISELOWER:
            {
                bool over = Button_GetCheck(GetDlgItem(dialog,IDC_MDAV_OVR_RAISELOWER));
                EnableWindow(GetDlgItem(dialog,IDC_MDAV_DETRIMENTAL),over);
                EnableWindow(GetDlgItem(dialog,IDC_MDAV_INCREMENTAL),over);
                result = 0; return true; // result = 0 to mark command message handled
            }
            case IDC_MDAV_OVR_RECOVERS:
            {
                bool over = Button_GetCheck(GetDlgItem(dialog,IDC_MDAV_OVR_RECOVERS));
                EnableWindow(GetDlgItem(dialog,IDC_MDAV_RECOVERS),over);
                result = 0; return true; // result = 0 to mark command message handled
            }
        }
    }  

    // unhandled message
    return false;
}
void ValueModifierEfitHandler::SetInDialog(HWND dialog)
{
    HWND ctl = 0;
    // AV
    TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MDAV_ACTORVALUE), (void*)GetActorValue());
    ctl = GetDlgItem(dialog,IDC_MDAV_OVR_ACTORVALUE);
    Button_SetCheck(ctl, (overrideFlags & kOverride_ActorValue) ? BST_CHECKED : BST_UNCHECKED );
    SendMessage(dialog,WM_USERCOMMAND,MAKEWPARAM(IDC_MDAV_OVR_ACTORVALUE,BN_CLICKED),(LPARAM)ctl);
    // AV Part
    TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MDAV_AVPART), (void*)GetAVPart());
    ctl = GetDlgItem(dialog,IDC_MDAV_OVR_AVPART);
    Button_SetCheck(ctl, (overrideFlags & kOverride_AVPart) ? BST_CHECKED : BST_UNCHECKED );
    SendMessage(dialog,WM_USERCOMMAND,MAKEWPARAM(IDC_MDAV_OVR_AVPART,BN_CLICKED),(LPARAM)ctl);
    // Detrimental option group
    bool det = GetDetrimental();
    Button_SetCheck(GetDlgItem(dialog,IDC_MDAV_DETRIMENTAL),det);
    Button_SetCheck(GetDlgItem(dialog,IDC_MDAV_INCREMENTAL),!det);
    ctl = GetDlgItem(dialog,IDC_MDAV_OVR_RAISELOWER);
    Button_SetCheck(ctl, (overrideFlags & kOverride_Detrimental) ? BST_CHECKED : BST_UNCHECKED );
    SendMessage(dialog,WM_USERCOMMAND,MAKEWPARAM(IDC_MDAV_OVR_RAISELOWER,BN_CLICKED),(LPARAM)ctl);
    // Recovers
    Button_SetCheck(GetDlgItem(dialog,IDC_MDAV_RECOVERS), GetRecoverable() ? BST_CHECKED : BST_UNCHECKED  );
    ctl = GetDlgItem(dialog,IDC_MDAV_OVR_RECOVERS);
    Button_SetCheck(ctl, (overrideFlags & kOverride_Recovers) ? BST_CHECKED : BST_UNCHECKED );
    SendMessage(dialog,WM_USERCOMMAND,MAKEWPARAM(IDC_MDAV_OVR_RECOVERS,BN_CLICKED),(LPARAM)ctl);
}
void ValueModifierEfitHandler::GetFromDialog(HWND dialog)
{
    // AV
    if (Button_GetCheck(GetDlgItem(dialog,IDC_MDAV_OVR_ACTORVALUE))) 
    {
        overrideFlags |= kOverride_ActorValue;
        parentItem.actorValue = (UInt32)TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_MDAV_ACTORVALUE));
    }
    else overrideFlags &= ~kOverride_ActorValue;

    // AV Part
    if (Button_GetCheck(GetDlgItem(dialog,IDC_MDAV_OVR_AVPART))) 
    {
        overrideFlags |= kOverride_AVPart;
        avPart = (UInt32)TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_MDAV_AVPART));
    }
    else overrideFlags &= ~kOverride_AVPart;

    // Detrimental
    if (Button_GetCheck(GetDlgItem(dialog,IDC_MDAV_OVR_RAISELOWER))) 
    {
        overrideFlags |= kOverride_Detrimental;
        detrimental = Button_GetCheck(GetDlgItem(dialog,IDC_MDAV_DETRIMENTAL));
    }
    else overrideFlags &= ~kOverride_Detrimental;

    // Recovers
    if (Button_GetCheck(GetDlgItem(dialog,IDC_MDAV_OVR_RECOVERS))) 
    {
        overrideFlags |= kOverride_Recovers;
        recovers = Button_GetCheck(GetDlgItem(dialog,IDC_MDAV_RECOVERS));
    }
    else overrideFlags &= ~kOverride_Recovers;
}
#endif
// methods
UInt32 ValueModifierEfitHandler::GetActorValue()
{
    return (overrideFlags & kOverride_ActorValue) ? parentItem.actorValue : parentItem.GetEffectSetting()->mgefParam;
}
UInt8 ValueModifierEfitHandler::GetAVPart()
{
    if (overrideFlags & kOverride_AVPart) return avPart;
    ValueModifierMgefHandler& mgefHandler = (ValueModifierMgefHandler&)parentItem.GetEffectSetting()->GetHandler();
    return mgefHandler.avPart;
}
bool ValueModifierEfitHandler::GetDetrimental()
{
    return (overrideFlags & kOverride_Detrimental) ? detrimental : parentItem.GetMgefFlag(EffectSetting::kMgefFlagShift_Detrimental);
}
bool ValueModifierEfitHandler::GetRecoverable()
{
    return (overrideFlags & kOverride_Recovers) ? recovers : parentItem.GetMgefFlag(EffectSetting::kMgefFlagShift_Recovers);
}
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