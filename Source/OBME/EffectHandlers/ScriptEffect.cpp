#include "OBME/EffectHandlers/ScriptEffect.h"
#include "OBME/EffectHandlers/EffectHandler.rc.h"
#include "OBME/EffectSetting.h"

#include "API/TESFiles/TESFile.h"
#include "API/CSDialogs/TESDialog.h"
#include "API/Actors/ActorValues.h"
#include "Components/TESFileFormats.h"

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
    #ifndef OBLIVION
    if (TESForm* form = TESForm::LookupByFormID(scriptFormID)) form->AddCrossReference(&parentEffect);
    if (TESForm* form = TESFileFormats::GetFormFromCode(customParam,customParamResType)) form->AddCrossReference(&parentEffect);
    #endif
}
void ScriptMgefHandler::UnlinkHandler() 
{
    // remove cross references
    #ifndef OBLIVION
    if (TESForm* form = TESForm::LookupByFormID(scriptFormID)) form->RemoveCrossReference(&parentEffect);
    if (TESForm* form = TESFileFormats::GetFormFromCode(customParam,customParamResType)) form->RemoveCrossReference(&parentEffect);
    #endif
}
// copy/compare
void ScriptMgefHandler::CopyFrom(const MgefHandler& copyFrom)
{
    const ScriptMgefHandler* src = dynamic_cast<const ScriptMgefHandler*>(&copyFrom);
    if (!src) return; // wrong polymorphic type

    // modify references
    #ifndef OBLIVION
    if (TESForm* form = TESForm::LookupByFormID(scriptFormID)) form->RemoveCrossReference(&parentEffect);
    if (TESForm* form = TESFileFormats::GetFormFromCode(customParam,customParamResType)) form->RemoveCrossReference(&parentEffect);
    if (TESForm* form = TESForm::LookupByFormID(src->scriptFormID)) form->AddCrossReference(&parentEffect);
    if (TESForm* form = TESFileFormats::GetFormFromCode(src->customParam,src->customParamResType)) form->AddCrossReference(&parentEffect);
    #endif

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