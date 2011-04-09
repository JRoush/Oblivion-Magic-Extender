#include "OBME/EffectHandlers/ScriptEffect.h"
#include "OBME/EffectHandlers/EffectHandler.rc.h"
#include "OBME/EffectSetting.h"
#include "OBME/EffectItem.h"
#include "OBME/Magic.h"

#include "API/TESFiles/TESFile.h"
#include "API/CSDialogs/TESDialog.h"
#include "API/Actors/ActorValues.h"
#include "Components/TESFileFormats.h"
#include "Components/FormRefCounter.h"

// global method for returning small enumerations as booleans
inline bool BoolEx(UInt8 value) {return *(bool*)&value;}
// send in lieu of WM_COMMAND to avoid problems with TESFormIDListView::DlgProc 
static const UInt32 WM_USERCOMMAND =  WM_APP + 0x55; 

namespace OBME {

/*************************** ScriptMgefHandler ********************************/
ScriptMgefHandler::ScriptMgefHandler(EffectSetting& effect) 
: MgefHandler(effect) , scriptFormID(0), useSEFFAlwaysApplies(false), efitAVResType(TESFileFormats::kResType_None), 
    customParamResType(TESFileFormats::kResType_None), customParam(0)
{
    // By default, only the original SEFF effect uses the 'Always Applies' behavior
    if (effect.mgefCode == Swap32('SEFF')) useSEFFAlwaysApplies = true;
}
// serialization
bool ScriptMgefHandler::LoadHandlerChunk(TESFile& file, UInt32 RecordVersion)
{
    if (file.currentChunk.chunkLength != kEHNDChunkSize) return false;  // wrong chunk size
    file.GetChunkData(&scriptFormID,kEHNDChunkSize);
    TESFileFormats::ResolveModValue(scriptFormID,file,TESFileFormats::kResType_FormID);  // resolve script formID    
    TESFileFormats::ResolveModValue(customParam,file,customParamResType);  // resolve script formID
    return true;
}
void ScriptMgefHandler::SaveHandlerChunk()
{
    TESForm::PutFormRecordChunkData(Swap32('EHND'),&scriptFormID,kEHNDChunkSize);
}
void ScriptMgefHandler::LinkHandler()
{
    // add cross references
    if (TESForm* form = TESForm::LookupByFormID(scriptFormID)) FormRefCounter::AddReference(&parentEffect,form);
    if (TESForm* form = TESFileFormats::GetFormFromCode(customParam,customParamResType)) FormRefCounter::AddReference(&parentEffect,form);
}
void ScriptMgefHandler::UnlinkHandler() 
{
    // remove cross references
    if (TESForm* form = TESForm::LookupByFormID(scriptFormID)) FormRefCounter::RemoveReference(&parentEffect,form);
    if (TESForm* form = TESFileFormats::GetFormFromCode(customParam,customParamResType)) FormRefCounter::RemoveReference(&parentEffect,form);
}
// copy/compare
void ScriptMgefHandler::CopyFrom(const MgefHandler& copyFrom)
{
    const ScriptMgefHandler* src = dynamic_cast<const ScriptMgefHandler*>(&copyFrom);
    if (!src) return; // wrong polymorphic type

    // modify references
    if (TESForm* form = TESForm::LookupByFormID(scriptFormID)) FormRefCounter::RemoveReference(&parentEffect,form);
    if (TESForm* form = TESFileFormats::GetFormFromCode(customParam,customParamResType)) FormRefCounter::RemoveReference(&parentEffect,form);
    if (TESForm* form = TESForm::LookupByFormID(src->scriptFormID)) FormRefCounter::AddReference(&parentEffect,form);
    if (TESForm* form = TESFileFormats::GetFormFromCode(src->customParam,src->customParamResType)) FormRefCounter::AddReference(&parentEffect,form);

    // copy members
    scriptFormID = src->scriptFormID;
    useSEFFAlwaysApplies = src->useSEFFAlwaysApplies;
    efitAVResType = src->efitAVResType;
    customParamResType = src->customParamResType;
    customParam = src->customParam;
}
bool ScriptMgefHandler::CompareTo(const MgefHandler& compareTo)
{   
    if (MgefHandler::CompareTo(compareTo)) return BoolEx(2);
    const ScriptMgefHandler* src = dynamic_cast<const ScriptMgefHandler*>(&compareTo);
    if (!src) return BoolEx(4); // wrong polymorphic type

    // compare members
    if (scriptFormID != src->scriptFormID) return BoolEx(10);
    if (useSEFFAlwaysApplies != src->useSEFFAlwaysApplies) return BoolEx(20);
    if (efitAVResType != src->efitAVResType) return BoolEx(30);
    if (customParamResType != src->customParamResType) return BoolEx(40);
    if (customParam != src->customParam) return BoolEx(50);

    // handlers are identical
    return false;
}
#ifndef OBLIVION
// reference management in CS
void ScriptMgefHandler::RemoveFormReference(TESForm& form) 
{
    if (form.formID == scriptFormID) scriptFormID = 0;
    if (&form == TESFileFormats::GetFormFromCode(customParam,customParamResType)) 
    {
        customParam = (customParamResType == TESFileFormats::kResType_ActorValue) ? ActorValues::kActorVal__UBOUND : 0;
    }
}
// child Dialog in CS
INT ScriptMgefHandler::DialogTemplateID() { return IDD_MGEF_SEFF; }
void ScriptMgefHandler::InitializeDialog(HWND dialog)
{
    HWND ctl = 0;
    // Script
    ctl = GetDlgItem(dialog,IDC_SEFF_SCRIPT);
    TESComboBox::PopulateWithScripts(ctl,true,false,true);
    // EFIT AV res type
    ctl = GetDlgItem(dialog,IDC_SEFF_EFITAVRESOLUTION);
    TESComboBox::Clear(ctl);
    TESComboBox::AddItem(ctl,"No Resolution",(void*)TESFileFormats::kResType_None);
    TESComboBox::AddItem(ctl,"FormID",(void*)TESFileFormats::kResType_FormID);
    TESComboBox::AddItem(ctl,"Magic Effect Code",(void*)TESFileFormats::kResType_MgefCode);
    TESComboBox::AddItem(ctl,"Actor Value",(void*)TESFileFormats::kResType_ActorValue);
    // Custom param res type
    ctl = GetDlgItem(dialog,IDC_SEFF_SCRIPTPARAMRESOLUTION);
    TESComboBox::Clear(ctl);
    TESComboBox::AddItem(ctl,"Unsigned Hex Integer",(void*)TESFileFormats::kResType_None);
    TESComboBox::AddItem(ctl,"FormID",(void*)TESFileFormats::kResType_FormID);
    TESComboBox::AddItem(ctl,"Magic Effect Code",(void*)TESFileFormats::kResType_MgefCode);
    TESComboBox::AddItem(ctl,"Actor Value",(void*)TESFileFormats::kResType_ActorValue); 
    // custom param
    Edit_LimitText(GetDlgItem(dialog,IDC_SEFF_SCRIPTPARAM),8);  // limit custom param to 8 characters
}
bool ScriptMgefHandler::DialogMessageCallback(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    return false;
}
void ScriptMgefHandler::SetInDialog(HWND dialog)
{
    HWND ctl = 0;

    // Script
    ctl = GetDlgItem(dialog,IDC_SEFF_SCRIPT);
    TESComboBox::SetCurSelByData(ctl, TESForm::LookupByFormID(scriptFormID));
    // Use 'SEFF Always Applies'
    Button_SetCheck(GetDlgItem(dialog,IDC_SEFF_ALLOWALWAYSAPPLIES),useSEFFAlwaysApplies);
    // EFIT AV res type
    ctl = GetDlgItem(dialog,IDC_SEFF_EFITAVRESOLUTION);
    TESComboBox::SetCurSelByData(ctl,(void*)efitAVResType);
    // Custom param res type
    ctl = GetDlgItem(dialog,IDC_SEFF_SCRIPTPARAMRESOLUTION);
    TESComboBox::SetCurSelByData(ctl,(void*)customParamResType); 
    // Custom param
    char buffer[10];
    sprintf_s(buffer,sizeof(buffer),"%08X",customParam);
    ctl = GetDlgItem(dialog,IDC_SEFF_SCRIPTPARAM);
    SetWindowText(ctl,buffer);
}
void ScriptMgefHandler::GetFromDialog(HWND dialog)
{
    HWND ctl = 0;

    // Script
    TESForm* script = (TESForm*)TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_SEFF_SCRIPT));
    scriptFormID = (script) ? script->formID : 0;
    // Use 'SEFF Always Applies'
    useSEFFAlwaysApplies = IsDlgButtonChecked(dialog,IDC_SEFF_ALLOWALWAYSAPPLIES);
    // EFIT AV res type
    efitAVResType = (UInt8)TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_SEFF_EFITAVRESOLUTION));
    // Custom param res type
    customParamResType = (UInt8)TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_SEFF_SCRIPTPARAMRESOLUTION));
    // Custom param
    char buffer[10];
    GetWindowText(GetDlgItem(dialog,IDC_SEFF_SCRIPTPARAM),buffer,sizeof(buffer));
    customParam = strtoul(buffer,0,16);
}
#endif
/*************************** ScriptEfitHandler ********************************/
ScriptEfitHandler::ScriptEfitHandler(EffectItem& item) 
: EfitHandler(item), customParamResType(TESFileFormats::kResType_None), customParam(0) {}
// serialization
void ScriptEfitHandler::LinkHandler()
{
    // incr ref for script fomid override
    TESForm* parentForm = dynamic_cast<TESForm*>(parentItem.GetParentList());
    if (parentItem.IsEfitFieldOverridden(EffectItem::kEfitFlagShift_ScriptFormID))
    {
        if (TESForm* form = TESForm::LookupByFormID(parentItem.scriptInfo->scriptFormID)) FormRefCounter::AddReference(parentForm,form);
    }

    // incr cross ref for custom param form
    if (TESForm* form = TESFileFormats::GetFormFromCode(customParam,customParamResType)) FormRefCounter::AddReference(parentForm,form);
}
void ScriptEfitHandler::UnlinkHandler()
{
    // decr ref for script fomid override
    TESForm* parentForm = dynamic_cast<TESForm*>(parentItem.GetParentList());
    if (parentItem.IsEfitFieldOverridden(EffectItem::kEfitFlagShift_ScriptFormID))
    {
        if (TESForm* form = TESForm::LookupByFormID(parentItem.scriptInfo->scriptFormID)) FormRefCounter::RemoveReference(parentForm,form);
    }

    // decr cross ref for custom param form
    if (TESForm* form = TESFileFormats::GetFormFromCode(customParam,customParamResType)) FormRefCounter::RemoveReference(parentForm,form);
} 
// copy/compare
void ScriptEfitHandler::CopyFrom(const EfitHandler& copyFrom)
{
    const ScriptEfitHandler* src = dynamic_cast<const ScriptEfitHandler*>(&copyFrom);
    if (!src) return; // wrong polymorphic type

    // reference incr/decr
    TESForm* parentForm = dynamic_cast<TESForm*>(parentItem.GetParentList());
    // script fomid override
    if (parentItem.IsEfitFieldOverridden(EffectItem::kEfitFlagShift_ScriptFormID))
    {
        if (TESForm* form = TESForm::LookupByFormID(parentItem.scriptInfo->scriptFormID)) FormRefCounter::RemoveReference(parentForm,form);
    }
    if (src->parentItem.IsEfitFieldOverridden(EffectItem::kEfitFlagShift_ScriptFormID))
    {
        if (TESForm* form = TESForm::LookupByFormID(src->parentItem.scriptInfo->scriptFormID)) FormRefCounter::AddReference(parentForm,form);
    }

    // custom param form
    if (TESForm* form = TESFileFormats::GetFormFromCode(customParam,customParamResType)) FormRefCounter::RemoveReference(parentForm,form);
    if (TESForm* form = TESFileFormats::GetFormFromCode(src->customParam,src->customParamResType)) FormRefCounter::AddReference(parentForm,form);

    // script formid
    if (src->parentItem.IsEfitFieldOverridden(EffectItem::kEfitFlagShift_ScriptFormID))
    {
        parentItem.SetEfitFieldOverride(EffectItem::kEfitFlagShift_ScriptFormID,true);
        parentItem.scriptInfo->scriptFormID = src->parentItem.scriptInfo->scriptFormID;
    }
    else parentItem.SetEfitFieldOverride(EffectItem::kEfitFlagShift_ScriptFormID,false);

    // custom param
    customParamResType = src->customParamResType;
    customParam = src->customParam;
}
bool ScriptEfitHandler::CompareTo(const EfitHandler& compareTo)
{
    if (EfitHandler::CompareTo(compareTo)) return BoolEx(02);
    const ScriptEfitHandler* src = dynamic_cast<const ScriptEfitHandler*>(&compareTo);
    if (!src) return BoolEx(04); // wrong polymorphic type

    // script formID
    if (parentItem.IsEfitFieldOverridden(EffectItem::kEfitFlagShift_ScriptFormID) != 
        src->parentItem.IsEfitFieldOverridden(EffectItem::kEfitFlagShift_ScriptFormID)) return BoolEx(10);
    if (parentItem.IsEfitFieldOverridden(EffectItem::kEfitFlagShift_ScriptFormID) 
        && parentItem.scriptInfo->scriptFormID != src->parentItem.scriptInfo->scriptFormID) return BoolEx(11);

    // custom param
    if (customParamResType != src->customParamResType) return BoolEx(20);
    if (customParam != src->customParam) return BoolEx(21);

    return false;
}
bool ScriptEfitHandler::Match(const EfitHandler& compareTo)
{
    return true;    // TODO - use GMST to optionally compare script formIDs
}
// game/CS specific
#ifndef OBLIVION
// reference management in CS
void ScriptEfitHandler::RemoveFormReference(TESForm& form)
{
    if (parentItem.IsEfitFieldOverridden(EffectItem::kEfitFlagShift_ScriptFormID) && form.formID == parentItem.scriptInfo->scriptFormID)
        parentItem.SetEfitFieldOverride(EffectItem::kEfitFlagShift_ScriptFormID,false); // clear scritp formid override

    if (&form == TESFileFormats::GetFormFromCode(customParam,customParamResType)) 
    {
        customParam = (customParamResType == TESFileFormats::kResType_ActorValue) ? ActorValues::kActorVal__UBOUND : 0;
    }
}
// child dialog in CS
INT ScriptEfitHandler::DialogTemplateID() { return IDD_EFIT_SEFF; }
void ScriptEfitHandler::InitializeDialog(HWND dialog)
{
    HWND ctl = 0;
    // Script
    TESComboBox::PopulateWithScripts(GetDlgItem(dialog,IDC_SEFF_SCRIPT),true,false,true);   
    // Custom param res type
    ctl = GetDlgItem(dialog,IDC_SEFF_SCRIPTPARAMRESOLUTION);
    TESComboBox::Clear(ctl);
    TESComboBox::AddItem(ctl,"Unsigned Hex Integer",(void*)TESFileFormats::kResType_None);
    TESComboBox::AddItem(ctl,"FormID",(void*)TESFileFormats::kResType_FormID);
    TESComboBox::AddItem(ctl,"Magic Effect Code",(void*)TESFileFormats::kResType_MgefCode);
    TESComboBox::AddItem(ctl,"Actor Value",(void*)TESFileFormats::kResType_ActorValue); 
    // custom param
    Edit_LimitText(GetDlgItem(dialog,IDC_SEFF_SCRIPTPARAM),8);  // limit custom param to 8 characters
}
bool ScriptEfitHandler::DialogMessageCallback(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    if (uMsg == WM_USERCOMMAND || uMsg == WM_COMMAND)
    {
        UInt32 commandCode = HIWORD(wParam);
        switch (LOWORD(wParam)) // switch on control id
        {            
            case IDC_SEFF_OVR_SCRIPT:
            {
                bool over = Button_GetCheck(GetDlgItem(dialog,IDC_SEFF_OVR_SCRIPT));  
                EnableWindow(GetDlgItem(dialog,IDC_SEFF_SCRIPT),over);                
                result = 0; return true; // result = 0 to mark command message handled
            }
        }
    }  

    // unhandled message
    return false;
}
void ScriptEfitHandler::SetInDialog(HWND dialog)
{
    HWND ctl = 0;
    // script
    TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_SEFF_SCRIPT), GetScript());
    ctl = GetDlgItem(dialog,IDC_SEFF_OVR_SCRIPT);
    Button_SetCheck(ctl, parentItem.IsEfitFieldOverridden(EffectItem::kEfitFlagShift_ScriptFormID) ? BST_CHECKED : BST_UNCHECKED );
    SendMessage(dialog,WM_USERCOMMAND,MAKEWPARAM(IDC_SEFF_OVR_SCRIPT,BN_CLICKED),(LPARAM)ctl);
    // Custom param res type
    TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_SEFF_SCRIPTPARAMRESOLUTION),(void*)customParamResType); 
    // Custom param
    char buffer[10];
    sprintf_s(buffer,sizeof(buffer),"%08X",customParam);
    SetWindowText(GetDlgItem(dialog,IDC_SEFF_SCRIPTPARAM),buffer);
}
void ScriptEfitHandler::GetFromDialog(HWND dialog)
{
    // script
    if (Button_GetCheck(GetDlgItem(dialog,IDC_SEFF_OVR_SCRIPT))) 
    {
        parentItem.SetEfitFieldOverride(EffectItem::kEfitFlagShift_ScriptFormID,true);
        TESForm* script = (TESForm*)TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_SEFF_SCRIPT));
        parentItem.scriptInfo->scriptFormID = script ? script->formID : 0;
    }
    else parentItem.SetEfitFieldOverride(EffectItem::kEfitFlagShift_ScriptFormID,false);
    // Custom param res type
    customParamResType = (UInt8)TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_SEFF_SCRIPTPARAMRESOLUTION));
    // Custom param
    char buffer[10];
    GetWindowText(GetDlgItem(dialog,IDC_SEFF_SCRIPTPARAM),buffer,sizeof(buffer));
    customParam = strtoul(buffer,0,16);
}
#endif
// methods
Script* ScriptEfitHandler::GetScript()
{
    // TODO - because Script is not yet defined in COEF, the expected dynamic_cast<Script*> is ommitted
    // if the specified formid doesn't belong to a script, interesting things may happen
    if (parentItem.IsEfitFieldOverridden(EffectItem::kEfitFlagShift_ScriptFormID))
    {
        return (Script*)TESForm::LookupByFormID(parentItem.scriptInfo->scriptFormID);
    }
    else
    {
        ScriptMgefHandler& mgefHandler = (ScriptMgefHandler&)parentItem.GetEffectSetting()->GetHandler();
        return (Script*)TESForm::LookupByFormID(mgefHandler.scriptFormID);
    }   
}
/*************************** ScriptEffect ********************************/
#ifdef OBLIVION 
ScriptEffect::ScriptEffect(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem)
: ::ScriptEffect(caster,magicItem,(::EffectItem*)effectItem) 
{
}
::ActiveEffect* ScriptEffect::Clone() const
{
    ::ActiveEffect* neweff = Make(caster,magicItem,(OBME::EffectItem*)effectItem); 
    CopyTo(*neweff); 
    return neweff;
}
::ActiveEffect* ScriptEffect::Make(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem)
{
    return new ScriptEffect(caster,magicItem,effectItem);
}
#endif
}   //  end namespace OBME